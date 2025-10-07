#include <cstdio>
#include <cmath>

#include "terrain.hpp"
#include "shared/include/ray.hpp"
#include "../server.hpp"


void AM::Terrain::add_chunk(const AM::Chunk& chunk) {
    auto inserted = this->chunk_map.insert(std::make_pair(chunk.pos, chunk)).first;
    if(inserted == this->chunk_map.end()) {
        fprintf(stderr, "ERROR! %s: Failed to add chunk (X=%i, Z=%i)\n",
                __func__, chunk.pos.x, chunk.pos.z);
        this->chunk_map_mutex.unlock();
        return;
    }
}

void AM::Terrain::remove_chunk(const AM::Chunk& chunk) {
    auto chunk_it = this->chunk_map.find(chunk.pos);
    if(chunk_it != this->chunk_map.end()) {
        chunk_it->second.unload();
        this->chunk_map.erase(chunk_it);
    }
}


void AM::Terrain::delete_terrain() {
    size_t num_unloaded = 0;
    for(auto it = this->chunk_map.begin(); it != this->chunk_map.end(); ++it) {
        it->second.unload();
        num_unloaded++;
    }

    printf("[WORLD_GEN]: Unloaded %li chunks\n", num_unloaded);
}

AM::ChunkPos AM::Terrain::get_chunk_pos(float world_x, float world_z) {
    return AM::ChunkPos(
            (int)floor(world_x / (m_server->config.chunk_size * m_server->config.chunk_scale)),
            (int)floor(world_z / (m_server->config.chunk_size * m_server->config.chunk_scale)));
}


AM::iVec2 AM::Terrain::get_chunk_local_coords(float world_x, float world_z) {
    int rx = (int)floor(world_x / m_server->config.chunk_scale);
    int rz = (int)floor(world_z / m_server->config.chunk_scale);

    // Return absolute x/z % chunk_size
    return AM::iVec2(
            (rx % m_server->config.chunk_size + m_server->config.chunk_size) % m_server->config.chunk_size,
            (rz % m_server->config.chunk_size + m_server->config.chunk_size) % m_server->config.chunk_size);
}

            
AM::Vec3 AM::Terrain::get_chunk_vertex(float world_x, float world_z, AM::iVec2 offset) {
    world_x += offset.x * m_server->config.chunk_scale;
    world_z += offset.y * m_server->config.chunk_scale;

    auto chunk_search = this->chunk_map.find(this->get_chunk_pos(world_x, world_z));
    if(chunk_search == this->chunk_map.end()) {
        //fprintf(stderr, "ERROR! %s: Chunk not found. worldXZ = (%0.2f, %0.2f)\n", 
        //        __func__, world_x, world_z);
        return AM::Vec3();
    }

    AM::Chunk& chunk = chunk_search->second;
    
    AM::iVec2 chunk_local_coords = this->get_chunk_local_coords(world_x, world_z);
    const float chunk_vertex_y = chunk.get_height_at(chunk_local_coords);
    const float chunk_vertex_x = floor(world_x / m_server->config.chunk_scale) * m_server->config.chunk_scale;
    const float chunk_vertex_z = floor(world_z / m_server->config.chunk_scale) * m_server->config.chunk_scale;

    return AM::Vec3(chunk_vertex_x, chunk_vertex_y, chunk_vertex_z);
}

AM::Rect AM::Terrain::get_chunk_meshrect(float world_x, float world_z, AM::iVec2 offset) {
    world_x += offset.x * m_server->config.chunk_scale;
    world_z += offset.y * m_server->config.chunk_scale;

    AM::Vec3 vertex_A = this->get_chunk_vertex(world_x, world_z);
    AM::Vec3 vertex_B = this->get_chunk_vertex(world_x, world_z, AM::iVec2(1, 0));
    AM::Vec3 vertex_C = this->get_chunk_vertex(world_x, world_z, AM::iVec2(1, 1));
    AM::Vec3 vertex_D = this->get_chunk_vertex(world_x, world_z, AM::iVec2(0, 1));

    // This could be optimized to save memory because only 4 vertices are needed, 
    // 2 of them are shared between the 2 triangles.
    // It will waste some memory for now because it feels more easy.

    return AM::Rect(
                AM::Triangle(
                    vertex_A,
                    vertex_B,
                    vertex_C
                ),
                AM::Triangle(
                    vertex_D,
                    vertex_A,
                    vertex_C
                )   
            );
}

float AM::Terrain::get_surface_level(const AM::Vec3& world_pos) {
    
    AM::Ray ray(
            world_pos + AM::Vec3(0.0f, 10.0f, 0.0f), // Ray origin
            AM::Vec3(0.0f, -1.0f, 0.0f));            // Ray direction

    AM::RayHitResult hit_result = {};
    AM::Rect rect = {};

    // Check the most likely hit first.
    rect = this->get_chunk_meshrect(world_pos.x, world_pos.z, AM::iVec2(0, 0));
    hit_result = ray.rectangle_intersection(rect);
   
    if(!hit_result.hit) {
        // The rectangle was not hit under the player, search bigger area.
        // Resulting search area will be (AREA_HALF*2) * (AREA_HALF*2)
        constexpr int AREA_HALF = 1;

        for(int z = -AREA_HALF; z <= AREA_HALF; z++) {
            for(int x = -AREA_HALF; x <= AREA_HALF; x++) {
                rect = this->get_chunk_meshrect(world_pos.x, world_pos.z, AM::iVec2(x, z));
                
                hit_result = ray.triangle_intersection(rect.tA);
                if(hit_result.hit) {
                    goto loop_exit;
                }

                hit_result = ray.triangle_intersection(rect.tB);
                if(hit_result.hit) {
                    goto loop_exit;
                }
            }
        }
    }

loop_exit:

    return hit_result.point.y;
}


void AM::Terrain::foreach_chunk_nearby(float world_x, float world_z, int distance,
        std::function<void(const AM::Chunk*, const AM::ChunkPos&)> callback) {

    AM::ChunkPos origin_chunk_pos = this->get_chunk_pos(world_x, world_z);

    /*
    printf("%s: (%i, %i)\n",
            __func__, origin_chunk_pos.x, origin_chunk_pos.z);
    */

    const int area_half = distance / 2;
    for(int chunk_lZ = -area_half; chunk_lZ <= area_half; chunk_lZ++) {
        for(int chunk_lX = -area_half; chunk_lX <= area_half; chunk_lX++) { 
            AM::ChunkPos chunk_pos {
                origin_chunk_pos.x + chunk_lX,
                origin_chunk_pos.z + chunk_lZ
            };

            auto chunk_it = this->chunk_map.find(chunk_pos);

            if(chunk_it != this->chunk_map.end()) {
                callback(&chunk_it->second, chunk_pos);
            }
            else {
                callback(NULL, chunk_pos);
            }
        }
    }
}


