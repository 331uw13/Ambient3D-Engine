#ifndef AMBIENT3D_SERVER_RECTANGLE_HPP
#define AMBIENT3D_SERVER_RECTANGLE_HPP

#include "triangle.hpp"

namespace AM {
    struct Rect {
        AM::Triangle tA;
        AM::Triangle tB;

        Rect(){}
        Rect(const Triangle& a, const Triangle& b)
            : tA(a), tB(b) {}
    };
};



#endif
