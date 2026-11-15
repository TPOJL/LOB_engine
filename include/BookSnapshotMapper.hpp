#pragma once

#include <cstddef>
#include <vector>
#include "ISnapshotMapper.hpp"

namespace lob {

/**
 * @brief Mapper (Martin Fowler) translating LimitOrderBook state into BookSnapshotDTO records.
 * Uses cached O(1) PriceLevel::totalVolume to compute order book imbalance without iterating queues.
 */
class BookSnapshotMapper : public ISnapshotMapper {
public:
    [[nodiscard]] BookSnapshotDTO createSnapshot(const LimitOrderBook& book, TimePoint timestamp) const override {
        BookSnapshotDTO dto;
        dto.timestamp = timestamp;

        if (!book.hasBothSides()) return dto;

        auto bestBidIt = book.bids().begin();
        auto bestAskIt = book.asks().begin();

        double bidPrice = static_cast<double>(bestBidIt->first.amount()) / 100.0;
        double askPrice = static_cast<double>(bestAskIt->first.amount()) / 100.0;
        dto.midPrice = (bidPrice + askPrice) * 0.5;

        double bidVol = static_cast<double>(bestBidIt->second.totalVolume);
        double askVol = static_cast<double>(bestAskIt->second.totalVolume);
        double totalVol = bidVol + askVol;

        if (totalVol > 0.0) {
            dto.imbalanceX = (bidVol - askVol) / totalVol;
        }

        return dto;
    }

    void populateFutureReturns(std::vector<BookSnapshotDTO>& snapshots, size_t horizonSteps = 1) const override {
        if (horizonSteps == 0 || snapshots.size() <= horizonSteps) return;

        for (size_t i = 0; i + horizonSteps < snapshots.size(); ++i) {
            snapshots[i].futureReturnY = snapshots[i + horizonSteps].midPrice - snapshots[i].midPrice;
        }
    }
};

} // namespace lob
