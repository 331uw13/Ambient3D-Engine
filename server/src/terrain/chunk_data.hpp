#ifndef AMBIENT3D_SERVER_CHUNK_DATA_HPP
#define AMBIENT3D_SERVER_CHUNK_DATA_HPP

#include <cstddef>

// ChunkData is helper class for handling chunk data which is sent to players after its compressed.


namespace AM {

    class ChunkData {
        public:
            static constexpr bool LAST_WRITE = true;
            
            char* data { NULL };

            void allocate(size_t num_bytes) {
                this->data = new char[num_bytes];
                m_memsizeb = num_bytes;
            }
            
            void free_memory() {
                if(data) {
                    delete[] data;
                    data = NULL;
                    m_memsizeb = 0;
                }
            }

            void clear();
            void write_bytes(void* bytes, size_t sizeb);
            void set_write_offset(size_t offsetb) { m_data_sizeb += offsetb; }
            size_t get_memsize() { return m_memsizeb; }
            size_t size_inbytes() { return m_data_sizeb; }


        private:

            size_t m_data_sizeb { 0 };
            size_t m_memsizeb { 0 };

    };

};


#endif
