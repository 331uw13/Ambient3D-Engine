#ifndef AMBIENT3D_FOG_HPP
#define AMBIENT3D_FOG_HPP


namespace AM {

    struct Fog {
        float density;

        // Color range is 0.0 to 1.0
        float color_r;
        float color_g;
        float color_b;

        void set_color(float r, float g, float b) {
            color_r = r;
            color_g = g;
            color_b = b;
        }
    };

};


#endif
