#include <fstream>
#include <openssl/sha.h>

#include "../include/file_sha256.hpp"
#include "../include/byte_array.hpp"


void AM::compute_sha256_filehash(const std::string filepath, std::string* out) {

    std::fstream file_stream(filepath);
    file_stream.seekg(0, file_stream.end);
    size_t file_size = file_stream.tellg();
    file_stream.seekg(0, file_stream.beg);
    
    if(file_size == 0) {
        fprintf(stderr, "ERROR! %s: \"%s\" File size is zero.\n", __func__, filepath.c_str());
    }

    char* buffer = new char[file_size];

    file_stream.read(buffer, file_size);

    AM::ByteArray hash(SHA256_DIGEST_LENGTH);
    SHA256((unsigned char*)buffer, file_size, (unsigned char*)hash.data());

    hash.to_hexstring(out);
    delete[] buffer;
}
