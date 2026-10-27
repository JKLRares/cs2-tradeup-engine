#pragma once

#include "tradeup/catalog_io.hpp"

#include <array>

namespace tradeup {
struct HistoricalScenario {
    std::string_view id, label, lower_wear, higher_wear;
    int cost_row;
    std::array<int, 5> output_rows;
};
[[nodiscard]] std::span<const HistoricalScenario> historical_scenarios();
[[nodiscard]] const HistoricalScenario& historical_scenario(std::string_view id_or_label);
[[nodiscard]] LoadedCatalog load_historical_2024(const std::filesystem::path& path,
                                               std::string_view scenario = "st-ft-mw-mov");
} // namespace tradeup
