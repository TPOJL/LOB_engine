#pragma once

#include <cstddef>
#include <vector>
#include "BookSnapshotDTO.hpp"
#include "LimitOrderBook.hpp"

namespace lob {

/**
 * @brief Separated Interface (Martin Fowler) for extracting BookSnapshotDTOs from LimitOrderBook.
 * Keeps snapshot calculation and DTO mapping completely decoupled from the order book engine.
 */
class ISnapshotMapper {
public:
    virtual ~ISnapshotMapper() = default;

    [[nodiscard]] virtual BookSnapshotDTO createSnapshot(const LimitOrderBook& book, TimePoint timestamp) const = 0;

    virtual void populateFutureReturns(std::vector<BookSnapshotDTO>& snapshots, size_t horizonSteps = 1) const = 0;
};

} // namespace lob
