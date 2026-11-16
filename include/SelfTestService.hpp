#pragma once

#include <cassert>
#include <cmath>
#include <iostream>
#include "BookSnapshotMapper.hpp"
#include "MarketDataGateway.hpp"
#include "MarketSimulationService.hpp"
#include "Money.hpp"

namespace lob {

/**
 * @brief Diagnostic Service Layer encapsulating automated self-tests for domain invariants.
 * Uses StubMarketDataGateway (Service Stub) to verify Money, LimitOrderBook, Event Sourcing, and Snapshot Mapping.
 */
class SelfTestService {
public:
    static void runAll() {
        std::cout << "=== Money Value Object Tests ===" << std::endl;

        Money m1 = Money::fromDecimal(10.50, "PLN");
        Money m2 = Money::fromDecimal(2.25, "PLN");
        assert(m1.add(m2) == Money::fromDecimal(12.75, "PLN"));
        assert(m1.subtract(m2) == Money::fromDecimal(8.25, "PLN"));
        assert(m1.multiply(1.5) == Money::fromDecimal(15.75, "PLN"));
        assert(m1.greaterThan(m2));

        auto parts = Money(5, "PLN").allocate({7, 3});
        assert(parts[0] == Money(4, "PLN") && parts[1] == Money(1, "PLN"));
        std::cout << "Money Value Object & Exact Allocation tests: PASSED" << std::endl;

        std::cout << "\n=== LimitOrderBook, Service Stub & Snapshot Mapper Tests ===" << std::endl;
        TimePoint t0{};
        StubMarketDataGateway stubGateway({
            DomainEvent{t0, 1, Money(10000, "USD"), 20, EventType::BUY},
            DomainEvent{t0, 2, Money(10000, "USD"), 30, EventType::BUY},
            DomainEvent{t0, 3, Money(10000, "USD"), 15, EventType::SELL},   // Partial fill of #1 (20 -> 5)
            DomainEvent{t0, 4, Money(9950, "USD"),  40, EventType::SELL},   // Sweeps #1 (5) & #2 (30), rests 5 @ 99.50
            DomainEvent{t0, 4, Money(9950, "USD"),  2,  EventType::MODIFY}, // Reduces #4 volume (5 -> 3)
            DomainEvent{t0, 4, Money(9980, "USD"),  10, EventType::MODIFY}  // Moves #4 price to 99.80 (vol 10)
        });

        BookSnapshotMapper mapper;
        MarketSimulationService simService(mapper, 1, 1);
        simService.runSimulation(stubGateway, 0, 0);

        LimitOrderBook& book = simService.book();
        assert(book.bids().empty());
        assert(book.asks().size() == 1);
        assert(book.asks().begin()->first == Money(9980, "USD"));
        assert(book.asks().begin()->second.totalVolume == 10);
        assert(book.asks().begin()->second.orders.front().volume == 10);
        std::cout << "FIFO, Partial Fill, Full Sweep & Modify via Service Layer + Stub: PASSED" << std::endl;

        LimitOrderBook replayedBook;
        replayedBook.replay(book.eventLog());
        assert(replayedBook.asks().size() == 1);
        assert(replayedBook.asks().begin()->second.totalVolume == 10);
        std::cout << "Event Sourcing State Replay test: PASSED" << std::endl;

        book.applyEvent(DomainEvent{t0, 5, Money(9920, "USD"), 30, EventType::BUY});
        BookSnapshotDTO snap = mapper.createSnapshot(book, t0);
        assert(std::abs(snap.midPrice - 99.50) < 1e-9);
        assert(std::abs(snap.imbalanceX - 0.5) < 1e-9);
        std::cout << "BookSnapshotDTO & Mapper test: PASSED (" << snap << ")" << std::endl;

        book.applyEvent(DomainEvent{t0, 4, Money(9980, "USD"), 10, EventType::CANCEL});
        book.applyEvent(DomainEvent{t0, 5, Money(9920, "USD"), 30, EventType::CANCEL});
        assert(book.activeOrdersCount() == 0);
        std::cout << "Order Cancel & Empty Book cleanup test: PASSED" << std::endl;
    }
};

} // namespace lob
