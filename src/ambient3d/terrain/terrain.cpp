#include <cstring>
#include <lz4.h>
#include <cstdio>

#include "terrain.hpp"
#include "ray.hpp"
#include "../ambient3d.hpp"


void AM::Terrain::allocate_regenbuf(size_t num_bytes) {
    if(!m_chunkdata_regenbuf) {
        m_chunkdata_regenbuf = new char[num_bytes];
        m_chunkdata_regenbuf_memsize = num_bytes;
        printf("[TERRAIN]: Allocated chunkdata regen buffer (%li bytes)\n", num_bytes);
    }
}

void AM::Terrain::free_regenbuf() {
    if(m_chunkdata_regenbuf) {
        delete[] m_chunkdata_regenbuf;
        m_chunkdata_regenbuf = NULL;
    }
}
            
void AM::Terrain::create_chunk_materials() {
    // In the future this can be changed easily to work with multiple materials for chunks.
    // When implemeting biomes.

    Material& grass_material = m_chunk_materials[AM::ChunkMaterial::CM_GRASS];
    grass_material = LoadMaterialDefault();
    grass_material.shader = m_engine->shaders[AM::ShaderIDX::DEFAULT];

}

void AM::Terrain::update_chunkdata_queue() {
    if(!m_chunkdata_regenbuf) {
        return;
    }

    SetTraceLogLevel(LOG_NONE);
    
    const int chunk_size = m_engine->net->server_cfg.chunk_size;
    const size_t chunk_height_points_sizeb = ((chunk_size+1)*(chunk_size+1)) * sizeof(float);

    m_chunkdata_queue_mutex.lock();
        
    // TODO: request resend if something fails

    for(size_t i = 0; i < m_chunkdata_queue.size(); i++) {
        mChunkData* chunkdata = &m_chunkdata_queue[i]; // <- Compressed data.
        if(chunkdata->num_bytes >= m_chunkdata_regenbuf_memsize) {
            fprintf(stderr, "ERROR! %s: Received chunk data has unexpectedly too many bytes (%li)."
                    " Has allocated %li bytes\n",
                    __func__, chunkdata->num_bytes, m_chunkdata_regenbuf_memsize);
            continue;
        }

        // chunk_x + chunk_z = 4*2 bytes.
        if(chunkdata->num_bytes <= (sizeof(int)*2)) {
            fprintf(stderr, "ERROR! %s: Received chunk data is too small to be valid. Got %li bytes\n",
                    __func__, chunkdata->num_bytes);
            continue;
        }

        if(m_chunkdata_regenbuf_memsize < chunkdata->num_bytes) {
            fprintf(stderr, "ERROR! %s: Chunk data has too many bytes to be handled.\n",
                    __func__);
            continue;
        }

        // Clear previously added data.
        if(m_chunkdata_regenbuf_size > 0) {
            memset(m_chunkdata_regenbuf, 0, m_chunkdata_regenbuf_size);
            m_chunkdata_regenbuf_size = 0;
        }

        ssize_t decompressed_size
            = LZ4_decompress_safe(
                    chunkdata->bytes.data(), // Source.
                    m_chunkdata_regenbuf,    // Destination.
                    chunkdata->num_bytes,    // Source size.
                    m_chunkdata_regenbuf_memsize); // Destination max size.
        
        if(decompressed_size <= 0) {
            fprintf(stderr, "ERROR! %s: Failed to decompress chunk data.\n",
                    __func__);
            continue;
        }


        size_t byte_offset = 0;
        while(byte_offset < (size_t)decompressed_size) {

            AM::ChunkPos chunk_pos;


            memmove(&chunk_pos.x, &m_chunkdata_regenbuf[byte_offset], sizeof(int));
            byte_offset += sizeof(int);
            
            memmove(&chunk_pos.z, &m_chunkdata_regenbuf[byte_offset], sizeof(int));
            byte_offset += sizeof(int);
            

            // Server is going to keep track of what chunks it has send to client
            // but its good idea to check here too.
            auto chunk_search = this->chunk_map.find(chunk_pos);
            if(chunk_search != this->chunk_map.end()) {
                fprintf(stderr, "WARNING! %s: Server sent chunk which is already loaded\n",
                        __func__);
                break; // TODO: Dont break here.
            }

            auto chunk = this->chunk_map.insert(std::make_pair(chunk_pos, AM::Chunk{})).first;

            chunk->second.pos = chunk_pos;
            chunk->second.load(
                    (float*)&m_chunkdata_regenbuf[byte_offset],
                    chunk_height_points_sizeb,
                    m_engine->net->server_cfg.chunk_size,
                    m_engine->net->server_cfg.chunk_scale,
                    &m_chunk_materials[AM::ChunkMaterial::CM_GRASS]);

            byte_offset += chunk_height_points_sizeb;
        }
    }

    m_chunkdata_queue.clear();
    m_chunkdata_queue_mutex.unlock();
    
    SetTraceLogLevel(LOG_ALL);
}

