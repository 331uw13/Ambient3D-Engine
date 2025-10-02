#include <cstdio>

#include "net_dynamic_data.hpp"


bool AM::NetworkDynamicData::m_is_valid_id(uint32_t ndd_id) {
    if(ndd_id >= AM::NDD_ID::NUM_IDS) { 
        fprintf(stderr, "ERROR! %s() at %s: Invalid ID(%i)\n",
                __func__, __FILE__, ndd_id);
        return false;
    }
    return true;
}

AM::NDD_ID_var* AM::NetworkDynamicData::
 m_get_NDD_ID_var_pointer(uint32_t ndd_id, AM::NDD_ID_var::Type var_type, const char* type_str) {
     if(!m_is_valid_id(ndd_id)) { 
        return NULL;
    }

    AM::NDD_ID_var* ptr = &m_variables[ndd_id];
    if(ptr->type != var_type) {
        fprintf(stderr, "ERROR! %s() at %s: NetworkDynamicDataID(%i) type is not compatible with type %s\n",
                __func__, __FILE__, ndd_id, type_str);
        return NULL;
    }

    return ptr;
}


void AM::NetworkDynamicData::set_float(uint32_t ndd_id, float value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(!m_is_valid_id(ndd_id)) {
        return;
    }
    AM::NDD_ID_var& var = m_variables[ndd_id];
    var.type = AM::NDD_ID_var::FLOAT_T;
    var.float_v = value;
}

void AM::NetworkDynamicData::set_int(uint32_t ndd_id, int value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(!m_is_valid_id(ndd_id)) {
        return;
    }
    AM::NDD_ID_var& var = m_variables[ndd_id];
    var.type = AM::NDD_ID_var::INT_T;
    var.int_v = value;
}

void AM::NetworkDynamicData::set_vector3(uint32_t ndd_id, const Vector3& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(!m_is_valid_id(ndd_id)) {
        return;
    }
    AM::NDD_ID_var& var = m_variables[ndd_id];
    var.type = AM::NDD_ID_var::VECTOR3_T;
    var.vector3_v = value;
}

float AM::NetworkDynamicData::get_float(uint32_t ndd_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AM::NDD_ID_var* ptr = m_get_NDD_ID_var_pointer(ndd_id, AM::NDD_ID_var::FLOAT_T, "float");
    return (ptr ? ptr->float_v : 0.0f);
}

int AM::NetworkDynamicData::get_int(uint32_t ndd_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AM::NDD_ID_var* ptr = m_get_NDD_ID_var_pointer(ndd_id, AM::NDD_ID_var::INT_T, "int");
    return (ptr ? ptr->int_v : 0.0f);
}

Vector3 AM::NetworkDynamicData::get_vector3(uint32_t ndd_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AM::NDD_ID_var* ptr = m_get_NDD_ID_var_pointer(ndd_id, AM::NDD_ID_var::VECTOR3_T, "vector3");
    return (ptr ? ptr->vector3_v : Vector3(0, 0, 0));
}

