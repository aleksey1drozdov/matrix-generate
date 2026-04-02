#ifndef CORE_H
#define CORE_H

#include <vector>
#include <string>
#include <cstdint>
#include <sstream>
#include <iostream>

struct Point {
    int x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    std::string toString() const {
        return std::to_string(x) + "," + std::to_string(y);
    }
};

struct Path {
    std::vector<Point> points;

    void add(Point p) {
        points.push_back(p);
    }

    std::string toString() const {
        std::stringstream ss;
        for (size_t i = 0; i < points.size(); ++i) {
            ss << points[i].x << "," << points[i].y;
            if (i < points.size() - 1) ss << ",";
        }
        ss << ";" << countTurns();
        return ss.str();
    }

    std::string toSemicolonString() const {
        std::stringstream ss;
        for (const auto& p : points) {
            ss << p.x << "," << p.y << ";";
        }
        return ss.str();
    }

    std::string toSimplifiedString(int N) const {
        std::stringstream ss;
        for (size_t i = 0; i < points.size(); ++i) {
            int p = points[i].y * N + points[i].x + 1;
            ss << p;
            if (i < points.size() - 1) ss << ",";
        }
        ss << ";" << countTurns();
        return ss.str();
    }

    int countTurns() const {
        if (points.size() < 3) return 0;
        int turns = 0;
        for (size_t i = 1; i < points.size() - 1; ++i) {
            int dx1 = points[i].x - points[i - 1].x;
            int dy1 = points[i].y - points[i - 1].y;
            int dx2 = points[i + 1].x - points[i].x;
            int dy2 = points[i + 1].y - points[i].y;
            if (dx1 != dx2 || dy1 != dy2) {
                turns++;
            }
        }
        return turns;
    }
};

class Grid {
public:
    Grid(int n) : N(n), mask(0) {}

    void setVisited(int x, int y) {
        mask |= (1ULL << (y * N + x));
    }

    bool isVisited(int x, int y) const {
        return (mask & (1ULL << (y * N + x))) != 0;
    }

    void clearVisited(int x, int y) {
        mask &= ~(1ULL << (y * N + x));
    }

    bool allVisited() const {
        if (N * N >= 64) return mask == ~0ULL;
        return mask == ((1ULL << (N * N)) - 1);
    }

    int countVisited() const {
        return __builtin_popcountll(mask);
    }

    int getN() const { return N; }

    uint64_t getMask() const { return mask; }
    void setMask(uint64_t m) { mask = m; }

private:
    int N;
    uint64_t mask; // Поддерживает до 8x8. Для больших N потребуется __int128 или массив.
};

#endif // CORE_H
