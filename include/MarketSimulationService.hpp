#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include "IMarketDataGateway.hpp"
#include "ISnapshotMapper.hpp"
#include "LimitOrderBook.hpp"

namespace lob {

/**
 * @brief Service Layer (Martin Fowler) defining the application boundary and coordinating
 * market data ingestion, order book matching, and DTO snapshot extraction.
 * Contains zero domain logic itself; delegates all business rules to domain objects and mappers.
 */
class MarketSimulationService {
private:
    LimitOrderBook m_book;
    const ISnapshotMapper& m_snapshotMapper;
    size_t m_snapshotIntervalEvents;
    size_t m_futureHorizonSteps;

public:
    explicit MarketSimulationService(
        const ISnapshotMapper& snapshotMapper,
        size_t snapshotIntervalEvents = 100,
        size_t futureHorizonSteps = 1
    ) noexcept
        : m_snapshotMapper(snapshotMapper),
          m_snapshotIntervalEvents(snapshotIntervalEvents),
          m_futureHorizonSteps(futureHorizonSteps) {}

    /**
     * @brief Coordinates a full simulation run from a market data gateway and returns collected DTO snapshots.
     */
    std::vector<BookSnapshotDTO> runSimulation(IMarketDataGateway& gateway, uint64_t maxPrintEvents = 10, size_t maxPrintSnapshots = 10) {
        uint64_t processedEvents = 0;
        std::vector<BookSnapshotDTO> snapshots;

        std::cout << std::fixed << std::setprecision(6);
        auto startTime = std::chrono::steady_clock::now();

        while (true) {
            DomainEvent event = gateway.readNextEvent();

            if (event.isEndOfStream()) break;
            if (event.isNull()) continue;

            ++processedEvents;
            bool shouldPrint = (processedEvents <= maxPrintEvents);

            if (shouldPrint) {
                std::cout << "#" << std::setw(2) << processedEvents << " " << event << '\n';
            }

            m_book.setLogFills(shouldPrint);
            m_book.applyEvent(event);

            if (m_snapshotIntervalEvents > 0 && (processedEvents % m_snapshotIntervalEvents == 0) && m_book.hasBothSides()) {
                snapshots.push_back(m_snapshotMapper.createSnapshot(m_book, event.timestamp));
            }
        }

        m_snapshotMapper.populateFutureReturns(snapshots, m_futureHorizonSteps);

        auto endTime = std::chrono::steady_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        if (processedEvents > maxPrintEvents) {
            std::cout << "... (" << (processedEvents - maxPrintEvents) << " more events processed)" << std::endl;
        }

        std::cout << std::setprecision(2);
        std::cout << "Simulation finished in " << elapsedMs << " ms | Total logged events: " << m_book.eventLog().size()
                  << " | Captured snapshots: " << snapshots.size()
                  << " | Active resting orders: " << m_book.activeOrdersCount() << std::endl;

        if (m_book.hasBothSides()) {
            std::cout << "Final Best Bid: " << m_book.bids().begin()->first
                      << " (vol: " << m_book.bids().begin()->second.totalVolume << ")"
                      << " | Final Best Ask: " << m_book.asks().begin()->first
                      << " (vol: " << m_book.asks().begin()->second.totalVolume << ")" << std::endl;
        }

        if (!snapshots.empty() && maxPrintSnapshots > 0) {
            std::cout << std::setprecision(4);
            std::cout << "\n--- Sample Captured BookSnapshotDTOs ---" << std::endl;
            size_t count = (snapshots.size() < maxPrintSnapshots) ? snapshots.size() : maxPrintSnapshots;
            for (size_t i = 0; i < count; ++i) {
                std::cout << "#" << (i + 1) << " " << snapshots[i] << std::endl;
            }
        }

        return snapshots;
    }

    [[nodiscard]] const LimitOrderBook& book() const noexcept { return m_book; }
    [[nodiscard]] LimitOrderBook& book() noexcept { return m_book; }
};

} // namespace lob
