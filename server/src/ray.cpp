#include "ray.hpp"

#define E 0.000001f

// ***********************************************************
// Copied from Raylib "src/rmodels.c" 'GetRayCollisionTriangle'   
// ***********************************************************
AM::RayHitResult AM::Ray::triangle_intersection(const AM::Triangle& triangle) {
    AM::RayHitResult result = {};

    AM::Vec3 edge1 = {};
    AM::Vec3 edge2 = {};
    AM::Vec3 p = {};
    AM::Vec3 q = {};
    AM::Vec3 tvA_to_origin = {};
    float det, inv_det, u, v, t;

    // Find vectors for two edges sharing vertex A (vA)
    edge1 = triangle.vB - triangle.vA;
    edge2 = triangle.vC - triangle.vA;

    // Begin calculating determinant - also used to calculate u parameter.
    p = this->direction.cross(edge2);

    // If deteminant is near zero, ray lies in plane of triangle or ray is parallel to plane
    // of triangle.
    det = edge1.dot(p);

    // Avoid culling
    if((det > -E) && (det < E)) {
        return result;
    }

    inv_det = 1.0f / det;

    // Calculate distance from vA to ray origin.
    tvA_to_origin = this->pos - triangle.vA;

    // Calculate u parameter and test bound.
    u = tvA_to_origin.dot(p) * inv_det;

    if((u < 0.0f) || (u > 1.0f)) {
        return result; // Intersection lies outside the triangle.
    }


    // Prepare to test v parameter.
    q = tvA_to_origin.cross(edge1);

    // Calculate v parameter and test bound.
    v = this->direction.dot(q) * inv_det;
    
    if((v < 0.0f) || (u > 1.0f)) {
        return result; // Intersection lies outside the triangle.
    }


    t = edge2.dot(q) * inv_det;


    if(t > E) {
        result.hit = true;
        result.distance = t;
        result.normal = edge1.cross(edge2).normalize();
        result.point = this->pos + (this->direction * t);
    }


    return result;
}

        
AM::RayHitResult AM::Ray::rectangle_intersection(const AM::Rect& rect) {
    AM::RayHitResult result = {};

    result = this->triangle_intersection(rect.tA);
    if(result.hit) {
        return result;
    }

    result = this->triangle_intersection(rect.tB);
    if(result.hit) {
        return result;
    }

    return result;
}



