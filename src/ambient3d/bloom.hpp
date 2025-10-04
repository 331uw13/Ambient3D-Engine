#ifndef AMBIENT3D_BLOOM_HPP
#define AMBIENT3D_BLOOM_HPP

#include "raylib.h"



namespace AM {
    namespace Bloom {
        void create_sample_targets(int num_samples, float scale_factor);
        void unload_sample_targets();
        
        void render(
                RenderTexture2D* result_out, 
                RenderTexture2D  scene,
                const Shader* treshold_shader,
                const Shader* upsample_filter_shader,
                const Shader* downsample_filter_shader);
    
    };
};




#endif
