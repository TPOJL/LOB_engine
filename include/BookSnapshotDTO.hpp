#pragma once

#include <chrono>
#include <ostream>
#include "DomainEvent.hpp"

namespace lob {

/**
 * @brief Data Transfer Object (Martin Fowler) carrying market snapshot metrics for statistical analysis.
 */
struct BookSnapshotDTO {
    TimePoint timestamp{};
    double midPrice = 0.0;
    double imbalanceX = 0.0;
    double futureReturnY = 0.0;

    friend std::ostream& operator<<(std::ostream& os, const BookSnapshotDTO& dto) {
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(dto.timestamp.time_since_epoch()).count();
        double secondsAfterMidnight = static_cast<double>(nanos) / 1e9;

        os << "[Snapshot t=" << secondsAfterMidnight << "s | midPrice=" << dto.midPrice
           << " | imbalanceX=" << dto.imbalanceX << " | futureReturnY=" << dto.futureReturnY << "]";
        return os;
    }
};

} // namespace lob
