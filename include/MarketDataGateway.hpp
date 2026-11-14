#pragma once

#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "IMarketDataGateway.hpp"

namespace lob {

/**
 * @brief Gateway for step-by-step historical market data playback from a LOBSTER CSV file.
 * Columns: Time, Type, OrderID, Size, Price (USD * 10000), Direction (1=Buy, -1=Sell).
 */
class MarketDataGateway : public IMarketDataGateway {
private:
    std::string m_filePath;
    std::ifstream m_stream;

    static EventType mapLobsterEventType(int rawType, int direction) noexcept {
        switch (rawType) {
            case 1:
                return (direction == 1) ? EventType::BUY : EventType::SELL;
            case 2:
                return EventType::MODIFY;
            case 3:
                return EventType::CANCEL;
            default:
                return EventType::NONE;
        }
    }

public:
    explicit MarketDataGateway(std::string filePath)
        : m_filePath(std::move(filePath)), m_stream(m_filePath) {
        if (!m_stream.is_open()) {
            throw std::runtime_error("Failed to open market data file: " + m_filePath);
        }
    }

    [[nodiscard]] const std::string& filePath() const noexcept {
        return m_filePath;
    }

    [[nodiscard]] DomainEvent readNextEvent() override {
        std::string line;
        while (std::getline(m_stream, line)) {
            if (line.empty()) {
                continue;
            }

            std::stringstream ss(line);
            std::string token;

            double timeSeconds = 0.0;
            int rawType = 0;
            uint64_t orderId = 0;
            uint32_t volume = 0;
            int64_t rawPrice = 0;
            int direction = 0;

            if (!std::getline(ss, token, ',')) continue;
            timeSeconds = std::stod(token);

            if (!std::getline(ss, token, ',')) continue;
            rawType = std::stoi(token);

            if (!std::getline(ss, token, ',')) continue;
            orderId = std::stoull(token);

            if (!std::getline(ss, token, ',')) continue;
            volume = static_cast<uint32_t>(std::stoul(token));

            if (!std::getline(ss, token, ',')) continue;
            rawPrice = std::stoll(token);

            if (!std::getline(ss, token, ',')) continue;
            direction = std::stoi(token);

            EventType mappedType = mapLobsterEventType(rawType, direction);
            if (mappedType == EventType::NONE) {
                continue;
            }

            auto nanos = std::chrono::nanoseconds(
                static_cast<int64_t>(std::llround(timeSeconds * 1e9))
            );

            DomainEvent event;
            event.timestamp = TimePoint(std::chrono::duration_cast<TimePoint::duration>(nanos));
            event.orderId = orderId;
            // LOBSTER stores price as USD * 10000; convert to cents (USD * 100)
            event.price = Money(rawPrice / 100, "USD");
            event.volume = volume;
            event.type = mappedType;
            return event;
        }

        DomainEvent eofEvent;
        eofEvent.type = EventType::END_OF_STREAM;
        return eofEvent;
    }
};

} // namespace lob
