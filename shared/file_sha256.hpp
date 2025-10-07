#ifndef AMBIENT3D_FILE_SHA256_HPP
#define AMBIENT3D_FILE_SHA256_HPP

#include <string>


namespace AM {

    void compute_sha256_filehash(const std::string filepath, std::string* out);

};




#endif
