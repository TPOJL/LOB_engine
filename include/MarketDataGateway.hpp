#pragma once

#include <string>
#include <utility>
#include "IMarketDataGateway.hpp"

namespace lob {

/**
 * @brief Gateway stub for reading raw market data from a file.
 * Parses external file records one by one into DomainEvent objects.
 */
class MarketDataGateway : public IMarketDataGateway {
private:
    std::string m_filePath;

public:
    explicit MarketDataGateway(std::string filePath) : m_filePath(std::move(filePath)) {}

    [[nodiscard]] const std::string& filePath() const noexcept {
        return m_filePath;
    }

    DomainEvent readNextEvent() override {
        // TODO: Open and parse m_filePath into DomainEvent records
        DomainEvent event;
        event.type = EventType::END_OF_STREAM;
        return event;
    }
};

} // namespace lob
