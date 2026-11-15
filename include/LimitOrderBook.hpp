#pragma once

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include "DomainEvent.hpp"

namespace lob {

struct BidPriceCompare {
    bool operator()(const Money& lhs, const Money& rhs) const { return rhs < lhs; }
};

/**
 * @brief Domain Model & Event Sourcing (Martin Fowler) engine for the Limit Order Book.
 * Implements price-time priority (FIFO) matching, partial fills, modifications, and cancellations.
 */
class LimitOrderBook {
public:
    using EventQueue = std::list<DomainEvent>;

    struct PriceLevel {
        uint32_t totalVolume = 0;
        EventQueue orders;
    };

    using BidMap = std::map<Money, PriceLevel, BidPriceCompare>;
    using AskMap = std::map<Money, PriceLevel>;

private:
    struct OrderHandle {
        BidMap::iterator bidLevel{};
        AskMap::iterator askLevel{};
        EventQueue::iterator orderIt{};
    };

    std::vector<DomainEvent> m_eventLog;
    BidMap m_bids;
    AskMap m_asks;
    std::unordered_map<uint64_t, OrderHandle> m_orderIndex;
    bool m_logFills = false;

    void printOrderFilled(const DomainEvent& order) const {
        if (!m_logFills) return;
        std::cout << "   -> Order FILLED: id=" << order.orderId
                  << " | type=" << eventTypeToString(order.type)
                  << " | price=" << order.price << '\n';
    }

    void removeOrder(std::unordered_map<uint64_t, OrderHandle>::iterator idxIt) {
        const OrderHandle& handle = idxIt->second;
        uint32_t vol = handle.orderIt->volume;

        if (handle.orderIt->type == EventType::BUY) {
            PriceLevel& level = handle.bidLevel->second;
            level.totalVolume -= vol;
            level.orders.erase(handle.orderIt);
            if (level.orders.empty()) m_bids.erase(handle.bidLevel);
        } else {
            PriceLevel& level = handle.askLevel->second;
            level.totalVolume -= vol;
            level.orders.erase(handle.orderIt);
            if (level.orders.empty()) m_asks.erase(handle.askLevel);
        }
        m_orderIndex.erase(idxIt);
    }

    void handleBuy(DomainEvent incoming) {
        if (incoming.volume == 0 || incoming.price.amount() <= 0) return;
        bool matchedAny = false;

        while (incoming.volume > 0 && !m_asks.empty()) {
            auto bestAskIt = m_asks.begin();
            if (bestAskIt->first.greaterThan(incoming.price)) break;

            PriceLevel& askLevel = bestAskIt->second;
            while (incoming.volume > 0 && !askLevel.orders.empty()) {
                DomainEvent& resting = askLevel.orders.front();
                uint32_t fillVol = std::min(incoming.volume, resting.volume);

                incoming.volume -= fillVol;
                resting.volume -= fillVol;
                askLevel.totalVolume -= fillVol;
                matchedAny = true;

                if (resting.volume == 0) {
                    printOrderFilled(resting);
                    m_orderIndex.erase(resting.orderId);
                    askLevel.orders.pop_front();
                }
            }

            if (askLevel.orders.empty()) m_asks.erase(bestAskIt);
        }

        if (matchedAny && incoming.volume == 0) {
            printOrderFilled(incoming);
        }

        if (incoming.volume > 0) {
            auto existingIt = m_orderIndex.find(incoming.orderId);
            if (existingIt != m_orderIndex.end()) {
                removeOrder(existingIt);
            }
            auto levelIt = m_bids.emplace(incoming.price, PriceLevel{}).first;
            levelIt->second.totalVolume += incoming.volume;
            levelIt->second.orders.push_back(incoming);
            m_orderIndex[incoming.orderId] = OrderHandle{levelIt, {}, std::prev(levelIt->second.orders.end())};
        }
    }

