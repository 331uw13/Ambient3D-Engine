#ifndef AMBIENT3D_CHUNK_POS_HPP
#define AMBIENT3D_CHUNK_POS_HPP

#include <functional>
#include <cstdio>

namespace AM {

    struct ChunkPos {
        int x;
        int z;

        ChunkPos(){}
        ChunkPos(int _x, int _z) : x(_x), z(_z) {}

        //float distance(const AM::ChunkPos& rhs);

        bool operator==(const ChunkPos& rhs) const {
            return (this->x == rhs.x && this->z == rhs.z);
        }
    };

};

namespace std {
    template <>
    struct hash<AM::ChunkPos> {
        std::size_t operator()(const AM::ChunkPos& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 1);
        }
    };
}




#endif
