#include "tradeup/reference.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <utility>

using namespace tradeup;
static_assert(!std::is_constructible_v<Rational, double>);
static_assert(!std::is_constructible_v<Rational, double, int>);
static_assert(!std::is_constructible_v<Rational, int, double>);
namespace {
void require(const bool condition) {
    if (!condition) { throw std::runtime_error("check failed"); }
}
template<class Exception, class Function> void rejects(Function action) {
    try { action(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected exception was not thrown");
}
Quote quote(const std::int64_t price) { return {Rational(price), QuoteKind::synthetic, "unit-test"}; }
Dataset catalog(const int lower = 2, const int higher = 2) {
    Dataset data{"snapshot", "scenario", "selected-higher-wear-v1", "test-unit", {}, {}, {}};
    data.collections.push_back({CollectionId{"C"}, {{OutcomeId{"O"}, quote(100)}}, true});
    for (int index = 0; index < lower + higher; ++index) {
        data.buckets.push_back({BucketId{"b" + std::to_string(index)}, CollectionId{"C"},
            index < lower ? WearCategory::lower : WearCategory::higher, quote(1)});
    }
    return data;
}
Evaluation evaluate(ReferenceEngine& engine, std::vector<Allocation> allocations) {
    return engine.evaluate(engine.canonical_contract(allocations));
}
} // namespace

int main() {
    int passed = 0;
    int failed = 0;
    const auto test = [&](const std::string& name, const std::function<void()>& action) {
        try { action(); ++passed; }
        catch (const std::exception& error) { ++failed; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
    };
    test("normalized fractions and exact decimals", [] {
        require(Rational(2, 4) == Rational(1, 2));
        require(Rational::parse("-12.125") == Rational(-97, 8));
        require(Rational::parse("0.000000000000000001") == Rational(1, 1'000'000'000'000'000'000));
        require(Rational::parse("-0/13").str() == "0/1");
    });
    test("malformed and nonfinite prices", [] {
        for (const auto* text : {"", "-", "+1", ".1", "1.", "1e3", "nan", "inf", "1/0", "1/-2", "1/2/3", " 1", "1x", "1.2.3", "1/2.0"}) {
            rejects<std::invalid_argument>([&] { (void)Rational::parse(text); });
        }
        rejects<std::invalid_argument>([] { (void)Rational::parse("0.0000000000000000001"); });
    });
    test("signed arithmetic and zero", [] {
        require(Rational(-1, 3) + Rational(1, 2) == Rational(1, 6));
        require(Rational(-1, 3) - Rational(1, 2) == Rational(-5, 6));
        require(Rational(-2, 3) / Rational(-4, 5) == Rational(5, 6));
        require(Rational(0) * Rational(-4, 5) == Rational(0));
        rejects<std::invalid_argument>([] { (void)(Rational(1) / Rational(0)); });
    });
    test("numeric limits fail closed", [] {
        constexpr auto max = std::numeric_limits<std::int64_t>::max();
        rejects<std::invalid_argument>([] { (void)Rational(std::numeric_limits<std::int64_t>::min()); });
        rejects<std::invalid_argument>([] { (void)Rational(1, -1); });
        rejects<std::overflow_error>([] { (void)Rational::parse("9223372036854775808"); });
        rejects<std::overflow_error>([&] { (void)(Rational(max) + Rational(1)); });
        rejects<std::overflow_error>([&] { (void)(Rational(-max) - Rational(1)); });
        rejects<std::overflow_error>([&] { (void)(Rational(max) * Rational(2)); });
        require(Rational(max, 2) * Rational(2, max) == Rational(1));
        require(Rational(max, max - 1) > Rational(max - 1, max));
        require(Rational(-max, max - 1) < Rational(-max + 1, max));
    });
    test("zero and negative input prices", [] {
        for (const int price : {0, -1}) {
            auto data = catalog(); data.buckets[0].unit_cost.value = Rational(price);
            rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        }
    });
    test("missing or negative outcome price", [] {
        auto data = catalog(); data.collections[0].outcomes[0].price.reset();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.collections[0].outcomes[0].price->value = Rational(-1);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("explicit zero outcome remains possible", [] {
        auto data = catalog(); data.collections[0].outcomes[0].price->value = Rational(0);
        ReferenceEngine engine(data);
        const auto value = evaluate(engine, {{{"b0"}, 10}});
        require(value.outcomes.size() == 1 && value.outcomes[0].probability == Rational(1));
        require(value.gross_ev == Rational(0) && value.roi_percent == Rational(-100));
    });
    test("empty or incomplete outcome sets", [] {
        auto data = catalog(); data.collections[0].outcomes.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.collections[0].declared_outcomes_complete = false;
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("duplicate collection and outcome IDs", [] {
        auto data = catalog(); data.collections.push_back(data.collections[0]);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.collections[0].outcomes.push_back(data.collections[0].outcomes[0]);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("duplicate and dangling bucket IDs", [] {
        auto data = catalog(); data.buckets.push_back(data.buckets[0]);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.buckets[0].collection = CollectionId{"missing"};
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("malformed identities cannot collide through delimiters", [] {
        auto data = catalog(); data.snapshot_id = "snapshot|scenario";
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.buckets[0].id = BucketId{"b:0"};
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.scenario_id.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("invalid enum values rejected", [] {
        auto data = catalog(); data.buckets[0].wear = static_cast<WearCategory>(7);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.buckets[0].unit_cost.kind = static_cast<QuoteKind>(99);
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        ReferenceEngine engine(catalog());
        rejects<std::invalid_argument>([&] { (void)engine.enumerate(static_cast<SearchProfile>(9)); });
    });
    test("canonical order and split entries", [] {
        ReferenceEngine engine(catalog());
        const auto a = engine.canonical_contract(std::vector<Allocation>{{{"b2"}, 2}, {{"b0"}, 5}, {{"b2"}, 3}});
        const auto b = engine.canonical_contract(std::vector<Allocation>{{{"b0"}, 5}, {{"b2"}, 5}});
        require(a == b);
        require(a.serialize() == "contract-v1|snapshot|scenario|b0:5,b2:5");
    });
    test("invalid quantities and unknown buckets", [] {
        ReferenceEngine engine(catalog());
        for (const int quantity : {-1, 0, 9, 11, std::numeric_limits<int>::max()}) {
            rejects<std::invalid_argument>([&] { (void)engine.canonical_contract(std::vector<Allocation>{{{"b0"}, quantity}}); });
        }
        rejects<std::invalid_argument>([&] { (void)engine.canonical_contract(std::vector<Allocation>{{{"missing"}, 10}}); });
        rejects<std::invalid_argument>([&] { (void)engine.canonical_contract(std::vector<Allocation>{}); });
    });
    test("untrusted serialized contracts validated", [] {
        for (const std::vector<Allocation>& allocations : {
            std::vector<Allocation>{{{"b0"}, 0}, {{"b1"}, 10}},
            std::vector<Allocation>{{{"b1"}, 5}, {{"b0"}, 5}},
            std::vector<Allocation>{{{"b0"}, 5}, {{"b0"}, 5}}}) {
            rejects<std::invalid_argument>([&] { (void)Contract{"s", "s", allocations}.serialize(); });
        }
    });
    test("snapshot and scenario mismatch rejected", [] {
        ReferenceEngine engine(catalog());
        auto contract = engine.canonical_contract(std::vector<Allocation>{{{"b0"}, 10}});
        contract.snapshot_id = "other";
        rejects<std::invalid_argument>([&] { (void)engine.evaluate(contract); });
        contract.snapshot_id = "snapshot"; contract.scenario_id = "other";
        rejects<std::invalid_argument>([&] { (void)engine.evaluate(contract); });
    });
    test("contract and policy identities separated", [] {
        auto data = catalog(); ReferenceEngine first(data);
        data.output_valuation_policy_id = "another-valuation-v1"; ReferenceEngine second(data);
        const auto a = evaluate(first, {{{"b0"}, 10}});
        const auto b = evaluate(second, {{{"b0"}, 10}});
        require(a.contract == b.contract && a.serialize_identity() != b.serialize_identity());
    });
    test("same collection across wear buckets counted once", [] {
        auto data = catalog(); data.collections[0].outcomes.push_back({OutcomeId{"P"}, quote(200)});
        ReferenceEngine engine(data);
        const auto result = evaluate(engine, {{{"b0"}, 5}, {{"b2"}, 5}});
        require(result.outcomes.size() == 2 && result.gross_ev == Rational(150));
        require(result.outcomes[0].probability == Rational(1, 2));
    });
    test("six outcomes are distinct events", [] {
        auto data = catalog(); data.collections[0].outcomes.clear();
        for (int price = 1; price <= 6; ++price) { data.collections[0].outcomes.push_back({OutcomeId{"o" + std::to_string(price)}, quote(price)}); }
        ReferenceEngine engine(data);
        const auto result = evaluate(engine, {{{"b0"}, 10}});
        require(result.outcomes.size() == 6 && result.gross_ev == Rational(7, 2));
        for (const auto& outcome : result.outcomes) { require(outcome.probability == Rational(1, 6)); }
    });
    test("strict threshold equality and immediate neighbors", [] {
        auto data = catalog(); data.collections[0].outcomes[0].price->value = Rational(115);
        for (auto& input : data.buckets) { input.unit_cost.value = Rational(87, 10); }
        ReferenceEngine engine(data); const auto value = evaluate(engine, {{{"b0"}, 10}});
        require(value.return_percent == Rational(115));
        require(!value.qualifies() && value.qualifies(Rational(114'999'999, 1'000'000)));
        require(!value.qualifies(Rational(115'000'001, 1'000'000)));
        require(engine.search().qualifying.empty());
        rejects<std::invalid_argument>([&] { (void)value.qualifies(Rational(-1)); });
    });
    test("positive EV does not establish worst-case profit", [] {
        auto data = catalog(); data.collections[0].outcomes = {{OutcomeId{"loss"}, quote(1)}, {OutcomeId{"win"}, quote(100)}};
        ReferenceEngine engine(data); const auto value = evaluate(engine, {{{"b0"}, 10}});
        require(value.net_ev > value.cost && value.worst_net < value.cost);
        require(value.expected_profit == value.net_ev - value.cost);
    });
    test("derived estimates retain source and limitations", [] {
        auto data = catalog(); auto& price = *data.collections[0].outcomes[0].price;
        price.kind = QuoteKind::composite_estimate; price.source = "workbook-cell-average";
        ReferenceEngine engine(data); const auto value = evaluate(engine, {{{"b0"}, 10}});
        require(value.outcomes[0].gross_price.source == "workbook-cell-average");
        require(std::any_of(value.limitations.begin(), value.limitations.end(), [](const auto& text) { return text.find("Estimated/composite") != std::string::npos; }));
        data.collections[0].outcomes[0].price->source.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("2 by 2 catalog has 36 contracts", [] {
        ReferenceEngine engine(catalog()); const auto enumeration = engine.enumerate();
        require(enumeration.contracts.size() == 36);
        std::set<std::string> ids;
        for (const auto& contract : enumeration.contracts) { ids.insert(contract.serialize()); }
        require(ids.size() == 36);
    });
    test("legacy skipped branch now has all 56 allocations", [] {
        ReferenceEngine engine(catalog(1, 4)); const auto result = engine.enumerate();
        require(result.contracts.size() == 56);
        const auto witness = engine.canonical_contract(std::vector<Allocation>{{{"b0"}, 5}, {{"b4"}, 5}});
        require(std::find(result.contracts.begin(), result.contracts.end(), witness) != result.contracts.end());
    });
    test("active support cap excludes six-bucket contracts", [] {
        ReferenceEngine engine(catalog(3, 3)); const auto result = engine.enumerate();
        require(result.contracts.size() == 405);
        for (const auto& contract : result.contracts) { require(contract.allocations.size() >= 2 && contract.allocations.size() <= 5); }
    });
    test("equal returns preserve more than 99 distinct results", [] {
        ReferenceEngine engine(catalog(3, 3)); const auto result = engine.search();
        require(result.candidates_evaluated == 405 && result.qualifying.size() == 405);
        require(result.qualifying[0].return_percent == result.qualifying[1].return_percent);
        require(result.qualifying[0].contract != result.qualifying[1].contract);
    });
    test("clean 6 by 4 extension shares enumerator", [] {
        ReferenceEngine engine(catalog(3, 3)); const auto result = engine.enumerate(SearchProfile::historical_6_4);
        require(result.contracts.size() == 390);
        require(profile_id(SearchProfile::historical_6_4) == "historical-6-4-up-to-5-buckets-v1");
    });
    test("catalog without both wear categories is empty search", [] {
        ReferenceEngine engine(catalog(2, 0)); require(engine.enumerate().contracts.empty());
    });
    test("determinism independent of input vector order", [] {
        auto data = catalog(); ReferenceEngine first(data);
        std::reverse(data.buckets.begin(), data.buckets.end()); ReferenceEngine second(data);
        require(first.enumerate().contracts == second.enumerate().contracts);
    });
    test("resource exhaustion returns no partial complete result", [] {
        ReferenceEngine engine(catalog());
        rejects<SearchLimitExceeded>([&] { (void)engine.enumerate(SearchProfile::historical_5_5, {1, 100}); });
        rejects<SearchLimitExceeded>([&] { (void)engine.search(SearchProfile::historical_5_5, Rational(115), {100'000, 35}); });
        require(engine.enumerate(SearchProfile::historical_5_5, {100'000, 36}).contracts.size() == 36);
        rejects<std::invalid_argument>([&] { (void)engine.enumerate(SearchProfile::historical_5_5, {0, 100}); });
    });
    test("large catalog cannot silently launch enumeration", [] {
        ReferenceEngine engine(catalog(7, 6));
        rejects<std::invalid_argument>([&] { (void)engine.enumerate(); });
        require(evaluate(engine, {{{"b0"}, 10}}).cost == Rational(10));
    });
    test("evaluation overflow is an explicit error", [] {
        auto data = catalog(); data.buckets[0].unit_cost.value = Rational(std::numeric_limits<std::int64_t>::max());
        ReferenceEngine engine(data);
        rejects<std::overflow_error>([&] { (void)evaluate(engine, {{{"b0"}, 10}}); });
    });
    test("empty dataset metadata and catalogs rejected", [] {
        auto data = catalog(); data.price_unit.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.collections.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.buckets.clear();
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
        data = catalog(); data.output_valuation_policy_id = std::string(129, 'x');
        rejects<std::invalid_argument>([&] { (void)ReferenceEngine(data); });
    });
    test("outcome and collection ordering cannot change evaluations", [] {
        auto data = catalog();
        data.collections[0].outcomes.push_back({OutcomeId{"A"}, quote(50)});
        data.collections.push_back({CollectionId{"D"}, {{OutcomeId{"X"}, quote(9)}}, true});
        data.buckets[2].collection = CollectionId{"D"};
        ReferenceEngine first(data);
        std::reverse(data.collections[0].outcomes.begin(), data.collections[0].outcomes.end());
        std::reverse(data.collections.begin(), data.collections.end());
        std::reverse(data.buckets.begin(), data.buckets.end());
        ReferenceEngine second(data);
        const auto a = evaluate(first, {{{"b0"}, 5}, {{"b2"}, 5}});
        const auto b = evaluate(second, {{{"b2"}, 5}, {{"b0"}, 5}});
        require(a.gross_ev == b.gross_ev && a.serialize_identity() == b.serialize_identity());
        require(a.outcomes.size() == b.outcomes.size());
        for (std::size_t index = 0; index < a.outcomes.size(); ++index) {
            require(a.outcomes[index].collection == b.outcomes[index].collection);
            require(a.outcomes[index].outcome == b.outcomes[index].outcome);
            require(a.outcomes[index].probability == b.outcomes[index].probability);
        }
    });
    test("sub-epsilon price changes still determine strict qualification", [] {
        for (const int direction : {-1, 1}) {
            auto data = catalog();
            for (auto& input : data.buckets) { input.unit_cost.value = Rational(87, 10); }
            data.collections[0].outcomes[0].price->value = Rational(115'000'000'000'000 + direction, 1'000'000'000'000);
            ReferenceEngine engine(data);
            const auto value = evaluate(engine, {{{"b0"}, 10}});
            require(value.qualifies() == (direction > 0));
        }
    });
    test("node budget permits exactly the completed traversal", [] {
        ReferenceEngine engine(catalog()); const auto full = engine.enumerate();
        require(engine.enumerate(SearchProfile::historical_5_5, {full.nodes_visited, 36}).contracts == full.contracts);
        rejects<SearchLimitExceeded>([&] {
            (void)engine.enumerate(SearchProfile::historical_5_5, {full.nodes_visited - 1, 36});
        });
    });
    std::cout << "Reference unit tests: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