void AM::Terrain::add_chunkdata_to_queue(char* compressed_data, size_t sizeb) {
    m_chunkdata_queue_mutex.lock();
    
    if(!compressed_data || (sizeb == 0)) {
        return;
    }
    
    mChunkData& data_added = m_chunkdata_queue.emplace_back();

    memmove(data_added.bytes.data(), compressed_data, sizeb);
    data_added.num_bytes = sizeb;
    m_chunkdata_queue_mutex.unlock();
}
            
void AM::Terrain::unload_all_chunks() {
    SetTraceLogLevel(LOG_NONE);
    size_t num_chunks = 0;
    for(auto it = this->chunk_map.begin(); it != this->chunk_map.end(); ++it) {
        it->second.unload();
        num_chunks++;
    }
    printf("[TERRAIN]: Unloaded %li chunks\n", num_chunks);
    SetTraceLogLevel(LOG_ALL);
}

void AM::Terrain::unload_materials() {
    for(size_t i = 0; i < m_chunk_materials.size(); i++) {
        UnloadMaterial(m_chunk_materials[i], RL_DONT_UNLOAD_MAT_SHADER);
    }
}
            
void AM::Terrain::render() {
    for(auto it = this->chunk_map.begin(); it != this->chunk_map.end(); ++it) {
        it->second.render();
    }
}
            
bool AM::Terrain::chunkpos_in_renderdist(const AM::ChunkPos& origin, const AM::ChunkPos& chunk_pos) {

    int abs_x = abs(origin.x - chunk_pos.x);
    int abs_z = abs(origin.z - chunk_pos.z);

    if(abs_x > m_engine->config.render_distance) {
        return false;
    }
    if(abs_z > m_engine->config.render_distance) {
        return false;
    }

    return true;
}

AM::ChunkPos AM::Terrain::get_chunk_pos(float world_x, float world_z) {
    return AM::ChunkPos(
            (int)floor(world_x / (m_engine->net->server_cfg.chunk_size * m_engine->net->server_cfg.chunk_scale)),
            (int)floor(world_z / (m_engine->net->server_cfg.chunk_size * m_engine->net->server_cfg.chunk_scale)));
}


AM::iVec2 AM::Terrain::get_chunk_local_coords(float world_x, float world_z) {
    const int   chunk_size = m_engine->net->server_cfg.chunk_size;
    const float chunk_scale = m_engine->net->server_cfg.chunk_scale;

    int rx = (int)floor(world_x / chunk_scale);
    int rz = (int)floor(world_z / chunk_scale);

    // Return absolute x/z % chunk_size
    return AM::iVec2(
            (rx % chunk_size + chunk_size) % chunk_size,
            (rz % chunk_size + chunk_size) % chunk_size);
}

            
AM::Vec3 AM::Terrain::get_chunk_vertex(float world_x, float world_z, AM::iVec2 offset) {
    const float chunk_scale = m_engine->net->server_cfg.chunk_scale;

    world_x += offset.x * chunk_scale;
    world_z += offset.y * chunk_scale;

    auto chunk_search = this->chunk_map.find(this->get_chunk_pos(world_x, world_z));
    if(chunk_search == this->chunk_map.end()) {
        return AM::Vec3();
    }

    AM::Chunk& chunk = chunk_search->second;    
    AM::iVec2 chunk_local_coords = this->get_chunk_local_coords(world_x, world_z);

    const float chunk_vertex_y = chunk.get_height_at(chunk_local_coords);
    const float chunk_vertex_x = floor(world_x / chunk_scale) * chunk_scale;
    const float chunk_vertex_z = floor(world_z / chunk_scale) * chunk_scale;

    return AM::Vec3(chunk_vertex_x, chunk_vertex_y, chunk_vertex_z);
}

AM::Rect AM::Terrain::get_chunk_meshrect(float world_x, float world_z, AM::iVec2 offset) {
    const float chunk_scale = m_engine->net->server_cfg.chunk_scale;
    world_x += offset.x * chunk_scale;
    world_z += offset.y * chunk_scale;

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

float AM::Terrain::get_surface_level(const Vector3& world_pos) {
    
    AM::Ray ray(
            AM::Vec3(world_pos.x, world_pos.y+10.0f, world_pos.z), // Ray origin
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



