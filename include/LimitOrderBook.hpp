#pragma once

#include <list>
#include <map>
#include <vector>
#include "DomainEvent.hpp"

namespace lob {

struct BidPriceCompare {
    bool operator()(const Money& lhs, const Money& rhs) const { return rhs < lhs; }
};

/**
 * @brief Domain Model & Event Sourcing container for the Limit Order Book.
 * Stores chronological event history and price-time priority queues for bids and asks.
 */
class LimitOrderBook {
public:
    using EventQueue = std::list<DomainEvent>;
    using BidMap = std::map<Money, EventQueue, BidPriceCompare>;
    using AskMap = std::map<Money, EventQueue>;

private:
    std::vector<DomainEvent> m_eventLog;
    BidMap m_bids;
    AskMap m_asks;

public:
    void applyEvent(const DomainEvent& event) {
        m_eventLog.push_back(event);

        if (event.type == EventType::BUY) {
            m_bids[event.price].push_back(event);
        } else if (event.type == EventType::SELL) {
            m_asks[event.price].push_back(event);
        }
        // TODO: Add matching logic, MODIFY, and CANCEL handling
    }

    [[nodiscard]] const std::vector<DomainEvent>& eventLog() const noexcept { return m_eventLog; }
    [[nodiscard]] const BidMap& bids() const noexcept { return m_bids; }
    [[nodiscard]] const AskMap& asks() const noexcept { return m_asks; }
};

} // namespace lob
