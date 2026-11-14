#pragma once

#include "DomainEvent.hpp"

namespace lob {

/**
 * @brief Separated Interface for market data gateways.
 * Enables swapping file formats and injecting a Service Stub for unit testing.
 */
class IMarketDataGateway {
public:
    virtual ~IMarketDataGateway() = default;

    [[nodiscard]] virtual DomainEvent readNextEvent() = 0;
};

} // namespace lob
