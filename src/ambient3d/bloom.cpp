#include <stdio.h>

#include "bloom.hpp"
#include "ambient3d.hpp"
#include "util.hpp"



namespace {
    static std::vector<RenderTexture2D> bloom_samples;
};

        
void AM::Bloom::create_sample_targets(int num_samples, float scale_factor) {
    
    int sample_res_X = GetScreenWidth();
    int sample_res_Y = GetScreenHeight();

    for(int i = 0; i < num_samples; i++) {
        RenderTexture2D target = LoadRenderTexture(sample_res_X, sample_res_Y);
        SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

        bloom_samples.push_back(target);
        sample_res_X = round((float)sample_res_X * scale_factor);
        sample_res_Y = round((float)sample_res_Y * scale_factor);
    }
}

void AM::Bloom::unload_sample_targets() {
    for(size_t i = 0; i < bloom_samples.size(); i++) {
        UnloadRenderTexture(bloom_samples[i]);
    }
}

void AM::Bloom::render(
        RenderTexture2D* result_out, 
        RenderTexture2D scene,
        const Shader* treshold_shader,
        const Shader* upsample_filter_shader,
        const Shader* downsample_filter_shader
){

    // Get bloom treshold texture.
    AMutil::resample_texture(
            /* TO */   bloom_samples[0],
            /* FROM */ scene,
            treshold_shader);

    size_t p = 0;

    // Scale down and filter the texture each step.
    for(size_t i = 1; i < bloom_samples.size(); i++) {
        p = i - 1; // Previous sample.

        AMutil::resample_texture(
                /* TO */   bloom_samples[i],
                /* FROM */ bloom_samples[p],
                downsample_filter_shader);
    }

    // Scale up and filter the texture each step.
    for(size_t i = bloom_samples.size()-2; i > 0; i--) {
        p = i + 1; // Previous sample.

        AMutil::resample_texture(
                /* TO */   bloom_samples[i],
                /* FROM */ bloom_samples[p],
                upsample_filter_shader);
    }


    // Now the result is ready.

    AMutil::resample_texture(
            /* TO */   *result_out,
            /* FROM */ bloom_samples[1],
            NULL);
}





