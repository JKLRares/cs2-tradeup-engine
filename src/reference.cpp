#include "tradeup/reference.hpp"

#include <algorithm>
#include <charconv>
#include <functional>
#include <limits>
#include <numeric>
#include <utility>

namespace tradeup {
namespace {
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();

std::int64_t magnitude(const std::int64_t value) noexcept {
    return value < 0 ? -value : value; // All callers exclude INT64_MIN.
}
std::int64_t add(const std::int64_t a, const std::int64_t b) {
    if ((b > 0 && a > maximum - b) || (b < 0 && a < -maximum - b)) {
        throw std::overflow_error("rational addition exceeds signed-64-bit bounds");
    }
    return a + b;
}
std::int64_t multiply(const std::int64_t a, const std::int64_t b) {
    if (b != 0 && magnitude(a) > maximum / magnitude(b)) {
        throw std::overflow_error("rational multiplication exceeds signed-64-bit bounds");
    }
    return a * b;
}
std::int64_t unsigned_integer(const std::string_view text) {
    if (text.empty() || !std::all_of(text.begin(), text.end(), [](const char c) {
            return c >= '0' && c <= '9';
        })) {
        throw std::invalid_argument("expected unsigned decimal digits");
    }
    std::int64_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec == std::errc::result_out_of_range) {
        throw std::overflow_error("numeric token exceeds signed-64-bit bounds");
    }
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::invalid_argument("invalid numeric token");
    }
    return result;
}
// Euclidean comparison, with no potentially overflowing cross multiplication.
std::strong_ordering compare_positive(std::int64_t a, std::int64_t b,
                                      std::int64_t c, std::int64_t d) noexcept {
    bool inverted = false;
    for (;;) {
        const auto qa = a / b;
        const auto qc = c / d;
        if (qa != qc) {
            return inverted ? qc <=> qa : qa <=> qc;
        }
        const auto ra = a % b;
        const auto rc = c % d;
        if (ra == 0 || rc == 0) {
            if (ra == rc) { return std::strong_ordering::equal; }
            const bool less = ra == 0;
            return less != inverted ? std::strong_ordering::less : std::strong_ordering::greater;
        }
        a = b; b = ra; c = d; d = rc;
        inverted = !inverted;
    }
}
void token(const std::string_view value) {
    if (value.empty() || value.size() > 128 || !std::all_of(value.begin(), value.end(), [](const char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        })) {
        throw std::invalid_argument("identity tokens require 1..128 ASCII letters/digits/_.-");
    }
}
void validate_quote(const Quote& quote, const bool input) {
    if (quote.value < Rational(0) || (input && quote.value == Rational(0))) {
        throw std::invalid_argument("input prices must be positive; outcome prices nonnegative");
    }
    switch (quote.kind) {
    case QuoteKind::recorded: case QuoteKind::synthetic: case QuoteKind::unknown: break;
    case QuoteKind::minimum_derived: case QuoteKind::composite_estimate: case QuoteKind::ratio_estimate:
        if (quote.source.empty()) { throw std::invalid_argument("derived quotes require source provenance"); }
        break;
    default: throw std::invalid_argument("unknown quote-kind enum value");
    }
}
void append_limitation(std::vector<std::string>& limitations, const Quote& quote,
                       const std::string& location) {
    if (quote.kind == QuoteKind::composite_estimate || quote.kind == QuoteKind::ratio_estimate) {
        limitations.push_back("Estimated/composite valuation at " + location + ": " + quote.source);
    } else if (quote.kind == QuoteKind::unknown) {
        limitations.push_back("Unknown quote provenance at " + location);
    } else if (quote.kind == QuoteKind::synthetic) {
        limitations.push_back("Synthetic test quote at " + location);
    }
}
std::int64_t count(const std::size_t size) {
    if (size > static_cast<std::uint64_t>(maximum)) {
        throw std::overflow_error("outcome count exceeds arithmetic range");
    }
    return static_cast<std::int64_t>(size);
}
std::pair<int, int> split(const SearchProfile profile) {
    switch (profile) {
    case SearchProfile::historical_5_5: return {5, 5};
    case SearchProfile::historical_6_4: return {6, 4};
    default: throw std::invalid_argument("unsupported search profile");
    }
}
} // namespace

