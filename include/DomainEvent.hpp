#pragma once

#include <chrono>
#include <cstdint>
#include "Money.hpp"

namespace lob {

using TimePoint = std::chrono::system_clock::time_point;

enum class EventType {
    NONE,
    BUY,
    SELL,
    END_OF_STREAM
};

/**
 * @brief Domain Event representing a market order event parsed from raw data.
 */
struct DomainEvent {
    TimePoint timestamp{};
    uint64_t orderId = 0;
    Money price;
    uint32_t volume = 0;
    EventType type = EventType::NONE;
};

} // namespace lob
