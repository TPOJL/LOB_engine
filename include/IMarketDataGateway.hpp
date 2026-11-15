#pragma once

#include <utility>
#include <vector>
#include "DomainEvent.hpp"

namespace lob {

/**
 * @brief Separated Interface (Martin Fowler) for market data gateways.
 */
class IMarketDataGateway {
public:
    virtual ~IMarketDataGateway() = default;

    [[nodiscard]] virtual DomainEvent readNextEvent() = 0;
};

/**
 * @brief Service Stub (Martin Fowler) for deterministic in-memory testing without file I/O.
 */
class StubMarketDataGateway : public IMarketDataGateway {
private:
    std::vector<DomainEvent> m_events;
    size_t m_cursor = 0;

public:
    explicit StubMarketDataGateway(std::vector<DomainEvent> events) : m_events(std::move(events)) {}

    [[nodiscard]] DomainEvent readNextEvent() override {
        if (m_cursor >= m_events.size()) return DomainEvent::endOfStream();
        return m_events[m_cursor++];
    }
};

} // namespace lob
