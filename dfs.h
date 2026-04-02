#ifndef DFS_H
#define DFS_H

#include <iostream>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include "core.h"

class HamiltonianDFS {
public:
    HamiltonianDFS(int n, int threads_count, int max_parallel_depth) 
        : N(n), maxThreads(threads_count), maxParallelDepth(max_parallel_depth), 
          totalCount(0), activeTasks(0), stop(false) 
    {
        for (int i = 0; i < maxThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                    if (activeTasks.fetch_sub(1) == 1) {
                        waitCondition.notify_all();
                    }
                }
            });
        }
    }

    ~HamiltonianDFS() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    void addStartTask(Point start, std::function<void(const Path&)> onPathFound) {
        if (!isValidStart(start)) return;
        
        activeTasks.fetch_add(1);
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push([this, start, onPathFound] {
                Grid grid(N);
                grid.setVisited(start.x, start.y);
                Path currentPath;
                currentPath.add(start);
                dfs(start, grid, currentPath, onPathFound, 1);
            });
        }
        condition.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(waitMutex);
        waitCondition.wait(lock, [this] { return activeTasks.load() == 0; });
    }

    int getActiveTasks() const { return activeTasks.load(); }
    long long getTotalCount() const { return totalCount.load(); }

private:
    int N;
    int maxThreads;
    int maxParallelDepth;
    std::atomic<long long> totalCount;

    // Thread pool
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<int> activeTasks;
    std::mutex waitMutex;
    std::condition_variable waitCondition;
    std::mutex coutMutex;
    bool stop;

    bool isValidStart(Point p) {
        if (N % 2 != 0) {
            return (p.x + p.y) % 2 == 0;
        }
        return true;
    }

    int countFreeNeighbors(int x, int y, const Grid& grid) {
        int count = 0;
        static const int dx[] = {0, 0, 1, -1};
        static const int dy[] = {1, -1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx >= 0 && nx < N && ny >= 0 && ny < N && !grid.isVisited(nx, ny)) {
                count++;
            }
        }
        return count;
    }

    bool hasDeadEnds(const Grid& grid, Point current) {
        int target = N * N - 1;
        int visited = grid.countVisited();
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                if (!grid.isVisited(x, y)) {
                    int free = countFreeNeighbors(x, y, grid);
                    if (free < 2) {
                        if (visited < target) {
                            if (free < 1) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    void dfs(Point current, Grid& grid, Path& path, const std::function<void(const Path&)>& onPathFound, int depth) {
        if (path.points.size() == (size_t)(N * N)) {
            totalCount.fetch_add(1, std::memory_order_relaxed);
            onPathFound(path);
            return;
        }

        if (hasDeadEnds(grid, current)) return;

        static const int dx[] = {0, 0, 1, -1};
        static const int dy[] = {1, -1, 0, 0};

        std::vector<Point> nextPoints;
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            if (nx >= 0 && nx < N && ny >= 0 && ny < N && !grid.isVisited(nx, ny)) {
                nextPoints.push_back({nx, ny});
            }
        }

        for (size_t i = 0; i < nextPoints.size(); ++i) {
            Point next = nextPoints[i];
            
            // Adaptive parallel decomposition
            // If we have idle threads or we are at low depth, spawn new task
            bool shouldParallelize = (depth < maxParallelDepth);
            
            if (shouldParallelize && i > 0) {
                activeTasks.fetch_add(1);
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    // Copy current state for the new task
                    Grid newGrid = grid;
                    Path newPath = path;
                    newGrid.setVisited(next.x, next.y);
                    newPath.add(next);
                    tasks.push([this, next, newGrid, newPath, onPathFound, depth] () mutable {
                        dfs(next, newGrid, newPath, onPathFound, depth + 1);
                    });
                }
                condition.notify_one();
            } else {
                // Last option or depth limit exceeded - continue in current thread
                grid.setVisited(next.x, next.y);
                path.add(next);
                dfs(next, grid, path, onPathFound, depth + 1);
                // Backtrack
                path.points.pop_back();
                grid.clearVisited(next.x, next.y);
            }
        }
    }
};

#endif // DFS_H
