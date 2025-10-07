#ifndef AMBIENT3D_SERVER_TERRAIN_HPP
#define AMBIENT3D_SERVER_TERRAIN_HPP

#include <mutex>
#include <unordered_map>
#include <functional>

#include "chunk.hpp"
#include "shared/include/ivec2.hpp"
#include "shared/include/geometry/rect.hpp"


namespace AM {

    class Server;

    class Terrain {
        public:
            std::mutex                                  chunk_map_mutex;
            std::unordered_map<AM::ChunkPos, AM::Chunk> chunk_map;
            
            AM::NoiseGen noise_gen;


            void add_chunk(const AM::Chunk& chunk);
            void remove_chunk(const AM::Chunk& chunk);
            void delete_terrain();

            AM::ChunkPos get_chunk_pos       (float world_x, float world_z);
            AM::Rect     get_chunk_meshrect  (float world_x, float world_z, AM::iVec2 offset = {});
            AM::Vec3     get_chunk_vertex    (float world_x, float world_z, AM::iVec2 offset = {});
            float        get_surface_level   (const AM::Vec3& world_pos);

            // Returns X and Y in range of 0 to chunk size
            AM::iVec2 get_chunk_local_coords(float world_x, float world_z); 



            void foreach_chunk_nearby(float world_x, float world_z, int distance, 
                    std::function<void(const AM::Chunk*, const AM::ChunkPos&)> callback);
            
            void set_server(AM::Server* server) { m_server = server; }

        private:

            AM::Server* m_server;

    };


}

#endif
