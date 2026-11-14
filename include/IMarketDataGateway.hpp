#pragma once

#include "DomainEvent.hpp"

namespace lob {

/**
 * @brief Separated Interface for market data input.
 * Decouples the domain/engine from specific file formats or external feeds.
 */
class IMarketDataGateway {
public:
    virtual ~IMarketDataGateway() = default;

    virtual DomainEvent readNextEvent() = 0;
};

} // namespace lob
