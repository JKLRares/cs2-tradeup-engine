#include "tradeup/application.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <random>
#include <streambuf>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace tradeup {
std::vector<Allocation> parse_allocations(const std::string_view text) {
    std::vector<Allocation> result;
    for (const auto& item : split_fields(text, ',')) {
        const auto f = split_fields(item, ':');
        int quantity = 0;
        if (f.size() != 2 || f[1].empty()) { throw std::invalid_argument("contract requires bucket:quantity pairs separated by commas"); }
        const auto parsed = std::from_chars(f[1].data(), f[1].data()+f[1].size(), quantity);
        if (parsed.ec != std::errc{} || parsed.ptr != f[1].data()+f[1].size() || quantity <= 0 || quantity > 10) {
            throw std::invalid_argument("contract quantities must be decimal integers in 1..10");
        }
        result.push_back({BucketId{f[0]},quantity});
        if (result.size() > 10) { throw std::invalid_argument("at most ten contract entries are supported"); }
    }
    return result;
}
std::vector<std::size_t> select_results(const OptimizedEngine& engine, const OptimizedResult& result, const SearchQuery& query) {
    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < result.qualifying.size(); ++i) {
        if (!query.max_cost || result.qualifying[i].cost <= *query.max_cost) { order.push_back(i); }
    }
    std::sort(order.begin(), order.end(), [&](const std::size_t a, const std::size_t b) {
        const auto& x = result.qualifying[a]; const auto& y = result.qualifying[b];
        const auto vx = query.ranking == Ranking::roi ? x.roi_percent : x.expected_profit;
        const auto vy = query.ranking == Ranking::roi ? y.roi_percent : y.expected_profit;
        if (vx != vy) { return vx > vy; }
        return engine.contract(x).serialize() < engine.contract(y).serialize();
    });
    return order;
}
std::string decimal_display(const Rational value, const std::size_t places) {
    if (places > 12) { throw std::invalid_argument("display precision exceeds 12 places"); }
    const bool negative = value.numerator() < 0;
    const auto n = static_cast<std::uint64_t>(negative ? -value.numerator() : value.numerator());
    const auto d = static_cast<std::uint64_t>(value.denominator());
    auto remainder = n % d;
    std::string text = std::to_string(n/d);
    if (places) { text += '.'; }
    const auto digit = [&] {
        std::uint64_t accumulator = 0; int result = 0;
        for (int i = 0; i < 10; ++i) {
            if (accumulator >= d-remainder) { accumulator -= d-remainder; ++result; }
            else { accumulator += remainder; }
        }
        remainder = accumulator; return result;
    };
    for (std::size_t i = 0; i < places; ++i) { text += static_cast<char>('0'+digit()); }
    if (digit() >= 5) {
        bool carry = true;
        for (std::size_t i = text.size(); i > 0 && carry; --i) {
            if (text[i-1] == '.') { continue; }
            if (text[i-1] == '9') { text[i-1] = '0'; } else { ++text[i-1]; carry = false; }
        }
        if (carry) { text.insert(text.begin(),'1'); }
    }
    return negative ? "-"+text : text;
}
namespace {
class LimitedBuffer final : public std::streambuf {
public:
    LimitedBuffer(std::streambuf* sink, const std::size_t limit) : sink_(sink), limit_(limit) {}
private:
    std::streambuf* sink_;
    std::size_t limit_, written_ = 0;
    std::streamsize xsputn(const char* text, const std::streamsize count) override {
        if (count < 0 || static_cast<std::uint64_t>(count) > limit_-written_) { throw std::runtime_error("output byte budget exceeded; export not published"); }
        const auto actual = sink_->sputn(text,count);
        if (actual != count) { throw std::runtime_error("output write failed"); }
        written_ += static_cast<std::size_t>(actual); return actual;
    }
    int_type overflow(const int_type c) override {
        if (traits_type::eq_int_type(c, traits_type::eof())) { return traits_type::not_eof(c); }
        if (written_ == limit_) { throw std::runtime_error("output byte budget exceeded; export not published"); }
        const auto actual = sink_->sputc(traits_type::to_char_type(c));
        if (traits_type::eq_int_type(actual, traits_type::eof())) { throw std::runtime_error("output write failed"); }
        ++written_; return actual;
    }
    int sync() override { return sink_->pubsync(); }
};
bool protected_path(const std::filesystem::path& path) {
    for (const auto& part : path) {
        auto s = path_text(part);
        std::transform(s.begin(),s.end(),s.begin(),[](const unsigned char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c-'A'+'a') : static_cast<char>(c); });
        if (s == "historical" || s == ".git") { return true; }
    }
    return false;
}
constexpr auto output_exists_message = "output already exists; choose a new path (existing files are never overwritten)";
bool output_entry_exists(const std::filesystem::path& path) {
#ifdef _WIN32
    // Query the directory entry itself, even for a dangling symlink/junction.
    // Some MinGW symlink_status implementations report its missing target.
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) { return true; }
    const auto error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) { return false; }
    throw std::filesystem::filesystem_error("cannot inspect output destination", path,
        std::error_code(static_cast<int>(error), std::system_category()));
