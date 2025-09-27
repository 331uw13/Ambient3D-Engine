#include <cstdio>

#include "noise_generator.hpp"
#include "perlin_noise.hpp"


static float map(float t, float src_min, float src_max, float dst_min, float dst_max) {
    return (t - src_min) * (dst_max - dst_min) / (src_max - src_min) + dst_min;
}


const float AM::NoiseGen::get_noise(float x, float z) const {


    // Base noise for the terrain.

    float base = perlin_noise_2D(x * m_cfg.base_frq, z * m_cfg.base_frq) * m_cfg.base_amp;
    float base_detail_alt = 2.0*perlin_noise_2D(x * m_cfg.base_detail_alt, z * m_cfg.base_detail_alt);
    float base_detail = perlin_noise_2D(x * m_cfg.base_detail_frq, z * m_cfg.base_detail_frq) 
        * (m_cfg.base_detail_amp * base_detail_alt);
    
    base += base_detail;


    // Mountains.
    
    float mountains = 0;
    float mountain_frq = m_cfg.mountain_frq;
    float mountain_amp = m_cfg.mountain_amp;
    float mountain_alt = m_cfg.mountain_alt;
    for(int i = 0; i < m_cfg.mountain_iterations; i++) {
        mountains += 
            (perlin_noise_2D(x * mountain_frq, z * mountain_frq) * mountain_amp)
          * 2.0*perlin_noise_2D(x * mountain_alt, z * mountain_alt);
    
        mountain_frq += m_cfg.mountain_iteration_frq_add;
        mountain_amp += m_cfg.mountain_iteration_amp_add;
        mountain_alt += m_cfg.mountain_iteration_alt_add;
    }


    return base + mountains;
}



