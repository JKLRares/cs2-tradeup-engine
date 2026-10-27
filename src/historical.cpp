#include "tradeup/historical.hpp"

#include <iomanip>
#include <sstream>

namespace tradeup {
namespace {
constexpr std::array<HistoricalScenario, 12> scenarios{{
    {"normal-ft-mw-mov", "NORMAL FT MW mov", "FT", "MW", 67, {74,75,76,77,78}},
    {"normal-ft-mw-roz", "NORMAL FT MW roz", "FT", "MW", 69, {71,72,73,107,108}},
    {"st-ft-mw-mov", "StatTrak FT MW mov", "FT", "MW", 68, {82,83,84,85,86}},
    {"st-ft-mw-roz", "StatTrak FT MW roz", "FT", "MW", 70, {79,80,81,107,108}},
    {"normal-mw-fn-mov", "NORMAL MW FN mov", "MW", "FN", 103, {90,91,92,93,94}},
    {"normal-mw-fn-roz", "NORMAL MW FN roz", "MW", "FN", 105, {87,88,89,107,108}},
    {"st-mw-fn-mov", "StatTrak MW FN mov", "MW", "FN", 104, {98,99,100,101,102}},
    {"st-mw-fn-roz", "StatTrak MW FN roz", "MW", "FN", 106, {95,96,97,107,108}},
    {"normal-ft-mw-rosu", "NORMAL FT MW rosu", "FT", "MW", 111, {109,110,121,107,108}},
    {"st-ft-mw-rosu", "StatTrak FT MW rosu", "FT", "MW", 114, {112,113,121,107,108}},
    {"normal-mw-fn-rosu", "NORMAL MW FN rosu", "MW", "FN", 117, {115,116,121,107,108}},
    {"st-mw-fn-rosu", "StatTrak MW FN rosu", "MW", "FN", 120, {118,119,121,107,108}}
}};
constexpr std::array<std::string_view, 29> names{
    "BREAKOUT", "CS20", "CHROMA", "CHROMA 2", "CHROMA 3", "CLUTCH", "DANGER ZONE",
    "DREAMS&NIGHTMARES", "FALCHION", "FRACTURE", "GAMMA", "GAMMA 2", "GLOVE", "HORIZON",
    "BROKEN FANG", "HYDRA", "RIPTIDE", "PRISMA", "PRISMA 2", "RECOIL", "REVOLUTION",
    "REVOLVER", "SHADOW", "SHATTERED WEB", "SNAKEBITE", "SPECTRUM", "SPECTRUM 2", "WILDFIRE", "WINTER OFFENSIVE"};
struct HistoricalCell {
    std::string_view address, original;
    QuoteKind kind;
    bool padding;
    std::string_view lineage;
};
#include "historical_cells.inc"

std::string numbered(const char prefix, const std::size_t n, const int width) {
    std::ostringstream text; text << prefix << std::setw(width) << std::setfill('0') << n; return text.str();
}
std::size_t index(const int workbook_row, const std::size_t column) {
    return static_cast<std::size_t>(workbook_row - 67)*58 + column;
}
} // namespace
std::span<const HistoricalScenario> historical_scenarios() { return scenarios; }
const HistoricalScenario& historical_scenario(const std::string_view id_or_label) {
    for (const auto& scenario : scenarios) { if (scenario.id == id_or_label || scenario.label == id_or_label) { return scenario; } }
    throw std::invalid_argument("unknown historical scenario: " + std::string(id_or_label) + "; use scenarios");
}
LoadedCatalog load_historical_2024(const std::filesystem::path& path, const std::string_view scenario_id) {
    const auto& scenario = historical_scenario(scenario_id);
    const auto bytes = read_local_file(path, 1024*1024);
    LoadedCatalog result;
    result.source_path = path_text(path); result.source_format = "historical-2024-matrix-v1";
    result.mapping_version = "historical-2024-source-workbook-mapping-v1"; result.source_sha256 = sha256(bytes);
    result.scenario_label = scenario.label;
    std::istringstream input(bytes); std::string line;
    if (!std::getline(input, line)) { throw std::invalid_argument("historical matrix is empty"); }
    if (!line.empty() && line.back() == '\r') { line.pop_back(); }
    std::istringstream header(line); std::string n, extra;
    if (!(header >> n) || n != "58" || (header >> extra)) { throw std::invalid_argument("matrix header must contain exactly 58"); }
    for (int row = 0; row < 55; ++row) {
        if (!std::getline(input, line)) { throw std::invalid_argument("matrix truncated before data row " + std::to_string(row+1)); }
        std::istringstream values(line); std::string token;
        for (std::size_t c = 0; c < 58; ++c) {
            const auto location = "matrix data row " + std::to_string(row+1) + ", column " + std::to_string(c+1);
            if (!(values >> token)) { throw std::invalid_argument(location + ": missing price (58 prices required per row)"); }
            try {
                if (token.empty() || token.find_first_not_of("0123456789.") != std::string::npos) {
                    throw std::invalid_argument("expected nonnegative decimal price");
                }
                result.matrix.push_back(Rational::parse(token));
            } catch (const std::exception& e) { throw std::invalid_argument(location + ": " + e.what()); }
        }
        if (values >> token) { throw std::invalid_argument("matrix data row " + std::to_string(row+1) + ": more than 58 prices"); }
    }
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") != std::string::npos) { throw std::invalid_argument("matrix has extra data after 55 rows"); }
    }
    result.authentic_preserved_values = true;
    std::string binding = "historical-matrix-values-v1\n58\n55\n", ledger = "historical-cell-evidence-v1\n";
    for (std::size_t i = 0; i < result.matrix.size(); ++i) {
        const auto& e = historical_cells[i]; const auto v = result.matrix[i];
        if (e.padding && v != Rational(0)) { throw std::invalid_argument("nonzero price in declared structural padding at Sheet1!" + std::string(e.address)); }
        if (v != Rational::parse(e.original)) { result.authentic_preserved_values = false; }
        binding += v.str() + '\n';
        ledger += std::string(e.address) + '\t' + std::string(e.original) + '\t' + std::string(quote_kind_name(e.kind)) + '\t'
            + (e.padding ? "padding" : "modeled") + '\t' + std::string(e.lineage) + '\n';
    }
    for (const auto& s : scenarios) {
        for (std::size_t c = 0; c < 58; ++c) {
            if (result.matrix[index(s.cost_row,c)] <= Rational(0)) {
                throw std::invalid_argument("nonpositive input cost at workbook row " + std::to_string(s.cost_row) + ", bucket " + std::to_string(c+1));
            }
        }
    }
    result.dataset.scenario_id = scenario.id;
    result.dataset.price_unit = "unknown-historical-price-unit";
    result.dataset.output_valuation_policy_id = "historical-higher-wear-quote-basket-v1";
    result.dataset.limitations = {
        "Historical declared quote basket; current game mechanics have not been verified.",
        "Currency and actual historical quote timestamps are unknown; archive dates are not quote dates.",
        "Exact input floats, output wear feasibility and purchasable listing quantities are unknown.",
        "Expected ROI is not guaranteed profit; the 13 percent proportional deduction is a historical model.",
        "Spreadsheet minima, composite averages and ratio estimates are retained where present; values are exported cent-rounded quotes.",
        "Outcome identities denote modeled spreadsheet slots, including composite slots, not a complete independently verified item catalog."};
    if (!result.authentic_preserved_values) { result.dataset.limitations.push_back("Edited matrix values: original slot mapping retained, but changed-cell price provenance is unknown."); }
    const auto quote = [&](const int row, const std::size_t c) {
        const auto i = index(row,c); const auto& e = historical_cells[i]; const auto value = result.matrix[i];
        std::string source = "matrix row " + std::to_string(row-66) + " column " + std::to_string(c+1) + "; Sheet1!" + std::string(e.address)
            + "; " + std::string(e.lineage) + "; exported cent-rounded value";
        auto kind = e.kind;
        if (value != Rational::parse(e.original)) { kind = QuoteKind::unknown; source += "; replacement differs from preserved value " + std::string(e.original); }
        return Quote{value, kind, std::move(source)};
    };
    for (std::size_t c = 0; c < names.size(); ++c) {
        const auto cid = numbered('c', c+1, 2);
        Collection collection{CollectionId{cid}, {}, true}; result.collection_labels[cid] = names[c];
        for (const int row : scenario.output_rows) {
            const auto column = 2*c+1; // Preserved odd buckets use the following even bucket's output valuation.
            if (!historical_cells[index(row,column)].padding) {
                collection.outcomes.push_back({OutcomeId{"row" + std::to_string(row)}, quote(row,column)});
            }
        }
        result.dataset.collections.push_back(std::move(collection));
        for (std::size_t wear = 0; wear < 2; ++wear) {
            const auto bid = numbered('b', 2*c+wear+1, 3);
            result.dataset.buckets.push_back({BucketId{bid}, CollectionId{cid}, wear == 0 ? WearCategory::lower : WearCategory::higher,
                quote(scenario.cost_row,2*c+wear)});
            result.bucket_labels[bid] = std::string(names[c]) + (wear == 0 ? " lower " : " higher ")
                + std::string(wear == 0 ? scenario.lower_wear : scenario.higher_wear);
        }
    }
    finalize_identity(result, sha256(binding) + ":" + sha256(ledger)); return result;
}
} // namespace tradeup
