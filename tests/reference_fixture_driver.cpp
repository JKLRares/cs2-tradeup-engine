// Private test protocol for Python differential verification; not an application CLI.
#include "tradeup/reference.hpp"

#include <iostream>
#include <string>
#include <utility>

using namespace tradeup;
namespace {
template<class T> T read() {
    T value{};
    if (!(std::cin >> value)) { throw std::invalid_argument("truncated or malformed fixture stream"); }
    return value;
}
std::size_t size() {
    const auto value = read<int>();
    if (value < 0 || value > 10'000) { throw std::invalid_argument("fixture count outside 0..10000"); }
    return static_cast<std::size_t>(value);
}
Dataset dataset() {
    Dataset data;
    data.snapshot_id = read<std::string>(); data.scenario_id = read<std::string>();
    data.output_valuation_policy_id = "fixture-valuations-v1"; data.price_unit = "fixture-unit";
    const auto collections = size(); const auto buckets = size();
    for (std::size_t index = 0; index < collections; ++index) {
        Collection collection; collection.id = CollectionId{read<std::string>()};
        const auto outcomes = size();
        for (std::size_t outcome_index = 0; outcome_index < outcomes; ++outcome_index) {
            const auto id = read<std::string>(); const auto price = Rational::parse(read<std::string>());
            collection.outcomes.push_back({OutcomeId{id}, Quote{price, QuoteKind::synthetic, "fixture"}});
        }
        data.collections.push_back(std::move(collection));
    }
    for (std::size_t index = 0; index < buckets; ++index) {
        const auto id = read<std::string>(); const auto collection = read<std::string>();
        const auto wear = read<std::string>(); const auto price = Rational::parse(read<std::string>());
        if (wear != "L" && wear != "H") { throw std::invalid_argument("invalid fixture wear"); }
        data.buckets.push_back({BucketId{id}, CollectionId{collection}, wear == "L" ? WearCategory::lower : WearCategory::higher,
                                Quote{price, QuoteKind::synthetic, "fixture"}});
    }
    return data;
}
void fields(const Evaluation& value) {
    std::cout << '\t' << value.cost.str() << '\t' << value.gross_ev.str() << '\t' << value.net_ev.str()
              << '\t' << value.return_percent.str() << '\t' << value.roi_percent.str()
              << '\t' << value.expected_profit.str() << '\t' << value.worst_net.str();
}
} // namespace
int main() {
    std::string command;
    while (std::cin >> command) {
        try {
            if (command == "calc") {
                const auto operation = read<std::string>();
                const auto left_text = read<std::string>(); const auto right_text = read<std::string>();
                const auto a = Rational::parse(left_text); const auto b = Rational::parse(right_text);
                Rational value;
                if (operation == "+") { value = a + b; }
                else if (operation == "-") { value = a - b; }
                else if (operation == "*") { value = a * b; }
                else if (operation == "/") { value = a / b; }
                else if (operation == "cmp") { value = Rational(a < b ? -1 : (a > b ? 1 : 0)); }
                else { throw std::invalid_argument("unknown fixture arithmetic operation"); }
                std::cout << "OK\t" << value.str() << '\n';
            } else if (command == "eval" || command == "enum55" || command == "enum64") {
                const ReferenceEngine engine(dataset());
                if (command == "eval") {
                    std::vector<Allocation> allocations;
                    const auto entries = size();
                    for (std::size_t index = 0; index < entries; ++index) {
                        const auto id = read<std::string>(); const auto quantity = read<int>();
                        allocations.push_back({BucketId{id}, quantity});
                    }
                    const auto value = engine.evaluate(engine.canonical_contract(allocations));
                    std::cout << "E"; fields(value);
                    std::cout << '\t' << value.qualifies() << '\t' << value.contract.serialize() << '\n';
                    for (const auto& outcome : value.outcomes) {
                        std::cout << "P\t" << outcome.collection.value << '\t' << outcome.outcome.value
                                  << '\t' << outcome.probability.str() << '\t' << outcome.gross_price.value.str()
                                  << '\t' << outcome.net_receipt.str() << '\n';
                    }
                } else {
                    const auto threshold = Rational::parse(read<std::string>());
                    const SearchLimits limits{size(), size()};
                    const auto profile = command == "enum55" ? SearchProfile::historical_5_5 : SearchProfile::historical_6_4;
                    const auto enumeration = engine.enumerate(profile, limits);
                    const auto result = engine.search(profile, threshold, limits);
                    std::cout << "S\t" << result.candidates_evaluated << '\t' << result.qualifying.size()
                              << '\t' << result.nodes_visited << '\n';
                    for (const auto& contract : enumeration.contracts) { std::cout << "C\t" << contract.serialize() << '\n'; }
                    for (const auto& value : result.qualifying) {
                        std::cout << "Q\t" << value.contract.serialize(); fields(value); std::cout << '\n';
                    }
                }
                std::cout << "END\n";
                std::cin >> std::ws;
                if (!std::cin.eof()) { throw std::invalid_argument("trailing fixture tokens"); }
            } else { throw std::invalid_argument("unknown fixture command"); }
        } catch (const SearchLimitExceeded& error) { std::cout << "ERR\tlimit\t" << error.what() << '\n'; return 2; }
        catch (const std::overflow_error& error) { std::cout << "ERR\toverflow\t" << error.what() << '\n'; }
        catch (const std::exception& error) { std::cout << "ERR\tinvalid\t" << error.what() << '\n'; return 2; }
    }
    return 0;
}
