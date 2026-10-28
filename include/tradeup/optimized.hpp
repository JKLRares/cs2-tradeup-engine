#pragma once

#include "tradeup/reference.hpp"

#include <array>
#include <chrono>

namespace tradeup {

struct CatalogLimits {
    std::size_t max_buckets = 4096;
    std::size_t max_collections = 4096;
    std::size_t max_outcomes = 100'000;
};
struct OptimizedLimits {
    std::uint64_t max_evaluations = 1'000'000;
    std::uint64_t max_work = 5'000'000;
    std::size_t max_results = 10'000;
    std::size_t max_result_bytes = 16 * 1024 * 1024;
    std::chrono::milliseconds max_time{5000};
};
struct OptimizedOptions {
    bool pruning = true;
    bool scaled_arithmetic = true;
};
enum class Completion {
    complete, evaluation_budget, work_budget, time_budget, result_budget, arithmetic_limit
};
[[nodiscard]] std::string_view completion_name(Completion value);
struct IndexedAllocation {
    std::size_t bucket = 0;
    int quantity = 0;
};
// Indices belong to this engine's immutable sorted catalog, not a persistent format.
struct OptimizedHit {
    std::array<IndexedAllocation, 5> inputs{};
    std::size_t active = 0;
    Rational cost, gross_ev, net_ev, expected_profit, return_percent, roi_percent, worst_net;
};
struct OptimizedStatistics {
    std::uint64_t domain_contracts = 0;
    bool domain_count_available = false;
    std::uint64_t candidates_considered = 0;
    std::uint64_t candidates_evaluated = 0;
    std::uint64_t candidates_pruned = 0;
    std::uint64_t qualifying_found = 0;
    std::uint64_t work = 0;
    std::uint64_t supports = 0;
    std::uint64_t unavailable_bounds = 0;
    std::uint64_t integer_decisions = 0;
    std::size_t result_capacity_bytes = 0;
    double elapsed_seconds = 0;
};
struct OptimizedResult {
    Completion completion = Completion::complete;
    std::string detail;
    OptimizedStatistics statistics;
    std::vector<OptimizedHit> qualifying;
    [[nodiscard]] bool complete() const noexcept { return completion == Completion::complete; }
};

class OptimizedEngine {
public:
    explicit OptimizedEngine(Dataset dataset, CatalogLimits limits = {});
    [[nodiscard]] const Dataset& dataset() const noexcept { return validated_.dataset(); }
    [[nodiscard]] bool has_scaled_arithmetic() const noexcept { return scaled_; }
    [[nodiscard]] std::uint64_t domain_size(SearchProfile profile) const;
    // nullopt is an internal verification mode retaining every evaluated contract, even zero EV.
    [[nodiscard]] OptimizedResult search(SearchProfile profile = SearchProfile::historical_5_5,
        std::optional<Rational> threshold = Rational(115), OptimizedLimits limits = {},
        OptimizedOptions options = {}) const;
    [[nodiscard]] Contract contract(const OptimizedHit& hit) const;
    [[nodiscard]] Evaluation evaluate(const Contract& value) const;
private:
    struct CollectionSummary {
        Rational sum, mean, worst;
        std::int64_t count = 0;
        std::vector<Rational> receipts;
    };
    struct BucketSummary {
        std::size_t collection = 0;
        std::int64_t cost_units = 0, sum_units = 0;
    };
    ReferenceEngine validated_; // Shared input validation/canonicalization only; never search/evaluate.
    std::vector<CollectionSummary> collections_;
    std::vector<BucketSummary> buckets_;
    std::array<std::vector<std::size_t>, 2> sides_;
    bool scaled_ = false;
    std::int64_t cost_scale_ = 1, outcome_scale_ = 1;
    [[nodiscard]] OptimizedHit values(const std::array<IndexedAllocation, 5>& inputs,
                                      std::size_t active, Rational cost, Rational gross) const;
};
} // namespace tradeup
