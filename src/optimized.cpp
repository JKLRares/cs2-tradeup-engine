#include "tradeup/optimized.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <utility>

namespace tradeup {
namespace {
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
std::int64_t mul(const std::int64_t a, const std::int64_t b) {
    if (a < 0 || b < 0 || (b != 0 && a > maximum / b)) {
        throw std::overflow_error("optimized nonnegative product exceeds signed-64-bit bounds");
    }
    return a * b;
}
std::int64_t add(const std::int64_t a, const std::int64_t b) {
    if (a < 0 || b < 0 || a > maximum - b) {
        throw std::overflow_error("optimized nonnegative sum exceeds signed-64-bit bounds");
    }
    return a + b;
}
std::uint64_t count_add(const std::uint64_t a, const std::uint64_t b) {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) {
        throw std::overflow_error("search domain count exceeds uint64");
    }
    return a + b;
}
std::uint64_t count_mul(const std::uint64_t a, const std::uint64_t b) {
    if (b && a > std::numeric_limits<std::uint64_t>::max() / b) {
        throw std::overflow_error("search domain count exceeds uint64");
    }
    return a * b;
}
std::uint64_t choose(const std::size_t n, const std::size_t k) {
    if (k > n) { return 0; }
    std::uint64_t result = 1;
    for (std::size_t i = 1; i <= std::min(k, n - k); ++i) {
        auto a = static_cast<std::uint64_t>(n - i + 1);
        auto b = static_cast<std::uint64_t>(i);
        const auto g = std::gcd(a, b); a /= g; b /= g;
        const auto h = std::gcd(result, b); result /= h; b /= h;
        if (b != 1) { throw std::logic_error("nonintegral binomial step"); }
        result = count_mul(result, a);
    }
    return result;
}
std::pair<int, int> split(const SearchProfile profile) {
    (void)profile_id(profile);
    return profile == SearchProfile::historical_5_5 ? std::pair{5, 5} : std::pair{6, 4};
}
Dataset checked_catalog(Dataset data, const CatalogLimits limits) {
    if (!limits.max_buckets || !limits.max_collections || !limits.max_outcomes ||
        data.buckets.size() > limits.max_buckets || data.collections.size() > limits.max_collections) {
        throw std::invalid_argument("optimized catalog budget exceeded or zero");
    }
    std::size_t outcomes = 0;
    for (const auto& c : data.collections) {
        if (c.outcomes.size() > limits.max_outcomes - outcomes) {
            throw std::invalid_argument("optimized outcome catalog budget exceeded");
        }
        outcomes += c.outcomes.size();
    }
    return data;
}
void limitation(std::vector<std::string>& output, const Quote& quote, const std::string& location) {
    if (quote.kind == QuoteKind::composite_estimate || quote.kind == QuoteKind::ratio_estimate) {
        output.push_back("Estimated/composite valuation at " + location + ": " + quote.source);
    } else if (quote.kind == QuoteKind::unknown) {
        output.push_back("Unknown quote provenance at " + location);
    } else if (quote.kind == QuoteKind::synthetic) {
        output.push_back("Synthetic test quote at " + location);
    }
}
struct Stopped { Completion reason; };
} // namespace

std::string_view completion_name(const Completion value) {
    switch (value) {
    case Completion::complete: return "complete";
    case Completion::evaluation_budget: return "evaluation-budget";
    case Completion::work_budget: return "work-budget";
    case Completion::time_budget: return "time-budget";
    case Completion::result_budget: return "result-budget";
    case Completion::arithmetic_limit: return "arithmetic-limit";
    }
    throw std::invalid_argument("invalid completion status");
}

