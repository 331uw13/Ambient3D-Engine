#include <cstring>
#include <cstdio>

#include "chunk_data.hpp"


void AM::ChunkData::clear() {
    if(!this->data) { return; }
    if(!m_data_sizeb) { return; }
    memset(this->data, 0, m_data_sizeb);
    m_data_sizeb = 0;
}

void AM::ChunkData::write_bytes(void* bytes, size_t sizeb) {
    if(!data) { 
        printf("ERROR! %s: No memory allocated for ChunkData?\n", __func__);
        return;
    }
    if((m_data_sizeb + sizeb) >= m_memsizeb) {
        fprintf(stderr, "ERROR! %s: Trying to write too much chunk data!\n",
                __func__);
        return;
    }

    memmove((void*)(this->data + m_data_sizeb), bytes, sizeb);
    m_data_sizeb += sizeb;
}


