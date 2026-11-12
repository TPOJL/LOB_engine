# Limit Order Book (LOB) Matching Engine

A low-latency, event-driven Limit Order Book matching engine implemented in C++.
Developed as part of academic research on statistical market correlation analysis under the supervision of Jarosław Duda (Jagiellonian University).

## Architecture & Design Patterns
The engine is heavily optimized for performance and predictable execution times. The architecture leverages several core patterns:
* **Object Pool:** To ensure zero-allocation order management during runtime and prevent memory fragmentation.
* **Event Sourcing & Observer:** For real-time state updates and decoupled market data dissemination.
* **Command, DTO & Strategy:** For structured request handling, data transfer, and flexible matching logic execution.

## Features
* Fast limit order insertion, modification, and cancellation.
* Price-time priority matching algorithm.
* Statistical module for computing hidden non-linear market correlations.

## Getting Started

### Prerequisites
* **C++ Compiler:** GCC (MSYS2 for Windows) or Clang (macOS).
* Support for C++17 / C++20.

### Build and Run
To compile the engine locally using the standard toolchain:

```bash
# Compile the source code
g++ main.cpp -o lob_engine

# Run the executable
./lob_engine