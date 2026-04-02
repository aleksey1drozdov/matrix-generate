#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <map>
#include <atomic>
#include <memory>
#include <vector>
#include "core.h"
#include "symmetry_manager.h"

namespace fs = std::filesystem;

class FileHandler {
public:
    FileHandler(int n, int minT) : N(n), minTurns(minT), writtenCount(0), stop(false) {
        basePath = "maps/" + std::to_string(N) + "x" + std::to_string(N);
        basePathSemicolon = "maps/" + std::to_string(N) + "x" + std::to_string(N) + "_semicolumn";
        basePathSimplified = "maps/" + std::to_string(N) + "x" + std::to_string(N) + "_simplified";
        
        writeStandard = true;
        writeSemicolon = true;
        writeSimplified = true;

        // Start I/O thread
        ioThread = std::thread([this] {
            while (true) {
                std::vector<QueuedPath> localQueue;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait(lock, [this] { return stop || !writeQueue.empty(); });
                    if (stop && writeQueue.empty()) return;
                    localQueue.swap(writeQueue);
                }
                
                for (const auto& qp : localQueue) {
                    processWritePath(qp.start, qp.path);
                }
            }
        });
    }

    ~FileHandler() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        queueCondition.notify_all();
        if (ioThread.joinable()) ioThread.join();
    }

    void setFormats(bool standard, bool semicolon, bool simplified) {
        writeStandard = standard;
        writeSemicolon = semicolon;
        writeSimplified = simplified;
    }

    void prepareDirectory() {
        if (writeStandard && fs::exists(basePath)) fs::remove_all(basePath);
        if (writeSemicolon && fs::exists(basePathSemicolon)) fs::remove_all(basePathSemicolon);
        if (writeSimplified && fs::exists(basePathSimplified)) fs::remove_all(basePathSimplified);
        
        if (writeStandard) fs::create_directories(basePath);
        if (writeSemicolon) fs::create_directories(basePathSemicolon);
        if (writeSimplified) fs::create_directories(basePathSimplified);
    }

    void closeFiles() {
        std::lock_guard<std::mutex> lock(fileMutex);
        openFiles.clear();
    }

    void finalizeDirectory(long long totalCount) {
        closeFiles();
        if (writeStandard) {
            std::string newPath = "maps/" + std::to_string(N) + "x" + std::to_string(N) + "(" + std::to_string(totalCount) + ")";
            if (fs::exists(newPath)) fs::remove_all(newPath);
            
            // Write turns summary
            writeTurnsSummary();

            fs::rename(basePath, newPath);
        }

        if (writeSemicolon) {
            std::string newPathSemicolon = "maps/" + std::to_string(N) + "x" + std::to_string(N) + "_semicolumn(" + std::to_string(totalCount) + ")";
            if (fs::exists(newPathSemicolon)) fs::remove_all(newPathSemicolon);
            fs::rename(basePathSemicolon, newPathSemicolon);
        }

        if (writeSimplified) {
            std::string newPathSimplified = "maps/" + std::to_string(N) + "x" + std::to_string(N) + "_simplified(" + std::to_string(totalCount) + ")";
            if (fs::exists(newPathSimplified)) fs::remove_all(newPathSimplified);
            fs::rename(basePathSimplified, newPathSimplified);
        }
    }

    void writePath(Point start, const Path& path) {
        int turns = path.countTurns();
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            turnsStats[turns]++;
        }

        if (turns < minTurns) return;
        writtenCount++;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            writeQueue.push_back({start, path});
            if (writeQueue.size() > 1000) {
                queueCondition.notify_one();
            }
        }
    }

    long long getWrittenCount() const { return writtenCount.load(); }

private:
    struct QueuedPath {
        Point start;
        Path path;
    };

    void processWritePath(Point start, const Path& path) {
        std::string fileName = SymmetryManager::getFileName(start, N);
        
        // Write standard format
        if (writeStandard) {
            std::string fullPath = basePath + "/" + fileName;
            std::ofstream& out = getFile(fullPath);
            if (out.is_open()) {
                std::string line = path.toString() + "\n";
                out.write(line.c_str(), line.size());
            }
        }

        // Write semicolon format
        if (writeSemicolon) {
            std::string fullPathSemicolon = basePathSemicolon + "/" + fileName;
            std::ofstream& outSemicolon = getFile(fullPathSemicolon);
            if (outSemicolon.is_open()) {
                std::string line = path.toSemicolonString() + "\n";
                outSemicolon.write(line.c_str(), line.size());
            }
        }

        // Write simplified format
        if (writeSimplified) {
            std::string fullPathSimplified = basePathSimplified + "/" + fileName;
            std::ofstream& outSimplified = getFile(fullPathSimplified);
            if (outSimplified.is_open()) {
                std::string line = path.toSimplifiedString(N) + "\n";
                outSimplified.write(line.c_str(), line.size());
            }
        }
    }

    int N;
    int minTurns;
    bool writeStandard;
    bool writeSemicolon;
    bool writeSimplified;
    std::atomic<long long> writtenCount;
    std::string basePath;
    std::string basePathSemicolon;
    std::string basePathSimplified;
    std::mutex fileMutex;
    std::mutex statsMutex;
    std::map<int, long long> turnsStats;
    std::map<std::string, std::unique_ptr<std::ofstream>> openFiles;

    // Async I/O
    std::vector<QueuedPath> writeQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::thread ioThread;
    bool stop;

    std::ofstream& getFile(const std::string& path) {
        auto it = openFiles.find(path);
        if (it == openFiles.end()) {
            auto out = std::make_unique<std::ofstream>(path, std::ios::binary | std::ios::app);
            it = openFiles.emplace(path, std::move(out)).first;
        }
        return *(it->second);
    }

    void writeTurnsSummary() {
        std::string summaryPath = basePath + "/turns_summary.txt";
        std::ofstream out(summaryPath);
        if (out.is_open()) {
            for (auto const& [turns, count] : turnsStats) {
                out << turns << " - " << count << "\n";
            }
        }
    }
};

#endif // FILE_HANDLER_H
