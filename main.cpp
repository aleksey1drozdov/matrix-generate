#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "core.h"
#include "dfs.h"
#include "symmetry_manager.h"
#include "file_handler.h"

// Simple config.ini parser
struct Config {
    int threads_count = 1;
    int recursion_depth_parallelism = 5;

    bool write_standard = true;
    bool write_semicolon = true;
    bool write_simplified = true;

    void load(const std::string& path) {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.find("threads_count") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    threads_count = std::stoi(line.substr(pos + 1));
                }
            } else if (line.find("recursion_depth_parallelism") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    recursion_depth_parallelism = std::stoi(line.substr(pos + 1));
                }
            } else if (line.find("write_standard") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    write_standard = (line.substr(pos + 1).find("true") != std::string::npos || line.substr(pos + 1).find("1") != std::string::npos);
                }
            } else if (line.find("write_semicolon") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    write_semicolon = (line.substr(pos + 1).find("true") != std::string::npos || line.substr(pos + 1).find("1") != std::string::npos);
                }
            } else if (line.find("write_simplified") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    write_simplified = (line.substr(pos + 1).find("true") != std::string::npos || line.substr(pos + 1).find("1") != std::string::npos);
                }
            }
        }
    }
};

int main() {
    std::cout << "=== Hamiltonian Path Generator ===" << std::endl;
    
    Config config;
    config.load("config.ini");
    std::cout << "Threads from config: " << config.threads_count << std::endl;

    int N;
    std::cout << "Enter grid size N: " << std::flush;
    if (!(std::cin >> N) || N <= 1) {
        std::cerr << "Error: N must be > 1" << std::endl;
        return 1;
    }

    int turns;
    std::cout << "Enter minimum turns count: " << std::flush;
    if (!(std::cin >> turns) || turns < 1) {
        std::cerr << "Error: turns must be >= 1" << std::endl;
        return 1;
    }

    if (N > 8) {
        std::cout << "Warning: N > 8 may take a lot of time and memory." << std::endl;
    }

    FileHandler fileHandler(N, turns);
    fileHandler.setFormats(config.write_standard, config.write_semicolon, config.write_simplified);
    fileHandler.prepareDirectory();

    HamiltonianDFS dfs(N, config.threads_count, config.recursion_depth_parallelism);
    
    auto startTime = std::chrono::high_resolution_clock::now();

    // Start points from 1/8 zone
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            Point p = {x, y};
            if (SymmetryManager::isInSearchZone(p, N)) {
                dfs.addStartTask(p, [p, &fileHandler](const Path& path) {
                    fileHandler.writePath(p, path);
                });
            }
        }
    }

    // Wait with progress reporting
    while (dfs.getActiveTasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "\rPaths found: " << dfs.getTotalCount() << " | Written: " << fileHandler.getWrittenCount() << "        " << std::flush;
    }
    std::cout << "\rPaths found: " << dfs.getTotalCount() << " | Written: " << fileHandler.getWrittenCount() << "        " << std::endl;
    dfs.wait();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    long long total = dfs.getTotalCount();
    long long written = fileHandler.getWrittenCount();
    std::cout << "\nSearch complete!" << std::endl;
    std::cout << "Total original paths found: " << total << std::endl;
    std::cout << "Paths written (with turns >= " << turns << "): " << written << std::endl;
    std::cout << "Execution time: " << elapsed.count() << " sec." << std::endl;

    fileHandler.finalizeDirectory(written);
    std::cout << "Results saved in maps/" << std::endl;

    return 0;
}