Rational::Rational(const std::int64_t numerator, const std::int64_t denominator)
    : numerator_(numerator), denominator_(denominator) {
    if (numerator == std::numeric_limits<std::int64_t>::min() || denominator <= 0) {
        throw std::invalid_argument("rational requires numerator >= -INT64_MAX and denominator > 0");
    }
    const auto divisor = std::gcd(magnitude(numerator), denominator);
    numerator_ /= divisor;
    denominator_ /= divisor;
}
Rational Rational::parse(std::string_view text) {
    if (text.empty() || text.size() > 128) { throw std::invalid_argument("invalid numeric token length"); }
    const bool negative = text.front() == '-';
    if (negative) { text.remove_prefix(1); }
    Rational result;
    const auto slash = text.find('/');
    const auto dot = text.find('.');
    if (slash != std::string_view::npos) {
        result = Rational(unsigned_integer(text.substr(0, slash)), unsigned_integer(text.substr(slash + 1)));
    } else if (dot != std::string_view::npos) {
        const auto whole = unsigned_integer(text.substr(0, dot));
        const auto digits = text.substr(dot + 1);
        if (digits.empty() || digits.size() > 18) { throw std::invalid_argument("expected 1..18 decimal places"); }
        const auto fraction = unsigned_integer(digits);
        std::int64_t denominator = 1;
        for (std::size_t index = 0; index < digits.size(); ++index) { denominator = multiply(denominator, 10); }
        result = Rational(whole) + Rational(fraction, denominator);
    } else {
        result = Rational(unsigned_integer(text));
    }
    return negative ? Rational(-result.numerator_, result.denominator_) : result;
}
std::string Rational::str() const {
    return std::to_string(numerator_) + "/" + std::to_string(denominator_);
}
Rational operator+(const Rational left, const Rational right) {
    const auto common = std::gcd(left.denominator_, right.denominator_);
    const auto numerator = add(multiply(left.numerator_, right.denominator_ / common),
                               multiply(right.numerator_, left.denominator_ / common));
    const auto reduce = std::gcd(magnitude(numerator), common);
    return {numerator / reduce,
            multiply(left.denominator_ / common, right.denominator_ / reduce)};
}
Rational operator-(const Rational left, const Rational right) {
    return left + Rational(-right.numerator_, right.denominator_);
}
Rational operator*(const Rational left, const Rational right) {
    const auto cancel_left = std::gcd(magnitude(left.numerator_), right.denominator_);
    const auto cancel_right = std::gcd(magnitude(right.numerator_), left.denominator_);
    return {multiply(left.numerator_ / cancel_left, right.numerator_ / cancel_right),
            multiply(left.denominator_ / cancel_right, right.denominator_ / cancel_left)};
}
Rational operator/(const Rational left, const Rational right) {
    if (right.numerator_ == 0) { throw std::invalid_argument("division by zero"); }
    const auto signed_denominator = right.numerator_ < 0 ? -right.denominator_ : right.denominator_;
    return left * Rational(signed_denominator, magnitude(right.numerator_));
}
std::strong_ordering operator<=>(const Rational left, const Rational right) noexcept {
    if ((left.numerator_ < 0) != (right.numerator_ < 0)) { return left.numerator_ <=> right.numerator_; }
    if (left.numerator_ < 0) {
        return compare_positive(magnitude(right.numerator_), right.denominator_,
                                magnitude(left.numerator_), left.denominator_);
    }
    return compare_positive(left.numerator_, left.denominator_, right.numerator_, right.denominator_);
}

std::string Contract::serialize() const {
    token(snapshot_id); token(scenario_id);
    std::string result = "contract-v1|" + snapshot_id + "|" + scenario_id + "|";
    int total = 0;
    std::string previous;
    for (const auto& allocation : allocations) {
        token(allocation.bucket.value);
        if (allocation.quantity <= 0 || allocation.quantity > 10 ||
            (!previous.empty() && allocation.bucket.value <= previous)) {
            throw std::invalid_argument("contract allocations must be positive, distinct, and sorted");
        }
        total += allocation.quantity; // At most ten entries can be accepted before total > 10.
        if (total > 10) { throw std::invalid_argument("contract total exceeds ten"); }
        if (!previous.empty()) { result += ','; }
        result += allocation.bucket.value + ":" + std::to_string(allocation.quantity);
        previous = allocation.bucket.value;
    }
    if (total != 10) { throw std::invalid_argument("contract must contain exactly ten inputs"); }
    return result;
}
bool Evaluation::qualifies(const Rational threshold_percent) const {
    if (threshold_percent < Rational(0)) { throw std::invalid_argument("return threshold must be nonnegative"); }
    return return_percent > threshold_percent;
}
std::string Evaluation::serialize_identity() const {
    token(policy.probability); token(policy.output_valuation); token(policy.fee);
    return "evaluation-v1|" + contract.serialize() + "|" + policy.probability + "|" +
           policy.output_valuation + "|" + policy.fee;
}
std::string_view profile_id(const SearchProfile profile) {
    switch (profile) {
    case SearchProfile::historical_5_5: return "historical-5-5-up-to-5-buckets-v1";
    case SearchProfile::historical_6_4: return "historical-6-4-up-to-5-buckets-v1";
    default: throw std::invalid_argument("unsupported search profile");
    }
}

