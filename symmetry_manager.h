#ifndef SYMMETRY_MANAGER_H
#define SYMMETRY_MANAGER_H

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include "core.h"
#include "transforms.h"

struct SymmetryInfo {
    Point point;
    std::string symmetryString; // Например, "H_V_M_T"
    std::vector<char> availableTransforms;
};

class SymmetryManager {
public:
    static bool isInSearchZone(Point p, int N) {
        // Условие 1/8: 0 <= x <= floor((N-1)/2) и x <= y <= floor((N-1)/2)
        // Но так как у нас x,y от 0 до N-1, и мы хотим покрыть всё:
        // x <= y (нижний треугольник относительно диагонали T)
        // y <= (N-1)/2 (верхняя половина сетки)
        // x <= (N-1)/2 (левая половина сетки)
        int mid = (N - 1) / 2;
        return (p.x >= 0 && p.x <= mid && p.y >= p.x && p.y <= mid);
    }

    static SymmetryInfo getSymmetryInfo(Point p, int N) {
        SymmetryInfo info;
        info.point = p;
        
        std::set<char> transforms;
        
        // Проверяем, какие трансформации дают УНИКАЛЬНЫЕ стартовые точки
        // (те, которые не в зоне поиска или просто другие).
        // Согласно ТЗ, мы пишем в файл список симметрий, которыми можно получить другие точки.
        
        Point ph = Transforms::horizontal(p, N);
        Point pv = Transforms::vertical(p, N);
        Point pm = Transforms::mirror(p, N);
        Point pt = Transforms::transpose(p, N);

        if (!(ph == p)) transforms.insert('H');
        if (!(pv == p)) transforms.insert('V');
        if (!(pm == p)) transforms.insert('M');
        if (!(pt == p)) transforms.insert('T');

        // Генерируем строку для названия файла
        std::string s;
        if (transforms.count('H')) s += "H_";
        if (transforms.count('V')) s += "V_";
        if (transforms.count('M')) s += "M_";
        if (transforms.count('T')) s += "T_";
        
        if (s.empty()) s = "N"; // No symmetry
        else if (s.back() == '_') s.pop_back();

        info.symmetryString = s;
        for (char c : transforms) info.availableTransforms.push_back(c);
        
        return info;
    }

    static std::string getFileName(Point p, int N) {
        SymmetryInfo info = getSymmetryInfo(p, N);
        return std::to_string(p.x) + "-" + std::to_string(p.y) + "_" + info.symmetryString + ".txt";
    }
};

#endif // SYMMETRY_MANAGER_H