OptimizedEngine::OptimizedEngine(Dataset data, const CatalogLimits limits)
    : validated_(checked_catalog(std::move(data), limits)) {
    for (const auto& c : dataset().collections) {
        CollectionSummary summary;
        if (c.outcomes.size() > static_cast<std::uint64_t>(maximum)) {
            throw std::overflow_error("unsupported outcome count");
        }
        summary.count = static_cast<std::int64_t>(c.outcomes.size());
        for (const auto& o : c.outcomes) {
            summary.sum = summary.sum + o.price->value;
            const auto net = o.price->value * Rational(87, 100);
            if (summary.receipts.empty() || net < summary.worst) { summary.worst = net; }
            summary.receipts.push_back(net);
        }
        summary.mean = summary.sum / Rational(summary.count);
        collections_.push_back(std::move(summary));
    }
    for (std::size_t i = 0; i < dataset().buckets.size(); ++i) {
        const auto& b = dataset().buckets[i];
        const auto found = std::lower_bound(dataset().collections.begin(), dataset().collections.end(), b.collection,
            [](const Collection& c, const CollectionId& id) { return c.id < id; });
        buckets_.push_back({static_cast<std::size_t>(found - dataset().collections.begin()), 0, 0});
        sides_[b.wear == WearCategory::lower ? 0U : 1U].push_back(i);
    }
    // Common scales are an optional acceleration. Unrepresentable LCMs use Rational instead.
    try {
        for (const auto& b : dataset().buckets) {
            const auto d = b.unit_cost.value.denominator();
            cost_scale_ = mul(cost_scale_ / std::gcd(cost_scale_, d), d);
        }
        for (const auto& c : collections_) {
            const auto d = c.sum.denominator();
            outcome_scale_ = mul(outcome_scale_ / std::gcd(outcome_scale_, d), d);
        }
        for (std::size_t i = 0; i < buckets_.size(); ++i) {
            const auto cost = dataset().buckets[i].unit_cost.value;
            const auto sum = collections_[buckets_[i].collection].sum;
            buckets_[i].cost_units = mul(cost.numerator(), cost_scale_ / cost.denominator());
            buckets_[i].sum_units = mul(sum.numerator(), outcome_scale_ / sum.denominator());
        }
        scaled_ = true;
    } catch (const std::overflow_error&) { scaled_ = false; }
}

std::uint64_t OptimizedEngine::domain_size(const SearchProfile profile) const {
    const auto [lower, higher] = split(profile);
    std::uint64_t total = 0;
    for (std::size_t a = 1; a <= 4; ++a) {
        for (std::size_t b = 1; a + b <= 5; ++b) {
            auto n = count_mul(choose(sides_[0].size(), a), choose(sides_[1].size(), b));
            n = count_mul(n, choose(static_cast<std::size_t>(lower - 1), a - 1));
            n = count_mul(n, choose(static_cast<std::size_t>(higher - 1), b - 1));
            total = count_add(total, n);
        }
    }
    return total;
}

OptimizedHit OptimizedEngine::values(const std::array<IndexedAllocation, 5>& inputs,
    const std::size_t active, const Rational cost, const Rational gross) const {
    OptimizedHit hit;
    hit.inputs = inputs; hit.active = active; hit.cost = cost; hit.gross_ev = gross;
    hit.net_ev = gross * Rational(87, 100);
    hit.expected_profit = hit.net_ev - cost;
    hit.return_percent = (hit.net_ev / cost) * Rational(100);
    hit.roi_percent = hit.return_percent - Rational(100);
    for (std::size_t i = 0; i < active; ++i) {
        const auto worst = collections_[buckets_[inputs[i].bucket].collection].worst;
        if (i == 0 || worst < hit.worst_net) { hit.worst_net = worst; }
    }
    return hit;
}

