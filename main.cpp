#include <iostream>
#include <string>
#include "include/BookSnapshotMapper.hpp"
#include "include/MarketDataGateway.hpp"
#include "include/MarketSimulationService.hpp"
#include "include/SelfTestService.hpp"

int main() {
    std::ios::sync_with_stdio(false);

    const bool runTests = true;
    if (runTests) {
        lob::SelfTestService::runAll();
    }

    std::cout << "\n=== LOBSTER MarketSimulationService (Service Layer) ===" << std::endl;
    // const std::string lobsterPath = "../AAPL_data/AAPL_message.csv";
    const std::string lobsterPath = "../MSFT_data/MSFT_message.csv";

    try {
        lob::MarketDataGateway gateway(lobsterPath);
        lob::BookSnapshotMapper snapshotMapper;
        lob::MarketSimulationService simulationService(snapshotMapper, 100, 1);

        std::cout << "Reading from: " << gateway.filePath() << std::endl;
        auto snapshots = simulationService.runSimulation(gateway, 10, 10);
        (void)snapshots;
    } catch (const std::exception& e) {
        std::cerr << "Simulation error: " << e.what() << std::endl;
    }

    return 0;
}
