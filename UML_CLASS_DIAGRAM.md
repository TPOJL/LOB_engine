# UML Class Diagram — LOB Matching Engine & Snapshot Pipeline

This diagram reflects the **strict UML static dependency and ownership relationships** (who depends on / `#include`s whom) across the currently implemented codebase, structured around Martin Fowler's *Patterns of Enterprise Application Architecture (PoEAA)*.

> **Note:** GitHub renders the Mermaid diagram below natively. As new modules (such as `IAnalyzerPlugin`, `HCRCorrelationAnalyzer`, and `Registry`) are added, this diagram is updated alongside the code.

```mermaid
classDiagram
    direction TB

    class Money {
        <<Value Object / Money>>
        -int64_t m_amount
        -string m_currency
        -checkCurrency(Money other) void
        +fromDecimal(double amountMajor, string currency)$ Money
        +amount() int64_t
        +currency() string
        +add(Money other) Money
        +subtract(Money other) Money
        +multiply(double factor) Money
        +allocate(vector~int~ ratios) vector~Money~
        +compareTo(Money other) int
        +greaterThan(Money other) bool
    }

    class CurrencyMismatchException {
        <<Exception>>
        +CurrencyMismatchException(string curr1, string curr2)
    }

    class EventType {
        <<enumeration>>
        NONE
        BUY
        SELL
        MODIFY
        CANCEL
        END_OF_STREAM
    }

    class DomainEvent {
        <<Domain Event / Special Case>>
        +TimePoint timestamp
        +uint64_t orderId
        +Money price
        +uint32_t volume
        +EventType type
        +nullEvent()$ DomainEvent
        +endOfStream()$ DomainEvent
        +isEndOfStream() bool
        +isNull() bool
    }

    class IMarketDataGateway {
        <<Separated Interface>>
        +readNextEvent()* DomainEvent
    }

    class MarketDataGateway {
        <<Gateway>>
        -string m_filePath
        -ifstream m_stream
        -string m_lineBuffer
        -uint64_t m_syntheticOrderId
        -mapLobsterEventType(int rawType, int direction)$ EventType
        +filePath() string
        +readNextEvent() DomainEvent
    }

    class StubMarketDataGateway {
        <<Service Stub>>
        -vector~DomainEvent~ m_events
        -size_t m_cursor
        +readNextEvent() DomainEvent
    }

    class LimitOrderBook {
        <<Domain Model & Event Sourcing>>
        -vector~DomainEvent~ m_eventLog
        -BidMap m_bids
        -AskMap m_asks
        -unordered_map~uint64_t, OrderHandle~ m_orderIndex
        -bool m_logFills
        -handleBuy(DomainEvent incoming) void
        -handleSell(DomainEvent incoming) void
        -handleCancel(DomainEvent event) void
        -handleModify(DomainEvent event) void
        -removeOrder(iterator idxIt) void
        +setLogFills(bool enabled) void
        +applyEvent(DomainEvent event) void
        +replay(vector~DomainEvent~ history) void
        +hasBothSides() bool
        +eventLog() vector~DomainEvent~
        +bids() BidMap
        +asks() AskMap
        +activeOrdersCount() size_t
    }

    class PriceLevel {
        <<Inner Struct>>
        +uint32_t totalVolume
        +list~DomainEvent~ orders
    }

    class OrderHandle {
        <<Inner Struct>>
        +BidMap_iterator bidLevel
        +AskMap_iterator askLevel
        +EventQueue_iterator orderIt
    }

    class BookSnapshotDTO {
        <<Snapshot & DTO>>
        +TimePoint timestamp
        +double midPrice
        +double imbalanceX
        +double futureReturnY
    }

    class ISnapshotMapper {
        <<Separated Interface>>
        +createSnapshot(LimitOrderBook book, TimePoint timestamp)* BookSnapshotDTO
        +populateFutureReturns(vector~BookSnapshotDTO~ snapshots, size_t horizonSteps)* void
    }

    class BookSnapshotMapper {
        <<Mapper>>
        +createSnapshot(LimitOrderBook book, TimePoint timestamp) BookSnapshotDTO
        +populateFutureReturns(vector~BookSnapshotDTO~ snapshots, size_t horizonSteps) void
    }

    class MarketSimulationService {
        <<Service Layer>>
        -LimitOrderBook m_book
        -ISnapshotMapper m_snapshotMapper
        -size_t m_snapshotIntervalEvents
        -size_t m_futureHorizonSteps
        +runSimulation(IMarketDataGateway gateway, uint64_t maxPrintEvents, size_t maxPrintSnapshots) vector~BookSnapshotDTO~
        +book() LimitOrderBook
    }

    class SelfTestService {
        <<Service Layer>>
        +runAll()$ void
    }

    %% Value Object & Domain Event relationships
    Money ..> CurrencyMismatchException : throws
    DomainEvent *-- Money : price
    DomainEvent *-- EventType : type

    %% Input Gateway Layer relationships
    IMarketDataGateway <|.. MarketDataGateway : implements
    IMarketDataGateway <|.. StubMarketDataGateway : implements
    IMarketDataGateway ..> DomainEvent : produces

    %% Core Order Book Domain Model relationships
    LimitOrderBook *-- PriceLevel : m_bids / m_asks
    LimitOrderBook *-- OrderHandle : m_orderIndex
    PriceLevel o-- DomainEvent : FIFO queue
    LimitOrderBook ..> DomainEvent : processes & logs

    %% Snapshot & DTO Mapping Layer relationships
    ISnapshotMapper <|.. BookSnapshotMapper : implements
    ISnapshotMapper ..> LimitOrderBook : reads state
    ISnapshotMapper ..> BookSnapshotDTO : creates

    %% Service Layer relationships
    MarketSimulationService *-- LimitOrderBook : owns m_book
    MarketSimulationService --> ISnapshotMapper : uses m_snapshotMapper
    MarketSimulationService ..> IMarketDataGateway : coordinates
    MarketSimulationService ..> BookSnapshotDTO : returns vector

    SelfTestService ..> MarketSimulationService : tests
    SelfTestService ..> StubMarketDataGateway : uses
    SelfTestService ..> BookSnapshotMapper : tests
```

