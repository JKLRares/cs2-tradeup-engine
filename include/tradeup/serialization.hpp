#pragma once

#include "tradeup/application.hpp"

namespace tradeup {
struct ReportInfo {
    const LoadedCatalog& catalog;
    std::string_view operation;
    const SearchQuery* query = nullptr;
    const OptimizedResult* search = nullptr;
    std::size_t matched = 1, human_shown = 1;
    bool outcomes = false;
};
void write_scenarios_json(std::ostream& out);
void write_inspect_json(std::ostream& out, const LoadedCatalog& catalog, bool matrix);
void write_report_json(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine,
                       std::span<const std::size_t> order, const Evaluation* single = nullptr);
void write_report_csv(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine,
                      std::span<const std::size_t> order, const Evaluation* single = nullptr);
void print_inspect(std::ostream& out, const LoadedCatalog& catalog);
void print_evaluation(std::ostream& out, const LoadedCatalog& catalog, const Evaluation& value, bool outcomes);
void print_search(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine, std::span<const std::size_t> order);
} // namespace tradeup
