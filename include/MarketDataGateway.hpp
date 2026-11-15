#pragma once

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "IMarketDataGateway.hpp"

namespace lob {

/**
 * @brief Gateway (Martin Fowler) encapsulating external LOBSTER file access and translation.
 * Translates raw CSV rows (including re-indexing Type 4 executions into incoming BUY/SELL orders)
 * into clean DomainEvent objects completely isolated from the matching engine.
 */
class MarketDataGateway : public IMarketDataGateway {
private:
    std::string m_filePath;
    std::ifstream m_stream;
    std::string m_lineBuffer;
    uint64_t m_syntheticOrderId = 1ULL << 62;

    static EventType mapLobsterEventType(int rawType, int direction) noexcept {
        switch (rawType) {
            case 1:
                // Passive limit order submission: 1 = BUY, -1 = SELL
                return (direction == 1) ? EventType::BUY : EventType::SELL;
            case 4:
                // Re-index spread-crossing execution back into the incoming aggressive order:
                // hitting a resting SELL (-1) means an incoming BUY arrived;
                // hitting a resting BUY (1) means an incoming SELL arrived.
                return (direction == -1) ? EventType::BUY : EventType::SELL;
            case 2:
                return EventType::MODIFY;
            case 3:
                return EventType::CANCEL;
            default:
                return EventType::NONE;
        }
    }

public:
    explicit MarketDataGateway(std::string filePath) : m_filePath(std::move(filePath)), m_stream(m_filePath) {
        if (!m_stream.is_open()) {
            throw std::runtime_error("Failed to open market data file: " + m_filePath);
        }
        m_lineBuffer.reserve(128);
    }

    [[nodiscard]] const std::string& filePath() const noexcept { return m_filePath; }

    [[nodiscard]] DomainEvent readNextEvent() override {
        while (std::getline(m_stream, m_lineBuffer)) {
            if (m_lineBuffer.empty()) continue;

            const char* ptr = m_lineBuffer.c_str();
            char* end = nullptr;

            double timeSeconds = std::strtod(ptr, &end);
            if (ptr == end || *end != ',') continue;

            ptr = end + 1;
            int rawType = static_cast<int>(std::strtol(ptr, &end, 10));
            if (ptr == end || *end != ',') continue;

            ptr = end + 1;
            uint64_t orderId = std::strtoull(ptr, &end, 10);
            if (ptr == end || *end != ',') continue;

            ptr = end + 1;
            auto volume = static_cast<uint32_t>(std::strtoul(ptr, &end, 10));
            if (ptr == end || *end != ',') continue;

            ptr = end + 1;
            int64_t rawPrice = std::strtoll(ptr, &end, 10);
            if (ptr == end || *end != ',') continue;

            ptr = end + 1;
            int direction = static_cast<int>(std::strtol(ptr, &end, 10));
            if (ptr == end) continue;

            EventType mappedType = mapLobsterEventType(rawType, direction);
            if (mappedType == EventType::NONE) continue;

            auto nanos = std::chrono::nanoseconds(static_cast<int64_t>(std::llround(timeSeconds * 1e9)));

            return DomainEvent{
                TimePoint(std::chrono::duration_cast<TimePoint::duration>(nanos)),
                (rawType == 4) ? (++m_syntheticOrderId) : orderId,
                Money(rawPrice / 100, "USD"),
                volume,
                mappedType
            };
        }
        return DomainEvent::endOfStream();
    }
};

} // namespace lob
