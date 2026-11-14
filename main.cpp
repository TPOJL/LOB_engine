#include <iostream>
#include "Money.hpp"
#include "MarketDataGateway.hpp"

// Simulation loop sketch: consumes DomainEvents via Separated Interface (IMarketDataGateway)
void runSimulation(lob::IMarketDataGateway& gateway) {
    uint64_t processedEvents = 0;

    while (true) {
        lob::DomainEvent event = gateway.readNextEvent();

        if (event.type == lob::EventType::END_OF_STREAM) {
            break;
        }

        if (event.type == lob::EventType::NONE) {
            continue;
        }

        if (event.type == lob::EventType::BUY) {
            // TODO: book.addBid(event.orderId, event.price, event.volume, event.timestamp);
        } else if (event.type == lob::EventType::SELL) {
            // TODO: book.addAsk(event.orderId, event.price, event.volume, event.timestamp);
        }

        // TODO: if (snapshots.shouldCapture(event.timestamp)) snapshots.capture(book, event.timestamp);
        ++processedEvents;
    }

    std::cout << "Simulation finished. Processed events: " << processedEvents << std::endl;
}

int main() {
    using lob::Money;

    std::cout << "=== Money Value Object Tests ===" << std::endl;

    Money m1 = Money::from_decimal(10.50, "PLN");
    Money m2 = Money::from_decimal(2.25, "PLN");

    std::cout << "m1: " << m1 << std::endl;
    std::cout << "m2: " << m2 << std::endl;
    std::cout << "add(): " << m1.add(m2) << std::endl;
    std::cout << "subtract(): " << m1.subtract(m2) << std::endl;
    std::cout << "multiply(1.5): " << m1.multiply(1.5) << std::endl;
    std::cout << "greaterThan(m2): " << std::boolalpha << m1.greaterThan(m2) << std::endl;
    std::cout << "compareTo(m2): " << m1.compareTo(m2) << std::endl;

    // Test proportional allocation without remainder loss (5 minor units split 70% / 30%)
    std::cout << "\n--- Exact Allocation Test ({7, 3}) ---" << std::endl;
    Money five_cents(5, "PLN");
    std::cout << "Base amount: " << five_cents << std::endl;

    auto parts = five_cents.allocate({7, 3});
    std::cout << "Part 1 (70%): " << parts[0] << std::endl;
    std::cout << "Part 2 (30%): " << parts[1] << std::endl;
    std::cout << "Total: " << (parts[0] + parts[1]) << " (exact, zero rounding loss)" << std::endl;

    // Test currency validation
    std::cout << "\n--- Currency Mismatch Test ---" << std::endl;
    try {
        Money usd(100, "USD");
        Money invalid = m1.add(usd);
        (void)invalid;
    } catch (const std::exception& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    // Gateway + Simulation loop stub
    std::cout << "\n--- Gateway Simulation Stub ---" << std::endl;
    lob::MarketDataGateway gateway("data/sample_events.csv");
    std::cout << "Gateway file: " << gateway.filePath() << std::endl;
    runSimulation(gateway);

    return 0;
}