#else
    return std::filesystem::symlink_status(path).type() != std::filesystem::file_type::not_found;
#endif
}
std::filesystem::path resolved_output_parent(const std::filesystem::path& parent) {
#ifdef _WIN32
    // Resolve directory reparse points through a native directory handle. The
    // verified MinGW library's canonical(existing_directory) fails here.
    const auto handle = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::filesystem::filesystem_error("cannot open output parent", parent,
            std::error_code(static_cast<int>(GetLastError()), std::system_category()));
    }
    std::array<wchar_t, 32768> text{};
    const auto length = GetFinalPathNameByHandleW(handle, text.data(), static_cast<DWORD>(text.size()),
        FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    const auto error = GetLastError(); CloseHandle(handle);
    if (length == 0 || length >= text.size()) {
        throw std::filesystem::filesystem_error("cannot resolve output parent", parent,
            std::error_code(static_cast<int>(length == 0 ? error : ERROR_BUFFER_OVERFLOW), std::system_category()));
    }
    return std::filesystem::path(std::wstring(text.data(), length));
#else
    return std::filesystem::canonical(parent);
#endif
}
// Keep the exclusively created file open until serialization finishes. Reopening
// by name would permit a symlink substitution between creation and truncation.
class ExclusiveFileBuffer final : public std::streambuf {
public:
    ExclusiveFileBuffer() { setp(storage_.data(), storage_.data()+storage_.size()); }
    ExclusiveFileBuffer(const ExclusiveFileBuffer&) = delete;
    ExclusiveFileBuffer& operator=(const ExclusiveFileBuffer&) = delete;
    bool open(const std::filesystem::path& path) {
#ifdef _WIN32
        handle_ = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        return handle_ != INVALID_HANDLE_VALUE;
#else
        descriptor_ = ::open(path.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0600);
        return descriptor_ >= 0;
#endif
    }
    ~ExclusiveFileBuffer() override { close_impl(); }
    void close() {
        try { if (pptr() != pbase()) { sync(); } }
        catch (...) { close_impl(); throw; }
        if (!close_impl()) { throw std::runtime_error("cannot finish export file"); }
    }
private:
    std::array<char, 8192> storage_{};
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    bool close_impl() noexcept {
        if (handle_ == INVALID_HANDLE_VALUE) { return true; }
        const auto handle = handle_; handle_ = INVALID_HANDLE_VALUE;
        return CloseHandle(handle) != 0;
    }
#else
    int descriptor_ = -1;
    bool close_impl() noexcept {
        if (descriptor_ < 0) { return true; }
        const auto descriptor = descriptor_; descriptor_ = -1;
        return ::close(descriptor) == 0;
    }
#endif
    std::streamsize write_bytes(const char* text, const std::streamsize count) {
        std::streamsize written = 0;
        while (written < count) {
            const auto chunk = static_cast<std::size_t>(std::min<std::streamsize>(count-written, 1024*1024));
#ifdef _WIN32
            DWORD actual = 0;
            if (!WriteFile(handle_, text+written, static_cast<DWORD>(chunk), &actual, nullptr) || actual == 0) {
                throw std::runtime_error("output write failed");
            }
#else
            const auto actual = ::write(descriptor_, text+written, chunk);
            if (actual < 0 && errno == EINTR) { continue; }
            if (actual <= 0) { throw std::runtime_error("output write failed"); }
#endif
            written += static_cast<std::streamsize>(actual);
        }
        return written;
    }
    int sync() override {
        const auto count = pptr()-pbase();
        write_bytes(pbase(), count);
        setp(storage_.data(), storage_.data()+storage_.size());
        return 0;
    }
    std::streamsize xsputn(const char* text, const std::streamsize count) override {
        std::streamsize written = 0;
        while (written < count) {
            if (pptr() == epptr()) { sync(); }
            const auto chunk = std::min<std::streamsize>(count-written, epptr()-pptr());
            std::copy_n(text+written, chunk, pptr());
            pbump(static_cast<int>(chunk)); written += chunk;
        }
        return written;
    }
    int_type overflow(const int_type c) override {
        if (traits_type::eq_int_type(c, traits_type::eof())) { return traits_type::not_eof(c); }
        sync(); *pptr() = traits_type::to_char_type(c); pbump(1); return c;
    }
};
} // namespace
void write_bounded(std::ostream& sink, const std::size_t max_bytes, const std::function<void(std::ostream&)>& writer) {
    LimitedBuffer buffer(sink.rdbuf(),max_bytes);
    std::ostream out(&buffer); out.exceptions(std::ios::badbit|std::ios::failbit);
    writer(out); out.flush(); if (!sink) { throw std::runtime_error("output stream failed"); }
}
void write_export(const std::filesystem::path& path, const std::size_t max_bytes, const std::function<void(std::ostream&)>& writer) {
    if (path.empty() || path.filename().empty() || protected_path(path)) { throw std::invalid_argument("output must be a new file outside historical materials and .git"); }
    // Resolve parent aliases, but never follow an existing final-component link.
    const auto absolute = std::filesystem::absolute(path);
    const auto target = resolved_output_parent(absolute.parent_path()) / absolute.filename();
    if (protected_path(target)) { throw std::invalid_argument("resolved output is inside protected historical materials or .git"); }
    if (output_entry_exists(target)) { throw std::invalid_argument(output_exists_message); }
    if (!std::filesystem::is_directory(target.parent_path())) { throw std::invalid_argument("output parent directory does not exist"); }
    std::filesystem::path temporary;
    ExclusiveFileBuffer buffer;
    std::random_device random;
    for (int i = 0; i < 16; ++i) {
        auto candidate = target.parent_path() / (".tradeup-export-"+std::to_string(random())+"-"+std::to_string(random())+".tmp");
        if (buffer.open(candidate)) { temporary = std::move(candidate); break; }
    }
    if (temporary.empty()) { throw std::runtime_error("cannot create temporary export file in output directory"); }
    try {
        std::ostream file(&buffer);
        file.exceptions(std::ios::badbit|std::ios::failbit);
        write_bounded(file,max_bytes,writer); buffer.close();
        // create_hard_link fails if the destination exists: no overwrite race.
        std::error_code error;
        std::filesystem::create_hard_link(temporary,target,error);
        if (error) {
            if (error == std::errc::file_exists || output_entry_exists(target)) {
                throw std::filesystem::filesystem_error(output_exists_message, temporary, target, error);
            }
            throw std::filesystem::filesystem_error("cannot create hard link", temporary, target, error);
        }
        std::filesystem::remove(temporary);
    } catch (...) {
        try { buffer.close(); } catch (...) { /* Preserve the original diagnostic. */ }
        std::error_code ignored; std::filesystem::remove(temporary,ignored); throw;
    }
}
} // namespace tradeup
