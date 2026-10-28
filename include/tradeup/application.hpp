#pragma once

#include "tradeup/catalog_io.hpp"
#include "tradeup/optimized.hpp"

#include <functional>
#include <ostream>

namespace tradeup {
enum class Ranking { roi, profit };
struct SearchQuery {
    SearchProfile profile = SearchProfile::historical_5_5;
    Rational threshold{115};
    std::optional<Rational> max_cost;
    Ranking ranking = Ranking::roi;
    OptimizedLimits limits{100'000, 500'000, 10'000, 16*1024*1024, std::chrono::milliseconds(2000)};
    OptimizedOptions options;
    std::size_t top = 20;
};
[[nodiscard]] std::vector<Allocation> parse_allocations(std::string_view text);
[[nodiscard]] std::vector<std::size_t> select_results(const OptimizedEngine& engine,
    const OptimizedResult& result, const SearchQuery& query);
[[nodiscard]] std::string decimal_display(Rational value, std::size_t places = 6);
// New-file-only, bounded export, published atomically by a hard link in the same directory.
void write_export(const std::filesystem::path& path, std::size_t max_bytes,
                  const std::function<void(std::ostream&)>& writer);
void write_bounded(std::ostream& sink, std::size_t max_bytes, const std::function<void(std::ostream&)>& writer);
} // namespace tradeup