OptimizedResult OptimizedEngine::search(const SearchProfile profile, const std::optional<Rational> threshold,
    const OptimizedLimits limits, const OptimizedOptions options) const {
    const auto [lower, higher] = split(profile);
    if ((threshold && *threshold < Rational(0)) || !limits.max_evaluations || !limits.max_work ||
        !limits.max_results || limits.max_result_bytes < sizeof(OptimizedHit) || limits.max_time.count() <= 0 ||
        limits.max_time > std::chrono::hours(24)) {
        throw std::invalid_argument("nonnegative threshold and positive usable search budgets required");
    }
    OptimizedResult result;
    auto& stats = result.statistics;
    const auto start = std::chrono::steady_clock::now();
    const auto tick = [&] {
        if (stats.work >= limits.max_work) { throw Stopped{Completion::work_budget}; }
        if (std::chrono::steady_clock::now() - start >= limits.max_time) { throw Stopped{Completion::time_budget}; }
        ++stats.work;
    };
    const auto capacity_limit = std::min(limits.max_results, limits.max_result_bytes / sizeof(OptimizedHit));
    std::array<IndexedAllocation, 5> active{};
    const bool scaled = scaled_ && options.scaled_arithmetic;
    std::optional<Rational> coefficient;
    if (scaled && threshold && *threshold > Rational(0)) {
        try { coefficient = Rational(87) * Rational(cost_scale_, outcome_scale_) / *threshold; }
        catch (const std::overflow_error&) { /* exact Rational decision below */ }
    }
    // An upper bound on G is the largest collection mean: G is its ticket-weighted average.
    // Cost lower bounds include each selected bucket once and cheapest remaining wear units.
    const auto cannot_qualify = [&](const Rational mean, const Rational minimum_cost) {
        if (!threshold || !options.pruning) { return false; }
        try { return Rational(87) * mean / minimum_cost <= *threshold; }
        catch (const std::overflow_error&) { ++stats.unavailable_bounds; return false; }
    };
    const auto emit = [&](const std::size_t size) {
        tick();
        if (stats.candidates_considered >= limits.max_evaluations) { throw Stopped{Completion::evaluation_budget}; }
        ++stats.candidates_considered;
        Rational cost, gross;
        std::int64_t tickets = 0;
        std::int64_t c = 0, s = 0;
        bool use_scaled = scaled;
        bool qualifies = true;
        if (scaled) {
            try {
                for (std::size_t i = 0; i < size; ++i) {
                    const auto& b = buckets_[active[i].bucket];
                    const auto q = active[i].quantity;
                    c = add(c, mul(q, b.cost_units)); s = add(s, mul(q, b.sum_units));
                    tickets = add(tickets, mul(q, collections_[b.collection].count));
                }
            } catch (const std::overflow_error&) { use_scaled = false; tickets = 0; }
        }
        if (use_scaled) {
            bool decided = false;
            if (threshold && *threshold == Rational(0)) { qualifies = s > 0; decided = true; }
            else if (coefficient) {
                try {
                    qualifies = mul(coefficient->numerator(), s) > mul(mul(coefficient->denominator(), tickets), c);
                    decided = true; ++stats.integer_decisions;
                } catch (const std::overflow_error&) { /* fall back, never wrap */ }
            }
            // No distributions, strings, or result values are built for fast rejected candidates.
            if (decided && !qualifies) { ++stats.candidates_evaluated; return; }
            cost = Rational(c, cost_scale_);
            gross = Rational(s, outcome_scale_) / Rational(tickets);
            if (!decided && threshold) { qualifies = (gross * Rational(87, 100) / cost) * Rational(100) > *threshold; }
        } else {
            Rational sum;
            for (std::size_t i = 0; i < size; ++i) {
                const auto& b = buckets_[active[i].bucket];
                const auto q = active[i].quantity;
                cost = cost + Rational(q) * dataset().buckets[active[i].bucket].unit_cost.value;
                sum = sum + Rational(q) * collections_[b.collection].sum;
                tickets = add(tickets, mul(q, collections_[b.collection].count));
            }
            gross = sum / Rational(tickets);
            if (threshold) { qualifies = (gross * Rational(87, 100) / cost) * Rational(100) > *threshold; }
        }
        if (!qualifies) { ++stats.candidates_evaluated; return; }
        const auto hit = values(active, size, cost, gross);
        ++stats.candidates_evaluated; ++stats.qualifying_found;
        if (result.qualifying.size() == capacity_limit) { throw Stopped{Completion::result_budget}; }
        if (result.qualifying.size() == result.qualifying.capacity()) {
            const auto old = result.qualifying.capacity();
            const auto next = old == 0 ? std::min<std::size_t>(64, capacity_limit) :
                old + std::min(old, capacity_limit - old);
            result.qualifying.reserve(next);
        }
        result.qualifying.push_back(hit);
    };
    try {
        stats.domain_contracts = domain_size(profile);
        stats.domain_count_available = true;
        tick();
        if (stats.domain_contracts != 0) {
            Rational mean;
            std::array<Rational, 2> cheapest{};
            for (std::size_t side = 0; side < 2; ++side) {
                bool first = true;
                for (const auto index : sides_[side]) {
                    const auto cost = dataset().buckets[index].unit_cost.value;
                    if (first || cost < cheapest[side]) { cheapest[side] = cost; }
                    mean = std::max(mean, collections_[buckets_[index].collection].mean);
                    first = false;
                }
            }
            bool root_pruned = false;
            if (threshold && options.pruning) {
                try {
                    root_pruned = cannot_qualify(mean, Rational(lower) * cheapest[0] + Rational(higher) * cheapest[1]);
                } catch (const std::overflow_error&) { ++stats.unavailable_bounds; }
            }
            if (root_pruned) { stats.candidates_pruned = stats.domain_contracts; }
            else {
                for (std::size_t nl = 1; nl <= 4 && nl <= sides_[0].size(); ++nl) {
                    for (std::size_t nh = 1; nl + nh <= 5 && nh <= sides_[1].size(); ++nh) {
                        if (nl > static_cast<std::size_t>(lower) || nh > static_cast<std::size_t>(higher)) { continue; }
                        const auto size = nl + nh;
                        const auto support = [&] {
                            ++stats.supports;
                            Rational max_mean, minimum_cost;
                            std::array<Rational, 2> cheapest_selected{};
                            bool pruned = false;
                            if (threshold && options.pruning) {
                                try {
                                    for (std::size_t i = 0; i < size; ++i) {
                                        const auto index = active[i].bucket;
                                        const auto side = i < nl ? 0U : 1U;
                                        const auto price = dataset().buckets[index].unit_cost.value;
                                        if (i == 0 || i == nl || price < cheapest_selected[side]) { cheapest_selected[side] = price; }
                                        minimum_cost = minimum_cost + price;
                                        max_mean = std::max(max_mean, collections_[buckets_[index].collection].mean);
                                    }
                                    minimum_cost = minimum_cost + Rational(lower - static_cast<int>(nl)) * cheapest_selected[0]
                                        + Rational(higher - static_cast<int>(nh)) * cheapest_selected[1];
                                    pruned = cannot_qualify(max_mean, minimum_cost);
                                } catch (const std::overflow_error&) { ++stats.unavailable_bounds; }
                            }
                            if (pruned) {
                                stats.candidates_pruned += count_mul(choose(static_cast<std::size_t>(lower - 1), nl - 1),
                                    choose(static_cast<std::size_t>(higher - 1), nh - 1));
                                return;
                            }
                            // Positive compositions; the final quantity on each side is forced.
                            const auto compose = [&](auto&& self, const std::size_t position, const int left) -> void {
                                tick();
                                const auto end = position < nl ? nl : size;
                                if (position + 1 == end) {
                                    active.at(position).quantity = left;
                                    if (end == nl) { self(self, nl, higher); } else { emit(size); }
                                } else {
                                    const auto remaining = static_cast<int>(end - position - 1);
                                    for (int q = 1; q <= left - remaining; ++q) {
                                        active.at(position).quantity = q; self(self, position + 1, left - q);
                                    }
                                }
                            };
                            compose(compose, 0, lower);
                        };
                        // Choose sorted supports within each wear side, independent of quantity generation.
                        const auto select = [&](auto&& self, const std::size_t position, const std::size_t begin) -> void {
                            tick();
                            const auto side = position < nl ? 0U : 1U;
                            const auto end = side == 0 ? nl : size;
                            const auto remaining = end - position;
                            const auto& indices = sides_[side];
                            for (auto i = begin; i + remaining <= indices.size(); ++i) {
                                // Keep the recursive position bound locally checked, including
                                // aggressively inlined Release builds (-O3 array-bounds analysis).
                                active.at(position).bucket = indices[i];
                                if (position + 1 == size) { tick(); support(); }
                                else if (position + 1 == nl) { self(self, nl, 0); }
                                else { self(self, position + 1, i + 1); }
                            }
                        };
                        select(select, 0, 0);
                    }
                }
            }
        }
    } catch (const Stopped& stop) { result.completion = stop.reason; }
    catch (const std::overflow_error& error) { result.completion = Completion::arithmetic_limit; result.detail = error.what(); }
    stats.result_capacity_bytes = result.qualifying.capacity() * sizeof(OptimizedHit);
    stats.elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (result.complete() && stats.candidates_evaluated + stats.candidates_pruned != stats.domain_contracts) {
        throw std::logic_error("completed search does not account for its domain");
    }
    return result;
}

