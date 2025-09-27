#ifndef AMBIENT3D_CHUNK_HPP
#define AMBIENT3D_CHUNK_HPP

#include <cstddef>

#include "raylib.h"
#include "chunk_pos.hpp"


namespace AM {

    class Chunk {
        public:
            
            AM::ChunkPos pos;
            float* height_points;

            void load(float* points, size_t points_sizeb, int chunk_size, float scale, Material* mat);
            void unload();
            bool is_loaded() { return m_loaded; }

            void render();


        private:

            bool       m_loaded;
            Mesh       m_mesh;
            Material*  m_material;
            int        m_chunk_size;
            float      m_get_height_at_local(int x, int z);
            float      m_scale;
    };

};



#endif
