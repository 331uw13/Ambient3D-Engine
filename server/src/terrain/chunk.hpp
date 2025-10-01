#ifndef AMBIENT3D_SERVER_CHUNK_HPP
#define AMBIENT3D_SERVER_CHUNK_HPP

#include "noise_generator.hpp"
#include "server_config.hpp"
#include "chunk_pos.hpp"
#include "ivec2.hpp"

namespace AM {

    // grid which contains ids for example 0 is nothing, 1 is tree, 2 is rock.. etc

    class Chunk {
        public:

            AM::ChunkPos  pos;

            // One dimensional array which 
            // contains (chunk_size * chunk_size) number of height points.
            float* height_points { NULL };
            float get_height_at(const AM::iVec2& local_coords);

            void  generate(
                    const AM::ServerCFG& server_cfg,
                    const AM::NoiseGen& noise_gen,
                    const AM::ChunkPos& chunk_pos,
                    int seed);
            
            bool  unload();
            bool  is_loaded() const { return m_loaded; }

        private:

            bool      m_loaded { false };
            uint8_t   m_chunk_size { 0 };
    };

};



#endif