ReferenceEngine::ReferenceEngine(Dataset dataset) : dataset_(std::move(dataset)) {
    token(dataset_.snapshot_id); token(dataset_.scenario_id); token(dataset_.output_valuation_policy_id);
    if (dataset_.price_unit.empty() || dataset_.collections.empty() || dataset_.buckets.empty()) {
        throw std::invalid_argument("dataset requires price unit, collections, and input buckets");
    }
    auto& collections = dataset_.collections;
    std::sort(collections.begin(), collections.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    for (std::size_t index = 0; index < collections.size(); ++index) {
        auto& collection = collections[index];
        token(collection.id.value);
        if ((index > 0 && collections[index - 1].id == collection.id) ||
            collection.outcomes.empty() || !collection.declared_outcomes_complete) {
            throw std::invalid_argument("duplicate collection or incomplete/empty declared outcome set");
        }
        std::sort(collection.outcomes.begin(), collection.outcomes.end(),
                  [](const auto& a, const auto& b) { return a.id < b.id; });
        for (std::size_t outcome_index = 0; outcome_index < collection.outcomes.size(); ++outcome_index) {
            const auto& outcome = collection.outcomes[outcome_index];
            token(outcome.id.value);
            if (!outcome.price || (outcome_index > 0 && collection.outcomes[outcome_index - 1].id == outcome.id)) {
                throw std::invalid_argument("missing price or duplicate outcome identity");
            }
            validate_quote(*outcome.price, false);
        }
    }
    auto& buckets = dataset_.buckets;
    std::sort(buckets.begin(), buckets.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    for (std::size_t index = 0; index < buckets.size(); ++index) {
        const auto& input = buckets[index];
        token(input.id.value); token(input.collection.value);
        if ((index > 0 && buckets[index - 1].id == input.id) ||
            (input.wear != WearCategory::lower && input.wear != WearCategory::higher)) {
            throw std::invalid_argument("duplicate bucket or invalid wear category");
        }
        if (std::none_of(collections.begin(), collections.end(), [&](const auto& c) { return c.id == input.collection; })) {
            throw std::invalid_argument("bucket refers to an unknown collection");
        }
        validate_quote(input.unit_cost, true);
    }
}
const InputBucket& ReferenceEngine::bucket(const BucketId& id) const {
    const auto found = std::find_if(dataset_.buckets.begin(), dataset_.buckets.end(),
                                    [&](const auto& input) { return input.id == id; });
    if (found == dataset_.buckets.end()) { throw std::invalid_argument("unknown bucket identity"); }
    return *found;
}
Contract ReferenceEngine::canonical_contract(const std::span<const Allocation> allocations) const {
    if (allocations.empty() || allocations.size() > 10) { throw std::invalid_argument("expected 1..10 allocation entries"); }
    Contract result{dataset_.snapshot_id, dataset_.scenario_id, {allocations.begin(), allocations.end()}};
    int total = 0;
    for (const auto& allocation : result.allocations) {
        (void)bucket(allocation.bucket);
        if (allocation.quantity <= 0 || allocation.quantity > 10) { throw std::invalid_argument("invalid quantity"); }
        total += allocation.quantity;
        if (total > 10) { throw std::invalid_argument("contract total exceeds ten"); }
    }
    if (total != 10) { throw std::invalid_argument("contract total must equal ten"); }
    std::sort(result.allocations.begin(), result.allocations.end(), [](const auto& a, const auto& b) { return a.bucket < b.bucket; });
    std::vector<Allocation> merged;
    for (const auto& allocation : result.allocations) {
        if (!merged.empty() && merged.back().bucket == allocation.bucket) {
            merged.back().quantity += allocation.quantity;
        } else { merged.push_back(allocation); }
    }
    result.allocations = std::move(merged);
    return result;
}
Evaluation ReferenceEngine::evaluate(const Contract& contract) const {
    if (contract.snapshot_id != dataset_.snapshot_id || contract.scenario_id != dataset_.scenario_id) {
        throw std::invalid_argument("contract snapshot/scenario does not match dataset");
    }
    Evaluation result;
    result.contract = canonical_contract(contract.allocations);
    result.policy.output_valuation = dataset_.output_valuation_policy_id;
    result.limitations = dataset_.limitations;
    result.limitations.push_back("Modeled quote-basket result; exact float feasibility and listing supply are unverified.");
    std::vector<std::int64_t> quantities(dataset_.collections.size(), 0);
    for (const auto& allocation : result.contract.allocations) {
        const auto& input = bucket(allocation.bucket);
        result.cost = result.cost + Rational(allocation.quantity) * input.unit_cost.value;
        append_limitation(result.limitations, input.unit_cost, "bucket " + input.id.value);
        for (std::size_t index = 0; index < dataset_.collections.size(); ++index) {
            if (dataset_.collections[index].id == input.collection) { quantities[index] += allocation.quantity; }
        }
    }
    std::int64_t tickets = 0;
    for (std::size_t index = 0; index < quantities.size(); ++index) {
        tickets = add(tickets, multiply(quantities[index], count(dataset_.collections[index].outcomes.size())));
    }
    std::optional<Rational> worst;
    for (std::size_t index = 0; index < quantities.size(); ++index) {
        if (quantities[index] == 0) { continue; }
        const auto& collection = dataset_.collections[index];
        const Rational probability(quantities[index], tickets);
        for (const auto& outcome : collection.outcomes) {
            const auto& price = *outcome.price;
            const auto receipt = price.value * Rational(87, 100);
            result.outcomes.push_back({collection.id, outcome.id, probability, price, receipt});
            result.gross_ev = result.gross_ev + probability * price.value;
            if (!worst || receipt < *worst) { worst = receipt; }
            append_limitation(result.limitations, price, collection.id.value + "/" + outcome.id.value);
        }
    }
    result.net_ev = result.gross_ev * Rational(87, 100);
    result.expected_profit = result.net_ev - result.cost;
    result.return_percent = (result.net_ev / result.cost) * Rational(100);
    result.roi_percent = result.return_percent - Rational(100);
    result.worst_net = *worst;
    return result;
}
Enumeration ReferenceEngine::enumerate(const SearchProfile profile, const SearchLimits limits) const {
    const auto [lower_total, higher_total] = split(profile);
    if (dataset_.buckets.size() > maximum_enumeration_buckets || limits.max_nodes == 0 || limits.max_contracts == 0) {
        throw std::invalid_argument("reference enumeration requires <=12 buckets and positive resource budgets");
    }
    Enumeration result;
    std::vector<Allocation> active;
    std::function<void(std::size_t, int, int)> visit = [&](const std::size_t index, const int lower, const int higher) {
        if (result.nodes_visited >= limits.max_nodes) { throw SearchLimitExceeded("enumeration node budget exhausted; no complete result"); }
        ++result.nodes_visited;
        if (lower == 0 && higher == 0) {
            if (active.size() >= 2 && active.size() <= 5) {
                if (result.contracts.size() >= limits.max_contracts) { throw SearchLimitExceeded("contract budget exhausted; no complete result"); }
                result.contracts.push_back({dataset_.snapshot_id, dataset_.scenario_id, active});
            }
            return;
        }
        if (index == dataset_.buckets.size() || active.size() == 5) { return; }
        visit(index + 1, lower, higher); // Do not use this bucket.
        const auto& input = dataset_.buckets[index];
        const bool is_lower = input.wear == WearCategory::lower;
        const int available = is_lower ? lower : higher;
        for (int quantity = 1; quantity <= available; ++quantity) {
            active.push_back({input.id, quantity});
            visit(index + 1, lower - (is_lower ? quantity : 0), higher - (is_lower ? 0 : quantity));
            active.pop_back();
        }
    };
    visit(0, lower_total, higher_total);
    return result;
}
SearchResult ReferenceEngine::search(const SearchProfile profile, const Rational threshold_percent,
                                    const SearchLimits limits) const {
    if (threshold_percent < Rational(0)) { throw std::invalid_argument("return threshold must be nonnegative"); }
    const auto enumeration = enumerate(profile, limits);
    SearchResult result;
    result.nodes_visited = enumeration.nodes_visited;
    for (const auto& contract : enumeration.contracts) {
        auto evaluation = evaluate(contract);
        ++result.candidates_evaluated;
        if (evaluation.qualifies(threshold_percent)) { result.qualifying.push_back(std::move(evaluation)); }
    }
    return result;
}
} // namespace tradeup
