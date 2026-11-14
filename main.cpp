#include <iomanip>
#include <iostream>
#include "include/Money.hpp"
#include "include/MarketDataGateway.hpp"

void runSimulation(lob::IMarketDataGateway& gateway, uint64_t maxPrintEvents = 20) {
    uint64_t processedEvents = 0;

    std::cout << std::fixed << std::setprecision(6);

    while (true) {
        lob::DomainEvent event = gateway.readNextEvent();

        if (event.type == lob::EventType::END_OF_STREAM) {
            break;
        }

        if (event.type == lob::EventType::NONE) {
            continue;
        }

        ++processedEvents;

        if (processedEvents <= maxPrintEvents) {
            std::cout << "#" << std::setw(2) << processedEvents << " " << event << std::endl;
        }
    }

    if (processedEvents > maxPrintEvents) {
        std::cout << "... (" << (processedEvents - maxPrintEvents)
                  << " more events read from file)" << std::endl;
    }
    std::cout << "Simulation finished. Total parsed events: " << processedEvents << std::endl;
}

int main() {
    using lob::Money;

    std::cout << "=== Money Value Object Tests ===" << std::endl;

    Money m1 = Money::fromDecimal(10.50, "PLN");
    Money m2 = Money::fromDecimal(2.25, "PLN");

    std::cout << "m1: " << m1 << std::endl;
    std::cout << "m2: " << m2 << std::endl;
    std::cout << "add(): " << m1.add(m2) << std::endl;
    std::cout << "subtract(): " << m1.subtract(m2) << std::endl;
    std::cout << "multiply(1.5): " << m1.multiply(1.5) << std::endl;
    std::cout << "greaterThan(m2): " << std::boolalpha << m1.greaterThan(m2) << std::endl;
    std::cout << "compareTo(m2): " << m1.compareTo(m2) << std::endl;

    std::cout << "\n--- Exact Allocation Test ({7, 3}) ---" << std::endl;
    Money fiveCents(5, "PLN");
    std::cout << "Base amount: " << fiveCents << std::endl;

    auto parts = fiveCents.allocate({7, 3});
    std::cout << "Part 1 (70%): " << parts[0] << std::endl;
    std::cout << "Part 2 (30%): " << parts[1] << std::endl;
    std::cout << "Total: " << parts[0].add(parts[1]) << " (exact, zero rounding loss)" << std::endl;

    std::cout << "\n--- Currency Mismatch Test ---" << std::endl;
    try {
        Money usd(100, "USD");
        Money invalid = m1.add(usd);
        (void)invalid;
    } catch (const std::exception& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    std::cout << "\n=== LOBSTER MarketDataGateway Test ===" << std::endl;
    const std::string lobsterPath =
        "../LOBSTER_SampleFile_AAPL_2012-06-21_1/AAPL_2012-06-21_34200000_57600000_message_1.csv";

    try {
        lob::MarketDataGateway gateway(lobsterPath);
        std::cout << "Reading from: " << gateway.filePath() << std::endl;
        runSimulation(gateway);
    } catch (const std::exception& e) {
        std::cerr << "Gateway error: " << e.what() << std::endl;
    }

    return 0;
}
