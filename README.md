# Limit Order Book (LOB) Matching Engine & HCR Analysis

An event-driven Limit Order Book matching engine and statistical analysis pipeline implemented in C++.
Developed as part of academic research on statistical market correlation analysis (Hierarchical Correlation Reconstruction — HCR) under the supervision of Jarosław Duda (Jagiellonian University).

## Architecture & Design Patterns
The system is structured around classical enterprise and domain-driven patterns:
* **Gateway (`MarketDataGateway`):** Step-by-step historical market data playback and validation from external files.
* **Value Object (`Money`):** Exact fixed-point monetary representation with currency safety and remainder-preserving proportional allocation.
* **Domain Event & Event Sourcing (`DomainEvent`, `LimitOrderBook`):** Deterministic order book updates, chronological event journaling, and full historical state reconstruction.
* **Snapshot & DTO (`BookSnapshotDTO`):** Periodic extraction of market state features (`midPrice`, order book `imbalanceX`, and `futureReturnY`).
* **Separated Interface & Plugin (`IAnalyzerPlugin`, `HCRCorrelationAnalyzer`):** Pluggable statistical analysis module computing EDF normalization, mixed Legendre polynomial moments (`Matrix4x4`), and independence tests.

## Features
* Historical data playback and validation.
* Limit order queuing, price-time matching, partial execution, modification, and cancellation.
* Chronological event logging and state reconstruction from any point in time.
* Non-linear correlation analysis between order book imbalance and future price returns via HCR.

## Getting Started

### Prerequisites
* **C++ Compiler:** Clang (macOS) or GCC (MSYS2 for Windows).
* Support for C++20.

### Build and Run
To compile and run locally:

```bash
clang++ -std=c++20 -Wall -Wextra -Werror -O3 -Iinclude main.cpp -o lob_engine
./lob_engine
```
