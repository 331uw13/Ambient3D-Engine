#include <cstring>
#include <lz4.h>
#include <cstdio>

#include "terrain.hpp"
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

