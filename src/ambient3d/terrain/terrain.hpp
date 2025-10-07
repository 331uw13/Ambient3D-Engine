#ifndef AMBIENT3D_TERRAIN_HPP
#define AMBIENT3D_TERRAIN_HPP

#include <unordered_map>
#include <mutex>
#include <array>
#include <vector>
#include <cstddef>

#include "chunk.hpp"
#include "raylib.h"
#include "shared/include/chunk_pos.hpp"
#include "shared/include/geometry/rect.hpp"
#include "shared/include/networking_agreements.hpp"
#include "shared/include/ivec2.hpp"

namespace AM {

    enum ChunkMaterial : int {
        CM_GRASS=0,

        CM_NUM_MATERIALS
    };


    class State;
    class Terrain {
        public:

            std::unordered_map<AM::ChunkPos, AM::Chunk> chunk_map;

            // 'allocate_regenbuf' is called from 
            // "./network/network.cpp" handle_tcp_packet(size_t). case AM::PacketID::SERVER_CONFIG
            // because the server and client will agree how many bytes
            // the max uncompressed size for chunk data is.
            void allocate_regenbuf(size_t num_bytes);
            void free_regenbuf();
            void create_chunk_materials();
            
            void add_chunkdata_to_queue(char* compressed_data, size_t sizeb);
           

            // IMPORTANT NOTE: must be called from main thread.
            void update_chunkdata_queue();
            void unload_all_chunks();
            void unload_materials();
            void render();
 
            AM::ChunkPos get_chunk_pos       (float world_x, float world_z);
            AM::Rect     get_chunk_meshrect  (float world_x, float world_z, AM::iVec2 offset = {});
            AM::Vec3     get_chunk_vertex    (float world_x, float world_z, AM::iVec2 offset = {});
            float        get_surface_level   (const Vector3& world_pos);


            // Returns X and Y in range of 0 to chunk size
            AM::iVec2 get_chunk_local_coords(float world_x, float world_z); 

            bool chunkpos_in_renderdist(const AM::ChunkPos& origin, const AM::ChunkPos& chunk_pos);

            void set_engine_state(AM::State* engine_state) {
                m_engine = engine_state;
            }

        private:

            struct mChunkData { // Compressed chunk data
                mChunkData() {}
                std::array<char, AM::MAX_PACKET_SIZE> bytes;
                size_t                                num_bytes { 0 };
            };

            std::mutex              m_chunkdata_queue_mutex;
            std::vector<mChunkData> m_chunkdata_queue;

            // Used for decompressing chunk data.
            char* m_chunkdata_regenbuf { NULL };
            size_t m_chunkdata_regenbuf_memsize { 0 };
            size_t m_chunkdata_regenbuf_size { 0 };
  
            std::array<Material, AM::ChunkMaterial::CM_NUM_MATERIALS>
                m_chunk_materials;

            AM::State* m_engine;
    };
};



#endif
