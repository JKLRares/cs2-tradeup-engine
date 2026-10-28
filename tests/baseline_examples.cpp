#include <array>
#include <cmath>
#include <iostream>
#include <numeric>
#include <string_view>

// Fixed arithmetic witnesses for the specification, not a search/evaluation engine.
int main() {
    int checks = 0;
    int failures = 0;
    const auto check = [&](const std::string_view name,
                           const long double actual,
                           const long double expected) {
        ++checks;
        if (std::abs(actual - expected) > 1e-12L) {
            ++failures;
            std::cerr << "FAIL: " << name << '\n';
        }
    };

    constexpr long double retained = 0.87L;
    // Two outcomes per collection, five inputs each. Each outcome has 5/20 mass.
    constexpr long double equal_ev = (5.0L * (10 + 30) + 5.0L * (20 + 40)) / 20.0L;
    check("equal counts: gross EV", equal_ev, 25.0L);
    check("return percentage includes returned capital", equal_ev * retained / 15 * 100,
          145.0L);
    // Six inputs with two outcomes and four with three: 24 tickets, not 20.
    constexpr long double unequal_ev = (6.0L * (10 + 30) + 4.0L * (20 + 40 + 60)) / 24;
    check("unequal counts: gross EV", unequal_ev, 30.0L);
    check("positive EV can include a losing outcome", 10 * retained, 8.7L);

    constexpr int six = 6;
    constexpr int four = 4;
    check("historical integer division witness", six / four, 1.0L);
    check("intended rational ratio witness", static_cast<long double>(six) / four, 1.5L);

    // Same 5/2/3 quantities; integer division by Min=2 changes relative weights.
    constexpr long double exact_ev = (5.0L * 40 + 2.0L * 100 + 3.0L * 160) / 20;
    constexpr long double truncated_ev = (2.0L * 40 + 1.0L * 100 + 1.0L * 160) / 8;
    check("rational quantity weights", exact_ev, 44.0L);
    check("truncated quantity weights", truncated_ev, 42.5L);

    constexpr std::array<long double, 6> six_outcomes{1, 2, 3, 4, 5, 6};
    const auto total = std::accumulate(six_outcomes.begin(), six_outcomes.end(), 0.0L);
    check("six separate modeled outcomes", total / six_outcomes.size(), 3.5L);

    if (failures != 0) {
        return 1;
    }
    std::cout << "PASS: " << checks << " numerical and language-semantics checks\n";
    return 0;
}