Contract OptimizedEngine::contract(const OptimizedHit& hit) const {
    if (hit.active < 2 || hit.active > 5) { throw std::invalid_argument("invalid optimized hit support"); }
    std::vector<Allocation> allocations;
    for (std::size_t i = 0; i < hit.active; ++i) {
        if (hit.inputs[i].bucket >= buckets_.size()) { throw std::invalid_argument("invalid optimized bucket index"); }
        allocations.push_back({dataset().buckets[hit.inputs[i].bucket].id, hit.inputs[i].quantity});
    }
    return validated_.canonical_contract(allocations);
}

Evaluation OptimizedEngine::evaluate(const Contract& value) const {
    if (value.snapshot_id != dataset().snapshot_id || value.scenario_id != dataset().scenario_id) {
        throw std::invalid_argument("contract snapshot/scenario does not match dataset");
    }
    Evaluation result;
    result.contract = validated_.canonical_contract(value.allocations);
    result.policy.output_valuation = dataset().output_valuation_policy_id;
    result.limitations = dataset().limitations;
    result.limitations.push_back("Modeled quote-basket result; exact float feasibility and listing supply are unverified.");
    std::vector<std::int64_t> quantities(collections_.size(), 0);
    Rational sum;
    std::int64_t tickets = 0;
    for (const auto& allocation : result.contract.allocations) {
        const auto found = std::lower_bound(dataset().buckets.begin(), dataset().buckets.end(), allocation.bucket,
            [](const InputBucket& b, const BucketId& id) { return b.id < id; });
        const auto index = static_cast<std::size_t>(found - dataset().buckets.begin());
        const auto c = buckets_[index].collection;
        quantities[c] += allocation.quantity;
        result.cost = result.cost + Rational(allocation.quantity) * found->unit_cost.value;
        sum = sum + Rational(allocation.quantity) * collections_[c].sum;
        tickets = add(tickets, mul(allocation.quantity, collections_[c].count));
        limitation(result.limitations, found->unit_cost, "bucket " + found->id.value);
    }
    bool first = true;
    for (std::size_t c = 0; c < collections_.size(); ++c) {
        if (!quantities[c]) { continue; }
        const auto& collection = dataset().collections[c];
        if (first || collections_[c].worst < result.worst_net) { result.worst_net = collections_[c].worst; }
        first = false;
        for (std::size_t o = 0; o < collection.outcomes.size(); ++o) {
            const auto& outcome = collection.outcomes[o];
            result.outcomes.push_back({collection.id, outcome.id, Rational(quantities[c], tickets),
                *outcome.price, collections_[c].receipts[o]});
            limitation(result.limitations, *outcome.price, collection.id.value + "/" + outcome.id.value);
        }
    }
    result.gross_ev = sum / Rational(tickets);
    result.net_ev = result.gross_ev * Rational(87, 100);
    result.expected_profit = result.net_ev - result.cost;
    result.return_percent = result.net_ev / result.cost * Rational(100);
    result.roi_percent = result.return_percent - Rational(100);
    return result;
}
} // namespace tradeup
