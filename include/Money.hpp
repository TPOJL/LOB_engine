#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <utility>

namespace lob {

/**
 * @brief Exception thrown when attempting operations between mismatched currencies.
 */
class CurrencyMismatchException : public std::invalid_argument {
public:
    CurrencyMismatchException(const std::string& curr1, const std::string& curr2)
        : std::invalid_argument("Currency mismatch: cannot operate on " + curr1 + " and " + curr2) {}
};

/**
 * @brief Money Value Object implementation based on Martin Fowler's pattern.
 */
class Money {
private:
    int64_t m_amount;
    std::string m_currency;

    void checkCurrency(const Money& other) const {
        if (m_currency != other.m_currency) {
            throw CurrencyMismatchException(m_currency, other.m_currency);
        }
    }

public:
    Money() : m_amount(0), m_currency("PLN") {}

    Money(int64_t amount, std::string currency)
        : m_amount(amount), m_currency(std::move(currency)) {}

    [[nodiscard]] static Money fromDecimal(double amountMajor, const std::string& currency = "PLN") {
        int64_t cents = static_cast<int64_t>(std::round(amountMajor * 100.0));
        return Money(cents, currency);
    }

    [[nodiscard]] int64_t amount() const noexcept { return m_amount; }
    [[nodiscard]] const std::string& currency() const noexcept { return m_currency; }

    [[nodiscard]] Money add(const Money& other) const {
        checkCurrency(other);
        return Money(m_amount + other.m_amount, m_currency);
    }

    [[nodiscard]] Money subtract(const Money& other) const {
        checkCurrency(other);
        return Money(m_amount - other.m_amount, m_currency);
    }

    [[nodiscard]] Money multiply(double factor) const {
        int64_t newAmount = static_cast<int64_t>(std::round(static_cast<double>(m_amount) * factor));
        return Money(newAmount, m_currency);
    }

    [[nodiscard]] Money multiplay(double factor) const {
        return multiply(factor);
    }

    [[nodiscard]] std::vector<Money> allocate(const std::vector<int>& ratios) const {
        if (ratios.empty()) {
            throw std::invalid_argument("Ratios vector cannot be empty");
        }

        int64_t totalRatio = 0;
        for (int r : ratios) {
            if (r < 0) {
                throw std::invalid_argument("Ratio cannot be negative");
            }
            totalRatio += r;
        }

        if (totalRatio == 0) {
            throw std::invalid_argument("Sum of ratios must be greater than zero");
        }

        int64_t remainder = m_amount;
        std::vector<Money> results;
        results.reserve(ratios.size());

        for (int r : ratios) {
            int64_t share = (m_amount * r) / totalRatio;
            results.emplace_back(share, m_currency);
            remainder -= share;
        }

        int64_t step = (remainder >= 0) ? 1 : -1;
        for (size_t i = 0; remainder != 0 && i < results.size(); ++i) {
            results[i].m_amount += step;
            remainder -= step;
        }

        return results;
    }

    [[nodiscard]] int compareTo(const Money& other) const {
        checkCurrency(other);
        if (m_amount < other.m_amount) return -1;
        if (m_amount > other.m_amount) return 1;
        return 0;
    }

    [[nodiscard]] bool greaterThan(const Money& other) const {
        return compareTo(other) > 0;
    }

    [[nodiscard]] bool greaterThen(const Money& other) const {
        return greaterThan(other);
    }

    // Value Object equality and ordering operators (required for std::map<Money, uint32_t>)
    bool operator==(const Money& other) const noexcept {
        return m_amount == other.m_amount && m_currency == other.m_currency;
    }

    bool operator!=(const Money& other) const noexcept {
        return !(*this == other);
    }

    bool operator<(const Money& other) const { return compareTo(other) < 0; }

    friend std::ostream& operator<<(std::ostream& os, const Money& m) {
        int64_t absAmount = std::abs(m.m_amount);
        int64_t major = absAmount / 100;
        int64_t cents = absAmount % 100;

        if (m.m_amount < 0) {
            os << '-';
        }
        os << major << '.' << (cents < 10 ? "0" : "") << cents << " " << m.m_currency;
        return os;
    }
};

} // namespace lob
