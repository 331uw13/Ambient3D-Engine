#ifndef AMBIENT3D_NETWORK_DYNAMIC_DATA_HPP
#define AMBIENT3D_NETWORK_DYNAMIC_DATA_HPP

#include <mutex>
#include <cstdint>

#include "raylib.h"


// Server sends general information to players which changes overtime.
// This must be done in a way that respects thread safety
// because the networking runs in a separate thread.


namespace AM {

    // Stands for "Network Dynamic Data ID"
    namespace NDD_ID {
        enum : uint32_t {
            FOG_DENSITY=0,     // float
            FOG_COLOR,         // vector3
            TIMEOFDAY_SYNC,    // float
            //...

            NUM_IDS,
        };
    }

    struct NDD_ID_var {
        enum Type {
            FLOAT_T,
            INT_T,
            VECTOR3_T
        };
        union {
            float   float_v;
            int     int_v;
            Vector3 vector3_v;
        };

        Type type;
    };

    class NetworkDynamicData {
        public:

            void    set_float(uint32_t ndd_id, float value);            // < thread safe >
            void    set_int(uint32_t ndd_id, int value);                // < thread safe >
            void    set_vector3(uint32_t ndd_id, const Vector3& value); // < thread safe >

            float   get_float(uint32_t ndd_id);      // < thread safe >
            int     get_int(uint32_t ndd_id);        // < thread safe >
            Vector3 get_vector3(uint32_t ndd_id);    // < thread safe >

        private:

            bool             m_is_valid_id(uint32_t ndd_id);
            AM::NDD_ID_var*  m_get_NDD_ID_var_pointer(uint32_t ndd_id, AM::NDD_ID_var::Type var_type, const char* type_str);
            AM::NDD_ID_var   m_variables[AM::NDD_ID::NUM_IDS];
            std::mutex m_mutex;


    };

};


#endif
