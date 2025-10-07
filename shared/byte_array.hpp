#ifndef AMBIENT3D_BYTE_ARRAY_HPP
#define AMBIENT3D_BYTE_ARRAY_HPP


#include <string>
#include <vector>

namespace AM {

    class ByteArray {
        public:
            
            ByteArray(){}
            ByteArray(size_t size) { m_bytes.resize(size); }

            void from_hexstring(const std::string& hexstr);
            void to_hexstring(std::string* out);
            void copy_bytes(char* bytes, size_t sizeb);

            void   resize(size_t n) { m_bytes.resize(n); }
            size_t capacity() { return m_bytes.capacity(); }
            size_t size()  { return m_bytes.size(); }
            char*  data()  { return m_bytes.data(); }
            void   clear() { m_bytes.clear(); }

        private:
            std::vector<char> m_bytes;
    };

};


#endif
