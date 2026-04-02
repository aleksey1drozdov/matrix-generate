#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "core.h"

class Transforms {
public:
    static Point horizontal(Point p, int N) {
        return {p.x, N - 1 - p.y};
    }

    static Point vertical(Point p, int N) {
        return {N - 1 - p.x, p.y};
    }

    static Point mirror(Point p, int N) {
        return {N - 1 - p.x, N - 1 - p.y};
    }

    static Point transpose(Point p, int N) {
        return {p.y, p.x};
    }

    static Path transformPath(const Path& path, int N, char type) {
        Path newPath;
        for (const auto& p : path.points) {
            switch (type) {
                case 'H': newPath.add(horizontal(p, N)); break;
                case 'V': newPath.add(vertical(p, N)); break;
                case 'M': newPath.add(mirror(p, N)); break;
                case 'T': newPath.add(transpose(p, N)); break;
                default: newPath.add(p);
            }
        }
        return newPath;
    }
};

#endif // TRANSFORMS_H
