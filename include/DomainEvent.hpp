#pragma once

#include <chrono>
#include <cstdint>
#include <ostream>
#include "Money.hpp"

namespace lob {

using TimePoint = std::chrono::system_clock::time_point;

enum class EventType {
    NONE,
    BUY,
    SELL,
    MODIFY,
    CANCEL,
    END_OF_STREAM
};

inline const char* eventTypeToString(EventType type) noexcept {
    switch (type) {
        case EventType::BUY:           return "BUY";
        case EventType::SELL:          return "SELL";
        case EventType::MODIFY:        return "MODIFY";
        case EventType::CANCEL:        return "CANCEL";
        case EventType::END_OF_STREAM: return "END_OF_STREAM";
        case EventType::NONE:
        default:                       return "NONE";
    }
}

/**
 * @brief Domain Event representing a standardized market operation parsed from raw data.
 */
struct DomainEvent {
    TimePoint timestamp{};
    uint64_t orderId = 0;
    Money price;
    uint32_t volume = 0;
    EventType type = EventType::NONE;

    friend std::ostream& operator<<(std::ostream& os, const DomainEvent& ev) {
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            ev.timestamp.time_since_epoch()
        ).count();
        double secondsAfterMidnight = static_cast<double>(nanos) / 1e9;

        os << "[t=" << secondsAfterMidnight << "s"
           << " | type=" << eventTypeToString(ev.type)
           << " | id=" << ev.orderId
           << " | vol=" << ev.volume
           << " | price=" << ev.price << "]";
        return os;
    }
};

} // namespace lob
