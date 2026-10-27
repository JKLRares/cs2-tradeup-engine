#include "tradeup/application.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
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
        const auto value = traits_type::to_char_type(c); xsputn(&value,1); return c;
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
bool create_exclusive(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) { return false; }
    CloseHandle(handle); return true;
#else
    const auto fd = ::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
    if (fd < 0) { return false; } ::close(fd); return true;
#endif
}
} // namespace
void write_bounded(std::ostream& sink, const std::size_t max_bytes, const std::function<void(std::ostream&)>& writer) {
    LimitedBuffer buffer(sink.rdbuf(),max_bytes);
    std::ostream out(&buffer); out.exceptions(std::ios::badbit|std::ios::failbit);
    writer(out); out.flush(); if (!sink) { throw std::runtime_error("output stream failed"); }
}
void write_export(const std::filesystem::path& path, const std::size_t max_bytes, const std::function<void(std::ostream&)>& writer) {
    if (path.empty() || path.filename().empty() || protected_path(path)) { throw std::invalid_argument("output must be a new file outside historical materials and .git"); }
    const auto target = std::filesystem::weakly_canonical(std::filesystem::absolute(path));
    if (protected_path(target)) { throw std::invalid_argument("resolved output is inside protected historical materials or .git"); }
    if (std::filesystem::exists(target)) { throw std::invalid_argument("output already exists; choose a new path (existing files are never overwritten)"); }
    if (!std::filesystem::is_directory(target.parent_path())) { throw std::invalid_argument("output parent directory does not exist"); }
    std::filesystem::path temporary;
    std::random_device random;
    for (int i = 0; i < 16; ++i) {
        auto candidate = target.parent_path() / (".tradeup-export-"+std::to_string(random())+"-"+std::to_string(random())+".tmp");
        if (create_exclusive(candidate)) { temporary = std::move(candidate); break; }
    }
    if (temporary.empty()) { throw std::runtime_error("cannot create temporary export file in output directory"); }
    try {
        std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
        if (!file) { throw std::runtime_error("cannot open temporary export file"); }
        write_bounded(file,max_bytes,writer); file.close();
        if (!file) { throw std::runtime_error("cannot finish export file"); }
        // create_hard_link fails if the destination exists: no overwrite race.
        std::filesystem::create_hard_link(temporary,target);
        std::filesystem::remove(temporary);
    } catch (...) {
        std::error_code ignored; std::filesystem::remove(temporary,ignored); throw;
    }
}
} // namespace tradeup
