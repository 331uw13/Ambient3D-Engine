#include <cstring>
#include "../include/byte_array.hpp"




void AM::ByteArray::from_hexstring(const std::string& hexstr) {
    m_bytes.clear();

    constexpr size_t HEXBUF_SIZE = 8;
    char hexbuf[HEXBUF_SIZE] = { 0 };
    size_t hexbuf_i = 0;

    for(size_t i = 0; i < hexstr.size(); i++) {
        char c = hexstr[i];
        if(c == '-') {
            m_bytes.push_back(strtol(hexbuf, NULL, 16/*base*/));
            memset(hexbuf, 0, HEXBUF_SIZE);
            continue;
        }
        hexbuf[hexbuf_i++] = c;
        if(hexbuf_i >= HEXBUF_SIZE) {
            break;
        }
    }
}

void AM::ByteArray::to_hexstring(std::string* out) {
    out->clear();
    out->reserve(m_bytes.size());
    
    const char* HEX = "0123456789abcdef";
    for(size_t i = 0; i < m_bytes.size(); i++) {
        out->push_back(HEX[ (abs(m_bytes[i]) >> 4) % 0xf ]);
        out->push_back(HEX[ (abs(m_bytes[i])) & 0xf ]);
        if(i+1 < m_bytes.size()) {
            out->push_back('-');
        }
    }
}

void AM::ByteArray::copy_bytes(char* bytes, size_t sizeb) {
    if(m_bytes.capacity() < sizeb) {
        m_bytes.resize(m_bytes.capacity() + sizeb);
    }
    memmove(m_bytes.data(), bytes, sizeb);
}


