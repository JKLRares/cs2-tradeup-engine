#include "tradeup/catalog_io.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>

namespace tradeup {
std::filesystem::path utf8_path(const std::string_view text) {
    std::u8string converted;
    for(const unsigned char c:text){converted+=static_cast<char8_t>(c);}
    return std::filesystem::path(converted);
}
std::string path_text(const std::filesystem::path& path) {
    const auto text = path.u8string(); return {text.begin(), text.end()};
}
std::string read_local_file(const std::filesystem::path& path, const std::size_t max_bytes) {
    if (!std::filesystem::is_regular_file(path)) { throw std::invalid_argument("input is not a regular file: " + path_text(path)); }
    if (std::filesystem::file_size(path) > max_bytes) { throw std::invalid_argument("input exceeds byte limit: " + path_text(path)); }
    std::ifstream input(path, std::ios::binary);
    if (!input) { throw std::runtime_error("cannot open input: " + path_text(path)); }
    std::string result;
    std::array<char, 8192> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto size = static_cast<std::size_t>(input.gcount());
        if (size > max_bytes - result.size()) { throw std::invalid_argument("input grew beyond byte limit"); }
        result.append(buffer.data(), size);
    }
    if (!input.eof()) { throw std::runtime_error("input read failed: " + path_text(path)); }
    // Reject controls that would make records/diagnostics ambiguous. UTF-8 labels are retained.
    for (const unsigned char c : result) {
        if ((c < 32 && c != '\n' && c != '\r' && c != '\t') || c == 127) { throw std::invalid_argument("input contains unsupported control bytes"); }
    }
    for (std::size_t i=0;i<result.size();) {
        const auto c=static_cast<unsigned char>(result[i++]);
        if(c<128){continue;}
        const int continuation=c>=0xC2 && c<=0xDF ? 1 : (c>=0xE0 && c<=0xEF ? 2 : (c>=0xF0 && c<=0xF4 ? 3 : -1));
        if(continuation<0 || static_cast<std::size_t>(continuation)>result.size()-i){throw std::invalid_argument("input is not valid UTF-8");}
        std::uint32_t code=c & (continuation==1 ? 0x1FU : (continuation==2 ? 0x0FU : 0x07U));
        for(int j=0;j<continuation;++j){const auto next=static_cast<unsigned char>(result[i++]);if((next&0xC0U)!=0x80U){throw std::invalid_argument("input is not valid UTF-8");}code=(code<<6)|(next&0x3FU);}
        if((continuation==2 && code<0x800U)||(continuation==3 && code<0x10000U)||code>0x10FFFFU||(code>=0xD800U&&code<=0xDFFFU)){throw std::invalid_argument("input is not valid UTF-8");}
    }
    return result;
}
std::vector<std::string> split_fields(const std::string_view line, const char separator) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    for (;;) {
        const auto end = line.find(separator, begin);
        result.emplace_back(line.substr(begin, end == std::string_view::npos ? end : end-begin));
        if (end == std::string_view::npos) { break; }
        begin = end+1;
    }
    return result;
}
std::string_view quote_kind_name(const QuoteKind kind) {
    switch (kind) {
    case QuoteKind::recorded: return "recorded";
    case QuoteKind::minimum_derived: return "minimum_derived";
    case QuoteKind::composite_estimate: return "composite_estimate";
    case QuoteKind::ratio_estimate: return "ratio_estimate";
    case QuoteKind::synthetic: return "synthetic";
    case QuoteKind::unknown: return "unknown";
    }
    throw std::invalid_argument("unsupported quote kind");
}
QuoteKind parse_quote_kind(const std::string_view kind) {
    for (const auto value : {QuoteKind::recorded, QuoteKind::minimum_derived, QuoteKind::composite_estimate,
                           QuoteKind::ratio_estimate, QuoteKind::synthetic, QuoteKind::unknown}) {
        if (quote_kind_name(value) == kind) { return value; }
    }
    throw std::invalid_argument("unknown quote kind: " + std::string(kind));
}
void finalize_identity(LoadedCatalog& catalog, const std::string_view source_binding) {
    catalog.source_binding = source_binding;
    catalog.dataset.snapshot_id = "pending";
    const ReferenceEngine validated(catalog.dataset);
    catalog.dataset = validated.dataset();
    std::sort(catalog.dataset.limitations.begin(), catalog.dataset.limitations.end());
    std::string canonical = "tradeup-dataset-canonical-v1\n";
    const auto field = [&](const std::string_view s) { canonical += std::to_string(s.size()) + ":" + std::string(s); };
    field(catalog.source_format); field(catalog.mapping_version); field(source_binding);
    field(catalog.dataset.scenario_id); field(catalog.scenario_label);
    field("historical-quote-tickets-v1"); field("historical-proportional-13-percent-v1");
    field(catalog.dataset.output_valuation_policy_id); field(catalog.dataset.price_unit);
    field(std::to_string(catalog.dataset.collections.size()));
    const auto quote = [&](const Quote& q) { field(q.value.str()); field(quote_kind_name(q.kind)); field(q.source); };
    for (const auto& c : catalog.dataset.collections) {
        field(c.id.value); field(catalog.collection_labels.at(c.id.value)); field(std::to_string(c.outcomes.size()));
        for (const auto& o : c.outcomes) { field(o.id.value); quote(*o.price); }
    }
    field(std::to_string(catalog.dataset.buckets.size()));
    for (const auto& b : catalog.dataset.buckets) {
        field(b.id.value); field(catalog.bucket_labels.at(b.id.value)); field(b.collection.value);
        field(b.wear == WearCategory::lower ? "L" : "H"); quote(b.unit_cost);
    }
    field(std::to_string(catalog.dataset.limitations.size()));
    for (const auto& s : catalog.dataset.limitations) { field(s); }
    catalog.content_sha256 = sha256(canonical);
    catalog.dataset.snapshot_id = sha256("tradeup-snapshot-v1\n" + canonical);
}
LoadedCatalog load_catalog_v1(const std::filesystem::path& path) {
    const auto bytes = read_local_file(path);
    LoadedCatalog result;
    result.source_path = path_text(path); result.source_format = "tradeup-catalog-v1";
    result.source_sha256 = sha256(bytes); result.mapping_version = "explicit-catalog-v1";
    std::istringstream input(bytes);
    std::string line;
    if (!std::getline(input, line)) { throw std::invalid_argument("empty catalog"); }
    if (!line.empty() && line.back() == '\r') { line.pop_back(); }
    if (line != "tradeup-catalog-v1") { throw std::invalid_argument("catalog requires tradeup-catalog-v1 header"); }
    std::map<std::string, std::vector<Outcome>> outcomes;
    bool scenario = false, valuation = false, unit = false;
    std::size_t row = 1, outcome_count = 0;
    while (std::getline(input, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        if (line.empty() || line.front() == '#') { continue; }
        try {
            if (line.size() > 16 * 1024) { throw std::invalid_argument("record exceeds 16 KiB"); }
            const auto f = split_fields(line);
            const auto require = [&](const std::size_t n) { if (f.size() != n) { throw std::invalid_argument("wrong field count for " + f[0]); } };
            if (f[0] == "scenario") {
                require(3); if (scenario) { throw std::invalid_argument("duplicate scenario"); } scenario = true;
                result.dataset.scenario_id = f[1]; result.scenario_label = f[2];
            } else if (f[0] == "valuation") {
                require(2); if (valuation) { throw std::invalid_argument("duplicate valuation"); } valuation = true;
                result.dataset.output_valuation_policy_id = f[1];
            } else if (f[0] == "unit") {
                require(2); if (unit) { throw std::invalid_argument("duplicate unit"); } unit = true; result.dataset.price_unit = f[1];
            } else if (f[0] == "collection") {
                require(3); if (!result.collection_labels.emplace(f[1], f[2]).second) { throw std::invalid_argument("duplicate collection"); }
                if (result.collection_labels.size() > 4096) { throw std::invalid_argument("collection count exceeds 4096"); }
                result.dataset.collections.push_back({CollectionId{f[1]}, {}, true});
            } else if (f[0] == "outcome") {
                require(6); if (++outcome_count > 100'000) { throw std::invalid_argument("outcome count exceeds 100000"); }
                outcomes[f[1]].push_back({OutcomeId{f[2]}, Quote{Rational::parse(f[3]), parse_quote_kind(f[4]), f[5]}});
            } else if (f[0] == "bucket") {
                require(9); if (f[3] != "L" && f[3] != "H") { throw std::invalid_argument("wear must be L or H"); }
                if (!result.bucket_labels.emplace(f[1], f[8]).second) { throw std::invalid_argument("duplicate bucket"); }
                if (result.bucket_labels.size() > 4096) { throw std::invalid_argument("bucket count exceeds 4096"); }
                result.dataset.buckets.push_back({BucketId{f[1]}, CollectionId{f[2]}, f[3] == "L" ? WearCategory::lower : WearCategory::higher,
                    Quote{Rational::parse(f[4]), parse_quote_kind(f[5]), f[6]}});
                if (f[7] != "representative") { throw std::invalid_argument("bucket semantics must explicitly be representative"); }
            } else if (f[0] == "limitation") { require(2); result.dataset.limitations.push_back(f[1]); }
            else { throw std::invalid_argument("unknown catalog record: " + f[0]); }
        } catch (const std::exception& e) { throw std::invalid_argument("catalog line " + std::to_string(row) + ": " + e.what()); }
    }
    if (!scenario || !valuation || !unit || result.scenario_label.empty()) { throw std::invalid_argument("catalog requires scenario, valuation, and unit records"); }
    for (auto& c : result.dataset.collections) { c.outcomes = std::move(outcomes[c.id.value]); outcomes.erase(c.id.value); }
    if (!outcomes.empty()) { throw std::invalid_argument("outcome refers to undeclared collection"); }
    result.dataset.limitations.push_back("Local declared catalog; quote time, exact floats and listing supply are not verified.");
    finalize_identity(result); return result;
}
} // namespace tradeup
