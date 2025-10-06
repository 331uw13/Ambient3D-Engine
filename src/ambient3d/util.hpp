#ifndef AMBIENT3D_UTIL_HPP
#define AMBIENT3D_UTIL_HPP

#include <initializer_list>
#include <string>
#include <cstdint>

#include "raylib.h"
#include "raymath.h"


namespace AM {
    class State;
};


namespace AMutil {

    float  normalize(float t, float min, float max);
    float  lerp(float min, float max, float t);
    float  map(float t, float src_min, float src_max, float dst_min, float dst_max);
    int    randomi(int min, int max, int64_t* seed);
    float  randomf(float min, float max, int64_t* seed);

    template<typename T>
    inline void clamp(T& v, T min, T max) {
        if(v < min) { v = min; }
        else if(v > max) { v = max; }
    }

    std::string combine_constchar(std::initializer_list<const char*> list);
    std::string combine_files(std::initializer_list<const char*> filenames);

    // Returns Yaw(X) and Pitch(Y) rotations.
    Vector2 get_rotation_towards(const Vector3& a, const Vector3& b);
            
    // If 'shader_idx' < 0: Default shader is used.
    // If dimensions are negative the corresponding texture dimensions are used.
    void resample_texture(
            RenderTexture2D to,
            RenderTexture2D from,
            const Shader* shader,

            int src_width = -1,
            int src_height = -1,

            int dst_width = -1,
            int dst_height = -1
            );
};



#endif

