#ifndef AMBIENT3D_TRANSPARENT_STRING_HASH_HPP
#define AMBIENT3D_TRANSPARENT_STRING_HASH_HPP

#include <cstddef>
#include <string_view>
    
// Used for enabling heterogeneous lookup for std::unordered_map


namespace AM {

    struct TransparentStringHash {
        using is_transparent = void;

        // nodiscard for enabling warnings if the return value is not used.

        [[nodiscard]] size_t operator()(const char* str) const {
            return std::hash<std::string_view>{}(str);
        }

        [[nodiscard]] size_t operator()(const std::string_view& str) const {
            return std::hash<std::string_view>{}(str);
        }
    };

};



#endif
