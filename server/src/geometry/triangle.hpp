#ifndef AMBIENT3D_SERVER_TRIANGLE_HPP
#define AMBIENT3D_SERVER_TRIANGLE_HPP

#include "../vec3.hpp"

namespace AM {
    struct Triangle {
        AM::Vec3 vA { 0, 0, 0 };
        AM::Vec3 vB { 0, 0, 0 };
        AM::Vec3 vC { 0, 0, 0 };

        Triangle(){}
        Triangle(const AM::Vec3& a, const AM::Vec3& b, const AM::Vec3& c)
            : vA(a), vB(b), vC(c) {}
    };
};


#endif
