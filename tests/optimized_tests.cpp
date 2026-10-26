#include "tradeup/optimized.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <random>

using namespace tradeup;
namespace {
void require(const bool condition) { if (!condition) { throw std::runtime_error("check failed"); } }
template<class Exception, class F> void rejects(F action) {
    try { action(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected exception");
}
Quote quote(Rational p) { return {p, QuoteKind::synthetic, "optimized-test"}; }
Dataset catalog(int lower = 3, int higher = 3, int seed = 0) {
    Dataset d{"optimized-" + std::to_string(seed), "scenario", "basket-v1", "unit", {}, {}, {"declared model"}};
    std::mt19937 rng(static_cast<unsigned int>(seed));
    for (int c = 0; c < 3; ++c) {
        Collection collection{CollectionId{"c" + std::to_string(c)}, {}, true};
        for (int o = 0; o <= c; ++o) {
            collection.outcomes.push_back({OutcomeId{"o" + std::to_string(o)}, quote(Rational(rng() % 100U, 3))});
        }
        d.collections.push_back(std::move(collection));
    }
    for (int i = 0; i < lower + higher; ++i) {
        d.buckets.push_back({BucketId{"b" + std::to_string(i)}, CollectionId{"c" + std::to_string(i % 3)},
            i < lower ? WearCategory::lower : WearCategory::higher, quote(Rational(1 + rng() % 20U, 10))});
    }
    return d;
}
OptimizedLimits generous() { return {2'000'000, 10'000'000, 2'000'000, 512 * 1024 * 1024, std::chrono::milliseconds(60'000)}; }
std::uint64_t compared = 0, distributions = 0, searches = 0;
void equal_values(const Evaluation& a, const Evaluation& b) {
    require(a.contract == b.contract && a.serialize_identity() == b.serialize_identity());
    require(a.cost == b.cost && a.gross_ev == b.gross_ev && a.net_ev == b.net_ev);
    require(a.return_percent == b.return_percent && a.roi_percent == b.roi_percent);
    require(a.expected_profit == b.expected_profit && a.worst_net == b.worst_net);
    require(a.limitations == b.limitations && a.outcomes.size() == b.outcomes.size());
    for (std::size_t i = 0; i < a.outcomes.size(); ++i) {
        const auto& x = a.outcomes[i]; const auto& y = b.outcomes[i];
        require(x.collection == y.collection && x.outcome == y.outcome && x.probability == y.probability);
        require(x.gross_price.value == y.gross_price.value && x.gross_price.kind == y.gross_price.kind);
        require(x.gross_price.source == y.gross_price.source && x.net_receipt == y.net_receipt);
    }
    ++distributions;
}
void differential(Dataset d, SearchProfile profile, Rational threshold = Rational(115)) {
    ReferenceEngine ref(d); OptimizedEngine opt(d);
    const auto enumeration = ref.enumerate(profile, {10'000'000, 2'000'000});
    std::map<std::string, Evaluation> expected;
    for (const auto& c : enumeration.contracts) {
        auto e = ref.evaluate(c); expected.emplace(c.serialize(), std::move(e));
    }
    require(opt.domain_size(profile) == expected.size());
    const auto all = opt.search(profile, std::nullopt, generous(), {false, true});
    require(all.complete() && all.qualifying.size() == expected.size());
    std::map<std::string, bool> seen;
    for (const auto& hit : all.qualifying) {
        const auto c = opt.contract(hit); const auto key = c.serialize();
        require(seen.emplace(key, true).second && expected.contains(key));
        const auto e = opt.evaluate(c); equal_values(expected.at(key), e);
        require(hit.cost == e.cost && hit.gross_ev == e.gross_ev && hit.net_ev == e.net_ev);
        require(hit.expected_profit == e.expected_profit && hit.return_percent == e.return_percent);
        require(hit.roi_percent == e.roi_percent && hit.worst_net == e.worst_net);
        ++compared;
    }
    std::map<std::string, Rational> qualified;
    for (const auto& [key, e] : expected) { if (e.qualifies(threshold)) { qualified.emplace(key, e.return_percent); } }
    for (bool pruning : {false, true}) {
        for (bool scaled : {false, true}) {
            const auto found = opt.search(profile, threshold, generous(), {pruning, scaled});
            require(found.complete());
            require(found.statistics.candidates_evaluated + found.statistics.candidates_pruned == expected.size());
            std::map<std::string, Rational> actual;
            for (const auto& hit : found.qualifying) {
                const auto c = opt.contract(hit);
                require(actual.emplace(c.serialize(), hit.return_percent).second);
                equal_values(expected.at(c.serialize()), opt.evaluate(c));
            }
            require(actual == qualified); ++searches;
        }
    }
}
} // namespace

int main() {
    int passed = 0, failed = 0;
    const auto test = [&](const char* name, const std::function<void()>& action) {
        try { action(); ++passed; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
    };
    test("both profiles complete sets and distributions", [] {
        for (auto p : {SearchProfile::historical_5_5, SearchProfile::historical_6_4}) { differential(catalog(), p); }
    });
    test("seeded adversarial rational shared-collection searches", [] {
        for (int seed = 1; seed <= 40; ++seed) {
            for (auto p : {SearchProfile::historical_5_5, SearchProfile::historical_6_4}) {
                differential(catalog(1 + seed % 3, 1 + (seed / 3) % 3, seed), p, Rational(seed % 4 == 0 ? 0 : 115));
            }
        }
    });
    test("reference maximum twelve buckets", [] { differential(catalog(6, 6), SearchProfile::historical_5_5); });
    test("empty domains", [] {
        differential(catalog(0, 3), SearchProfile::historical_5_5);
        differential(catalog(3, 0), SearchProfile::historical_6_4);
    });
    test("historically omitted one-four compositions", [] {
        differential(catalog(1, 4), SearchProfile::historical_5_5);
        differential(catalog(1, 4), SearchProfile::historical_6_4);
    });
    test("identical returns retain 405 distinct contracts", [] {
        auto d = catalog();
        for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(100); } }
        for (auto& b : d.buckets) { b.unit_cost.value = Rational(1); }
        OptimizedEngine opt(d); const auto r = opt.search({}, Rational(115), generous());
        require(r.complete() && r.qualifying.size() == 405);
        differential(d, SearchProfile::historical_5_5);
    });
    test("zero outcomes retained by all-contract verification", [] {
        auto d = catalog(); for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(0); } }
        differential(d, SearchProfile::historical_5_5, Rational(0));
    });
    test("strict bound equality and tiny neighbors", [] {
        auto d = catalog(2, 2);
        for (auto& b : d.buckets) { b.unit_cost.value = Rational(87, 1000); }
        for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(115, 100); } }
        OptimizedEngine opt(d); const auto r = opt.search();
        require(r.complete() && r.qualifying.empty() && r.statistics.candidates_pruned == 36);
        for (auto t : {Rational(115), Rational(114999999999999LL, 1000000000000LL), Rational(115000000000001LL, 1000000000000LL)}) {
            differential(d, SearchProfile::historical_5_5, t);
        }
    });
    test("support bounds prune strict subsets", [] {
        auto d = catalog(3, 3);
        for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(100); } }
        for (std::size_t i = 0; i < d.buckets.size(); ++i) { d.buckets[i].unit_cost.value = Rational(i % 3 == 0 ? 1 : 100); }
        OptimizedEngine opt(d); const auto r = opt.search({}, Rational(115), generous());
        require(r.complete() && r.statistics.candidates_pruned > 0 && r.statistics.candidates_evaluated > 0);
        differential(d, SearchProfile::historical_5_5);
    });
    test("permutations deterministic hits and statistics", [] {
        auto d = catalog(); OptimizedEngine a(d);
        std::reverse(d.buckets.begin(), d.buckets.end()); std::reverse(d.collections.begin(), d.collections.end());
        for (auto& c : d.collections) { std::reverse(c.outcomes.begin(), c.outcomes.end()); }
        OptimizedEngine b(d); const auto x = a.search(); const auto y = b.search();
        require(x.complete() && y.complete() && x.qualifying.size() == y.qualifying.size());
        for (std::size_t i = 0; i < x.qualifying.size(); ++i) { equal_values(a.evaluate(a.contract(x.qualifying[i])), b.evaluate(b.contract(y.qualifying[i]))); }
        require(x.statistics.work == y.statistics.work && x.statistics.candidates_pruned == y.statistics.candidates_pruned);
    });
    test("evaluation budget exact and one below", [] {
        OptimizedEngine e(catalog()); auto l = generous(); l.max_evaluations = 405;
        auto r = e.search({}, std::nullopt, l); require(r.complete());
        l.max_evaluations = 404; r = e.search({}, std::nullopt, l);
        require(r.completion == Completion::evaluation_budget && r.statistics.candidates_evaluated == 404);
    });
    test("work budget exact and one below", [] {
        OptimizedEngine e(catalog()); auto l = generous(); const auto r = e.search({}, std::nullopt, l);
        l.max_work = r.statistics.work; require(e.search({}, std::nullopt, l).complete());
        --l.max_work; require(e.search({}, std::nullopt, l).completion == Completion::work_budget);
    });
    test("result storage budget explicit observed extra qualifier", [] {
        OptimizedEngine e(catalog()); auto l = generous(); l.max_results = 1;
        const auto r = e.search({}, std::nullopt, l);
        require(r.completion == Completion::result_budget && r.qualifying.size() == 1);
        require(r.statistics.qualifying_found == 2 && r.statistics.candidates_evaluated == 2);
        l.max_results = 405; require(e.search({}, std::nullopt, l).complete());
    });
    test("result byte cap includes vector capacity", [] {
        OptimizedEngine e(catalog()); auto l = generous(); l.max_result_bytes = sizeof(OptimizedHit) * 7;
        const auto r = e.search({}, std::nullopt, l);
        require(r.completion == Completion::result_budget && r.qualifying.size() == 7);
        require(r.statistics.result_capacity_bytes <= l.max_result_bytes);
    });
    test("time budget interrupts large finite domain", [] {
        OptimizedEngine e(catalog(29, 29)); auto l = generous(); l.max_time = std::chrono::milliseconds(1);
        const auto r = e.search({}, std::nullopt, l);
        require(r.completion == Completion::time_budget && r.statistics.candidates_evaluated < r.statistics.domain_contracts);
    });
    test("58 bucket structural catalog and bounded prefix", [] {
        const auto d = catalog(29, 29); OptimizedEngine e(d); ReferenceEngine reference(d);
        auto l = generous(); l.max_evaluations = 100;
        const auto r = e.search({}, std::nullopt, l);
        require(r.completion == Completion::evaluation_budget && r.statistics.candidates_evaluated == 100);
        require(r.statistics.domain_contracts > 1'000'000);
        for (const auto& hit : r.qualifying) { equal_values(reference.evaluate(e.contract(hit)), e.evaluate(e.contract(hit))); }
    });
    test("58 bucket independently checkable root proof", [] {
        auto d = catalog(29, 29); for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(0); } }
        const auto r = OptimizedEngine(d).search();
        require(r.complete() && r.statistics.candidates_evaluated == 0 && r.statistics.candidates_pruned == r.statistics.domain_contracts);
    });
    test("catalog budgets reject before preparation", [] {
        rejects<std::invalid_argument>([] { (void)OptimizedEngine(catalog(), {5, 10, 100}); });
        rejects<std::invalid_argument>([] { (void)OptimizedEngine(catalog(), {10, 2, 100}); });
        rejects<std::invalid_argument>([] { (void)OptimizedEngine(catalog(), {10, 10, 5}); });
    });
    test("domain count overflow is explicitly unknown", [] {
        OptimizedEngine e(catalog(16000, 16000), {32000, 3, 6});
        rejects<std::overflow_error>([&] { (void)e.domain_size(SearchProfile::historical_5_5); });
        const auto r = e.search();
        require(r.completion == Completion::arithmetic_limit && !r.statistics.domain_count_available);
        require(r.statistics.candidates_evaluated == 0);
    });
    test("invalid search budgets threshold profile", [] {
        OptimizedEngine e(catalog());
        for (int i = 0; i < 5; ++i) {
            auto l = generous();
            if (i == 0) { l.max_evaluations = 0; } if (i == 1) { l.max_work = 0; }
            if (i == 2) { l.max_results = 0; } if (i == 3) { l.max_result_bytes = 1; }
            if (i == 4) { l.max_time = std::chrono::milliseconds(0); }
            rejects<std::invalid_argument>([&] { (void)e.search({}, Rational(115), l); });
        }
        rejects<std::invalid_argument>([&] { (void)e.search({}, Rational(-1)); });
        rejects<std::invalid_argument>([&] { (void)e.search(static_cast<SearchProfile>(99)); });
        auto huge_time = generous(); huge_time.max_time = std::chrono::milliseconds::max();
        rejects<std::invalid_argument>([&] { (void)e.search({}, Rational(115), huge_time); });
    });
    test("invalid input validation unchanged", [] {
        auto d = catalog(); d.buckets[0].unit_cost.value = Rational(0);
        rejects<std::invalid_argument>([&] { (void)OptimizedEngine(d); });
        d = catalog(); d.collections[0].outcomes[0].price.reset();
        rejects<std::invalid_argument>([&] { (void)OptimizedEngine(d); });
        d = catalog(); d.collections[0].declared_outcomes_complete = false;
        rejects<std::invalid_argument>([&] { (void)OptimizedEngine(d); });
        d = catalog(); d.buckets[1].id = d.buckets[0].id;
        rejects<std::invalid_argument>([&] { (void)OptimizedEngine(d); });
    });
    test("preparation arithmetic overflow explicit", [] {
        auto d = catalog(); d.collections[0].outcomes[0].price->value = Rational(std::numeric_limits<std::int64_t>::max());
        rejects<std::overflow_error>([&] { (void)OptimizedEngine(d); });
    });
    test("candidate arithmetic overflow interrupted", [] {
        auto d = catalog(1, 1); for (auto& b : d.buckets) { b.unit_cost.value = Rational(std::numeric_limits<std::int64_t>::max()); }
        OptimizedEngine e(d); const auto r = e.search({}, std::nullopt, generous(), {false, true});
        require(r.completion == Completion::arithmetic_limit && r.statistics.candidates_evaluated == 0 && !r.detail.empty());
        rejects<std::overflow_error>([&] { (void)ReferenceEngine(d).search(); });
    });
    test("unrepresentable common scale rational fallback", [] {
        auto d = catalog(1, 1);
        // Unused collection denominators defeat the common scale; the actual contracts fit.
        d.collections.push_back({CollectionId{"unused1"}, {{OutcomeId{"o"}, quote(Rational(1, 3'000'000'001LL))}}, true});
        d.collections.push_back({CollectionId{"unused2"}, {{OutcomeId{"o"}, quote(Rational(1, 4'000'000'001LL))}}, true});
        OptimizedEngine e(d); require(!e.has_scaled_arithmetic());
        const auto r = e.search({}, std::nullopt, generous(), {false, true});
        require(r.complete() && r.qualifying.size() == 1);
        differential(d, SearchProfile::historical_5_5);
    });
    test("optional bound overflow never prunes", [] {
        auto d = catalog(1, 1); for (auto& b : d.buckets) { b.unit_cost.value = Rational(1, 1'000'000'000'000'000'000LL); }
        OptimizedEngine e(d); const auto r = e.search({}, Rational(115), generous());
        require(r.statistics.unavailable_bounds > 0 && r.statistics.candidates_pruned == 0);
        require(r.completion == Completion::arithmetic_limit);
    });
    test("scaled intermediate overflow falls back without losing valid contracts", [] {
        auto d = catalog(2, 1);
        for (auto& c : d.collections) { for (auto& o : c.outcomes) { o.price->value = Rational(0); } }
        for (auto& b : d.buckets) { b.unit_cost.value = Rational(1); }
        d.buckets[1].unit_cost.value = Rational(1, 1'000'000'000'000'000'000LL);
        require(OptimizedEngine(d).has_scaled_arithmetic());
        differential(d, SearchProfile::historical_5_5, Rational(0));
    });
    test("general evaluator one through ten buckets", [] {
        auto d = catalog(5, 5); ReferenceEngine r(d); OptimizedEngine o(d);
        std::vector<Allocation> a; for (const auto& b : d.buckets) { a.push_back({b.id, 1}); }
        equal_values(r.evaluate(r.canonical_contract(a)), o.evaluate(r.canonical_contract(a)));
        a = {{d.buckets[0].id, 4}, {d.buckets[0].id, 6}};
        equal_values(r.evaluate(r.canonical_contract(a)), o.evaluate(r.canonical_contract(a)));
    });
    test("metadata provenance and policy identity", [] {
        auto d = catalog(1, 1); d.buckets[0].unit_cost.kind = QuoteKind::minimum_derived;
        d.collections[0].outcomes[0].price->kind = QuoteKind::composite_estimate;
        d.collections[1].outcomes[0].price->kind = QuoteKind::unknown;
        differential(d, SearchProfile::historical_5_5);
    });
    std::cout << passed << " optimized tests passed; " << failed << " failed; " << compared
              << " full contracts compared; " << distributions << " complete evaluations compared; " << searches << " feature searches compared\n";
    return failed ? 1 : 0;
}
