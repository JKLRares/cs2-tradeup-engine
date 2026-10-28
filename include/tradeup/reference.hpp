#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace tradeup {

// Exact normalized fractions with checked signed-64-bit intermediates.
// INT64_MIN is excluded so absolute values and negation are always defined.
class Rational {
public:
    Rational(std::int64_t numerator = 0, std::int64_t denominator = 1);
    template<class T> requires std::is_floating_point_v<T>
    Rational(T) = delete;
    template<class A, class B> requires (std::is_floating_point_v<A> || std::is_floating_point_v<B>)
    Rational(A, B) = delete;
    static Rational parse(std::string_view text);
    [[nodiscard]] std::int64_t numerator() const noexcept { return numerator_; }
    [[nodiscard]] std::int64_t denominator() const noexcept { return denominator_; }
    [[nodiscard]] std::string str() const;
    friend Rational operator+(Rational left, Rational right);
    friend Rational operator-(Rational left, Rational right);
    friend Rational operator*(Rational left, Rational right);
    friend Rational operator/(Rational left, Rational right);
    friend bool operator==(Rational, Rational) = default;
    friend std::strong_ordering operator<=>(Rational left, Rational right) noexcept;
private:
    std::int64_t numerator_;
    std::int64_t denominator_;
};

struct CollectionId { std::string value; auto operator<=>(const CollectionId&) const = default; };
struct BucketId { std::string value; auto operator<=>(const BucketId&) const = default; };
struct OutcomeId { std::string value; auto operator<=>(const OutcomeId&) const = default; };

enum class WearCategory { lower, higher };
enum class QuoteKind { recorded, minimum_derived, composite_estimate, ratio_estimate, synthetic, unknown };

struct Quote {
    Rational value;
    QuoteKind kind = QuoteKind::unknown;
    std::string source;
};
struct Outcome {
    OutcomeId id;
    std::optional<Quote> price; // Missing prices are rejected, never treated as absent outcomes.
};
struct Collection {
    CollectionId id;
    std::vector<Outcome> outcomes;
    bool declared_outcomes_complete = true; // Complete for the modeled basket, not the game.
};
struct InputBucket {
    BucketId id;
    CollectionId collection;
    WearCategory wear;
    Quote unit_cost;
};
struct Dataset {
    std::string snapshot_id; // Caller assigns a content-bound ID; no implicit hashing of metadata.
    std::string scenario_id;
    std::string output_valuation_policy_id;
    std::string price_unit;
    std::vector<Collection> collections;
    std::vector<InputBucket> buckets;
    std::vector<std::string> limitations;
};
struct Allocation {
    BucketId bucket;
    int quantity;
    auto operator<=>(const Allocation&) const = default;
};
struct Contract {
    std::string snapshot_id;
    std::string scenario_id;
    std::vector<Allocation> allocations; // Sorted, distinct, strictly positive; total ten.
    [[nodiscard]] std::string serialize() const;
    auto operator<=>(const Contract&) const = default;
};
struct PolicyIdentity {
    std::string probability = "historical-quote-tickets-v1";
    std::string output_valuation;
    std::string fee = "historical-proportional-13-percent-v1";
};
struct OutcomeProbability {
    CollectionId collection;
    OutcomeId outcome;
    Rational probability;
    Quote gross_price;
    Rational net_receipt;
};
struct Evaluation {
    Contract contract;
    PolicyIdentity policy;
    std::vector<OutcomeProbability> outcomes;
    Rational cost;
    Rational gross_ev;
    Rational net_ev;
    Rational expected_profit;
    Rational return_percent;
    Rational roi_percent;
    Rational worst_net;
    std::vector<std::string> limitations;
    [[nodiscard]] bool qualifies(Rational threshold_percent = Rational(115)) const;
    [[nodiscard]] std::string serialize_identity() const;
};

enum class SearchProfile { historical_5_5, historical_6_4 };
[[nodiscard]] std::string_view profile_id(SearchProfile profile);
struct SearchLimits {
    std::size_t max_nodes = 100'000;
    std::size_t max_contracts = 10'000;
};
class SearchLimitExceeded : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
struct Enumeration {
    std::vector<Contract> contracts;
    std::size_t nodes_visited = 0;
};
struct SearchResult {
    std::size_t candidates_evaluated = 0;
    std::size_t nodes_visited = 0;
    std::vector<Evaluation> qualifying;
};

class ReferenceEngine {
public:
    // Bounds recursion/resource use; evaluation itself has no 12-bucket or five-outcome cap.
    static constexpr std::size_t maximum_enumeration_buckets = 12;
    explicit ReferenceEngine(Dataset dataset);
    [[nodiscard]] const Dataset& dataset() const noexcept { return dataset_; }
    [[nodiscard]] Contract canonical_contract(std::span<const Allocation> allocations) const;
    [[nodiscard]] Evaluation evaluate(const Contract& contract) const;
    [[nodiscard]] Enumeration enumerate(SearchProfile profile = SearchProfile::historical_5_5,
                                        SearchLimits limits = {}) const;
    [[nodiscard]] SearchResult search(SearchProfile profile = SearchProfile::historical_5_5,
                                      Rational threshold_percent = Rational(115),
                                      SearchLimits limits = {}) const;
private:
    Dataset dataset_;
    [[nodiscard]] const InputBucket& bucket(const BucketId& id) const;
};

} // namespace tradeup
