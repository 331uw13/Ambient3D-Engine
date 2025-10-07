#ifndef AMBIENT3D_SERVER_RAY_HPP
#define AMBIENT3D_SERVER_RAY_HPP

#include "geometry/rect.hpp"


namespace AM {

    struct RayHitResult {
        bool      hit       { false };
        float     distance  { 0.0f };
        AM::Vec3  normal;
        AM::Vec3  point;
    };

    struct Ray {

        AM::Vec3 pos;
        AM::Vec3 direction;
        
        Ray(){}
        Ray(const AM::Vec3& _pos, const AM::Vec3& _direction)
            : pos(_pos), direction(_direction) {}

        RayHitResult triangle_intersection(const AM::Triangle& triangle);
        RayHitResult rectangle_intersection(const AM::Rect& rect);

    };

};


#endif
