#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include "include/Money.hpp"
#include "include/MarketDataGateway.hpp"
#include "include/LimitOrderBook.hpp"
#include "include/BookSnapshotMapper.hpp"

void runOrderBookUnitTests() {
    using namespace lob;
    std::cout << "\n=== LimitOrderBook Matching & Service Stub Tests ===" << std::endl;

    TimePoint t0{};
    StubMarketDataGateway stubGateway({
        DomainEvent{t0, 1, Money(10000, "USD"), 20, EventType::BUY},
        DomainEvent{t0, 2, Money(10000, "USD"), 30, EventType::BUY},
        DomainEvent{t0, 3, Money(10000, "USD"), 15, EventType::SELL},   // Partial fill of #1 (20 -> 5)
        DomainEvent{t0, 4, Money(9950, "USD"),  40, EventType::SELL},   // Sweeps #1 (5) & #2 (30), rests 5 @ 99.50
        DomainEvent{t0, 4, Money(9950, "USD"),  2,  EventType::MODIFY}, // Reduces #4 volume (5 -> 3)
        DomainEvent{t0, 4, Money(9980, "USD"),  10, EventType::MODIFY}  // Moves #4 price to 99.80 (vol 10)
    });

    LimitOrderBook book;
    IMarketDataGateway& feed = stubGateway;
    while (true) {
        DomainEvent ev = feed.readNextEvent();
        if (ev.isEndOfStream()) break;
        book.applyEvent(ev);
    }

    assert(book.bids().empty());
    assert(book.asks().size() == 1);
    assert(book.asks().begin()->first == Money(9980, "USD"));
    assert(book.asks().begin()->second.totalVolume == 10);
    assert(book.asks().begin()->second.orders.front().volume == 10);
    std::cout << "FIFO, Partial Fill, Full Sweep & Modify via Service Stub: PASSED" << std::endl;

    // Event Sourcing state reconstruction (replay) test
    LimitOrderBook replayedBook;
    replayedBook.replay(book.eventLog());
    assert(replayedBook.asks().size() == 1);
    assert(replayedBook.asks().begin()->first == Money(9980, "USD"));
    assert(replayedBook.asks().begin()->second.totalVolume == 10);
    std::cout << "Event Sourcing State Replay test: PASSED" << std::endl;

    // Snapshot Mapper + DTO test (Bid: 99.20 vol 30, Ask: 99.80 vol 10 -> midPrice = 99.50, imbalanceX = (30-10)/40 = 0.5)
    book.applyEvent(DomainEvent{t0, 5, Money(9920, "USD"), 30, EventType::BUY});
    BookSnapshotMapper mapper;
    const ISnapshotMapper& snapshotInterface = mapper;
    BookSnapshotDTO snap = snapshotInterface.createSnapshot(book, t0);
    assert(std::abs(snap.midPrice - 99.50) < 1e-9);
    assert(std::abs(snap.imbalanceX - 0.5) < 1e-9);
    std::cout << "BookSnapshotDTO & Mapper test: PASSED (" << snap << ")" << std::endl;

    // Cancel existing & non-existent orders
    book.applyEvent(DomainEvent{t0, 999999, Money(9980, "USD"), 10, EventType::CANCEL});
    assert(book.activeOrdersCount() == 2);
    book.applyEvent(DomainEvent{t0, 4, Money(9980, "USD"), 10, EventType::CANCEL});
    book.applyEvent(DomainEvent{t0, 5, Money(9920, "USD"), 30, EventType::CANCEL});
    assert(book.asks().empty());
    assert(book.bids().empty());
    assert(book.activeOrdersCount() == 0);
    std::cout << "Order Cancel & Empty Book cleanup test: PASSED" << std::endl;
}

void runSimulation(lob::IMarketDataGateway& gateway, lob::LimitOrderBook& book, const lob::ISnapshotMapper& mapper, uint64_t maxPrintEvents = 10) {
    uint64_t processedEvents = 0;
    std::vector<lob::BookSnapshotDTO> snapshots;

    std::cout << std::fixed << std::setprecision(6);
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        lob::DomainEvent event = gateway.readNextEvent();

        if (event.isEndOfStream()) break;
        if (event.isNull()) continue;

        ++processedEvents;
        bool shouldPrint = (processedEvents <= maxPrintEvents);

        if (shouldPrint) {
            std::cout << "#" << std::setw(2) << processedEvents << " " << event << '\n';
        }

        book.setLogFills(shouldPrint);
        book.applyEvent(event);

        // Capture a snapshot every 100 events when both Bid and Ask sides are present
        if (processedEvents % 100 == 0 && book.hasBothSides()) {
            snapshots.push_back(mapper.createSnapshot(book, event.timestamp));
        }
    }

    mapper.populateFutureReturns(snapshots, 1);

    auto endTime = std::chrono::steady_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (processedEvents > maxPrintEvents) {
        std::cout << "... (" << (processedEvents - maxPrintEvents) << " more events processed)" << std::endl;
    }

    std::cout << std::setprecision(2);
    std::cout << "Simulation finished in " << elapsedMs << " ms | Total logged events: " << book.eventLog().size()
              << " | Captured snapshots: " << snapshots.size()
              << " | Active resting orders: " << book.activeOrdersCount() << std::endl;

    if (!book.bids().empty() && !book.asks().empty()) {
        std::cout << "Final Best Bid: " << book.bids().begin()->first
                  << " (vol: " << book.bids().begin()->second.totalVolume << ")"
                  << " | Final Best Ask: " << book.asks().begin()->first
                  << " (vol: " << book.asks().begin()->second.totalVolume << ")" << std::endl;
    }

    if (snapshots.size() >= 100) {
        std::cout << std::setprecision(4);
        std::cout << "\n--- Sample Captured BookSnapshotDTOs ---" << std::endl;
        for (size_t i = 0; i < 10; ++i) {
            std::cout << "#" << (i + 1) << " " << snapshots[i] << std::endl;
        }
    }
}

int main() {
    std::ios::sync_with_stdio(false);
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

    runOrderBookUnitTests();

    std::cout << "\n=== LOBSTER MarketDataGateway + LimitOrderBook Simulation ===" << std::endl;
    const std::string lobsterPath = "../LOBSTER_SampleFile_AAPL_2012-06-21_1/AAPL_2012-06-21_34200000_57600000_message_1.csv";

    try {
        lob::MarketDataGateway gateway(lobsterPath);
        lob::LimitOrderBook book;
        lob::BookSnapshotMapper snapshotMapper;
        std::cout << "Reading from: " << gateway.filePath() << std::endl;
        runSimulation(gateway, book, snapshotMapper);
    } catch (const std::exception& e) {
        std::cerr << "Gateway error: " << e.what() << std::endl;
    }

    return 0;
}
