#include <cstdio>

#include "chunk.hpp"
#include "shared/include/perlin_noise.hpp"


void AM::Chunk::generate(
        const AM::ServerCFG& server_cfg,
        const AM::NoiseGen& noise_gen,
        const AM::ChunkPos& chunk_pos,
        int seed
){
    m_chunk_size = (uint8_t)server_cfg.chunk_size;
    this->pos = chunk_pos;

    const size_t num_points = (server_cfg.chunk_size+1) * (server_cfg.chunk_size+1);
    this->height_points = new float[num_points];

    size_t point_index = 0;
    for(int local_z = 0; local_z <= server_cfg.chunk_size; local_z++) {
        for(int local_x = 0; local_x <= server_cfg.chunk_size; local_x++) {

            const float point_x = (float)(local_x + chunk_pos.x * m_chunk_size) / 10.0f;
            const float point_z = (float)(local_z + chunk_pos.z * m_chunk_size) / 10.0f;

            this->height_points[point_index] = noise_gen.get_noise(point_x, point_z); 
            point_index++;
        }
    }

    m_loaded = true;
}
            
float AM::Chunk::get_height_at(const AM::iVec2& local_coords) {
    if(!this->height_points) {
        return 0.0f;
    }
    size_t idx = local_coords.y * (m_chunk_size+1) + local_coords.x;
    if(idx >= ((m_chunk_size+1)*(m_chunk_size+1))) {
        return 0.0f;
    }
    return this->height_points[idx];
}

bool AM::Chunk::unload() {
    if(!m_loaded) {
        fprintf(stderr, "ERROR! Trying to unload not loaded chunk.\n");
        return false;
    }

    delete[] height_points;
    height_points = NULL;
    m_loaded = false;
    return true;
}