## Legend of UML Relationships

| Symbol | Relationship | Meaning in C++ Code |
| :--- | :--- | :--- |
| `A <\|.. B` | **Realization (Implementation)** | Class `B` inherits from and implements pure virtual interface `A`. |
| `A *-- B` | **Composition** | `A` owns `B` by value; `B`'s lifetime is strictly bound to `A`. |
| `A o-- B` | **Aggregation** | `A` holds a collection of `B` elements (`std::list<DomainEvent>`). |
| `A --> B` | **Directed Association** | `A` holds a reference or pointer to `B` (`const ISnapshotMapper&`). |
| `A ..> B` | **Dependency** | `A` `#include`s `B` and uses it as a method parameter or return type (`B` has no knowledge of `A`). |

<details>
<summary><b>Click to expand PlantUML source code (for Draw.io / PlantText export)</b></summary>

```plantuml
@startuml
skinparam classAttributeIconSize 0
skinparam packageStyle rectangle

package "Value Objects & Domain Events" {
    class Money <<Value Object / Money>> {
        - m_amount : int64_t
        - m_currency : string
        + {static} fromDecimal(amountMajor : double, currency : string) : Money
        + amount() : int64_t
        + currency() : const string&
        + add(other : Money) : Money
        + subtract(other : Money) : Money
        + multiply(factor : double) : Money
        + allocate(ratios : vector<int>) : vector<Money>
        + compareTo(other : Money) : int
        + greaterThan(other : Money) : bool
    }

    enum EventType {
        NONE
        BUY
        SELL
        MODIFY
        CANCEL
        END_OF_STREAM
    }

    struct DomainEvent <<Domain Event / Special Case>> {
        + timestamp : TimePoint
        + orderId : uint64_t
        + price : Money
        + volume : uint32_t
        + type : EventType
        + {static} nullEvent() : DomainEvent
        + {static} endOfStream() : DomainEvent
        + isEndOfStream() : bool
        + isNull() : bool
    }
}

package "Input Gateway Layer" {
    interface IMarketDataGateway <<Separated Interface>> {
        + {abstract} readNextEvent() : DomainEvent
    }

    class MarketDataGateway <<Gateway>> {
        - m_filePath : string
        - m_stream : ifstream
        - m_lineBuffer : string
        - m_syntheticOrderId : uint64_t
        - {static} mapLobsterEventType(rawType : int, direction : int) : EventType
        + filePath() : const string&
        + readNextEvent() : DomainEvent
    }

    class StubMarketDataGateway <<Service Stub>> {
        - m_events : vector<DomainEvent>
        - m_cursor : size_t
        + readNextEvent() : DomainEvent
    }
}

package "Core Matching Engine" {
    struct PriceLevel {
        + totalVolume : uint32_t
        + orders : list<DomainEvent>
    }

    class LimitOrderBook <<Domain Model & Event Sourcing>> {
        - m_eventLog : vector<DomainEvent>
        - m_bids : map<Money, PriceLevel, BidPriceCompare>
        - m_asks : map<Money, PriceLevel>
        - m_orderIndex : unordered_map<uint64_t, OrderHandle>
        - m_logFills : bool
        + applyEvent(event : DomainEvent) : void
        + replay(history : vector<DomainEvent>) : void
        + hasBothSides() : bool
        + eventLog() : const vector<DomainEvent>&
        + bids() : const BidMap&
        + asks() : const AskMap&
        + activeOrdersCount() : size_t
    }
}

package "Snapshot & Mapping Layer" {
    struct BookSnapshotDTO <<Snapshot & DTO>> {
        + timestamp : TimePoint
        + midPrice : double
        + imbalanceX : double
        + futureReturnY : double
    }

    interface ISnapshotMapper <<Separated Interface>> {
        + {abstract} createSnapshot(book : LimitOrderBook, timestamp : TimePoint) : BookSnapshotDTO
        + {abstract} populateFutureReturns(snapshots : vector<BookSnapshotDTO>&, horizonSteps : size_t) : void
    }

    class BookSnapshotMapper <<Mapper>> {
        + createSnapshot(book : LimitOrderBook, timestamp : TimePoint) : BookSnapshotDTO
        + populateFutureReturns(snapshots : vector<BookSnapshotDTO>&, horizonSteps : size_t) : void
    }
}

package "Application Service Layer" {
    class MarketSimulationService <<Service Layer>> {
        - m_book : LimitOrderBook
        - m_snapshotMapper : const ISnapshotMapper&
        - m_snapshotIntervalEvents : size_t
        - m_futureHorizonSteps : size_t
        + runSimulation(gateway : IMarketDataGateway&, maxPrintEvents : uint64_t, maxPrintSnapshots : size_t) : vector<BookSnapshotDTO>
        + book() : LimitOrderBook&
    }

    class SelfTestService <<Service Layer>> {
        + {static} runAll() : void
    }
}

DomainEvent *-- Money
DomainEvent *-- EventType
IMarketDataGateway <|.. MarketDataGateway
IMarketDataGateway <|.. StubMarketDataGateway
IMarketDataGateway ..> DomainEvent
LimitOrderBook *-- PriceLevel
PriceLevel o-- DomainEvent
LimitOrderBook ..> DomainEvent
ISnapshotMapper <|.. BookSnapshotMapper
ISnapshotMapper ..> LimitOrderBook
ISnapshotMapper ..> BookSnapshotDTO
MarketSimulationService *-- LimitOrderBook
MarketSimulationService --> ISnapshotMapper
MarketSimulationService ..> IMarketDataGateway
MarketSimulationService ..> BookSnapshotDTO
SelfTestService ..> MarketSimulationService
SelfTestService ..> StubMarketDataGateway
SelfTestService ..> BookSnapshotMapper
@enduml
```
</details>
