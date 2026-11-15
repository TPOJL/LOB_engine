#pragma once

#include "DomainEvent.hpp"

namespace lob {

/**
 * @brief Separated Interface for market data gateways.
 */
class IMarketDataGateway {
public:
    virtual ~IMarketDataGateway() = default;

    [[nodiscard]] virtual DomainEvent readNextEvent() = 0;
};

} // namespace lob