    void handleSell(DomainEvent incoming) {
        if (incoming.volume == 0 || incoming.price.amount() <= 0) return;
        bool matchedAny = false;

        while (incoming.volume > 0 && !m_bids.empty()) {
            auto bestBidIt = m_bids.begin();
            if (incoming.price.greaterThan(bestBidIt->first)) break;

            PriceLevel& bidLevel = bestBidIt->second;
            while (incoming.volume > 0 && !bidLevel.orders.empty()) {
                DomainEvent& resting = bidLevel.orders.front();
                uint32_t fillVol = std::min(incoming.volume, resting.volume);

                incoming.volume -= fillVol;
                resting.volume -= fillVol;
                bidLevel.totalVolume -= fillVol;
                matchedAny = true;

                if (resting.volume == 0) {
                    printOrderFilled(resting);
                    m_orderIndex.erase(resting.orderId);
                    bidLevel.orders.pop_front();
                }
            }

            if (bidLevel.orders.empty()) m_bids.erase(bestBidIt);
        }

        if (matchedAny && incoming.volume == 0) {
            printOrderFilled(incoming);
        }

        if (incoming.volume > 0) {
            auto existingIt = m_orderIndex.find(incoming.orderId);
            if (existingIt != m_orderIndex.end()) {
                removeOrder(existingIt);
            }
            auto levelIt = m_asks.emplace(incoming.price, PriceLevel{}).first;
            levelIt->second.totalVolume += incoming.volume;
            levelIt->second.orders.push_back(incoming);
            m_orderIndex[incoming.orderId] = OrderHandle{{}, levelIt, std::prev(levelIt->second.orders.end())};
        }
    }

    void handleCancel(const DomainEvent& event) {
        auto idxIt = m_orderIndex.find(event.orderId);
        if (idxIt != m_orderIndex.end()) {
            removeOrder(idxIt);
        }
    }

    void handleModify(const DomainEvent& event) {
        auto idxIt = m_orderIndex.find(event.orderId);
        if (idxIt == m_orderIndex.end()) return;

        OrderHandle& handle = idxIt->second;
        auto orderIt = handle.orderIt;

        if (event.price == orderIt->price) {
            if (event.volume == 0 || event.volume >= orderIt->volume) {
                removeOrder(idxIt);
            } else {
                orderIt->volume -= event.volume;
                if (orderIt->type == EventType::BUY) {
                    handle.bidLevel->second.totalVolume -= event.volume;
                } else {
                    handle.askLevel->second.totalVolume -= event.volume;
                }
            }
            return;
        }

        DomainEvent updated = *orderIt;
        updated.timestamp = event.timestamp;
        updated.price = event.price;
        updated.volume = event.volume;

        removeOrder(idxIt);

        if (updated.type == EventType::BUY) {
            handleBuy(updated);
        } else if (updated.type == EventType::SELL) {
            handleSell(updated);
        }
    }

public:
    LimitOrderBook() = default;

    void setLogFills(bool enabled) noexcept { m_logFills = enabled; }

    void applyEvent(const DomainEvent& event) {
        if (event.isNull() || event.isEndOfStream()) return;

        m_eventLog.push_back(event);

        switch (event.type) {
            case EventType::BUY:    handleBuy(event);    break;
            case EventType::SELL:   handleSell(event);   break;
            case EventType::CANCEL: handleCancel(event); break;
            case EventType::MODIFY: handleModify(event); break;
            default: break;
        }
    }

    /**
     * @brief Event Sourcing state reconstruction from a saved event log.
     */
    void replay(const std::vector<DomainEvent>& history) {
        m_eventLog.clear();
        m_bids.clear();
        m_asks.clear();
        m_orderIndex.clear();
        for (const auto& ev : history) applyEvent(ev);
    }

    [[nodiscard]] bool hasBothSides() const noexcept { return !m_bids.empty() && !m_asks.empty(); }
    [[nodiscard]] const std::vector<DomainEvent>& eventLog() const noexcept { return m_eventLog; }
    [[nodiscard]] const BidMap& bids() const noexcept { return m_bids; }
    [[nodiscard]] const AskMap& asks() const noexcept { return m_asks; }
    [[nodiscard]] size_t activeOrdersCount() const noexcept { return m_orderIndex.size(); }
};

} // namespace lob
