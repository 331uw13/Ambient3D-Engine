#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "chunk.hpp"
#include "raymath.h"
#include "rlgl.h"


            
float AM::Chunk::m_get_height_at_local(int x, int z) {
    int64_t idx = z * (m_chunk_size+1) + x;
    if(idx < 0) {
        return 0.0f;
    }
    if(idx >= (m_chunk_size+1)*(m_chunk_size+1)) {
        return 0.0f;
    }
    return this->height_points[idx];
}

void AM::Chunk::load(float* points, size_t points_sizeb, int chunk_size, float scale, Material* mat) {
    m_mesh = Mesh{
        .vertexCount = 0,
        .triangleCount = 0,
        .vertices = NULL,
        .texcoords = NULL,
        .texcoords2 = NULL,
        .normals = NULL,
        .tangents = NULL,
        .colors = NULL,
        .indices = NULL,
        .animVertices = NULL,
        .animNormals = NULL,
        .boneIds = NULL,
        .boneWeights = NULL,
        .boneMatrices = NULL,
        .boneCount = 0,
        .vaoId = 0,
        .vboId = NULL
    };

    this->height_points = new float[points_sizeb / sizeof(float)];
    m_chunk_size = chunk_size;
    m_material = mat;
    m_scale = scale;

    memmove(this->height_points, points, points_sizeb);

    m_mesh.triangleCount = (chunk_size * chunk_size) * 2;  // 2 Triangles per quad.
    m_mesh.vertexCount = m_mesh.triangleCount * 3;         // 3 Vertices per triangle.

    m_mesh.vertices    = new float[m_mesh.vertexCount * 3];
    m_mesh.normals     = new float[m_mesh.vertexCount * 3];
  //m_mesh.texcoords   = new float[m_mesh.vertexCount * 2];  <-- TODO

    // Used for calculating normals.
    Vector3 vA{ 0, 0, 0 };
    Vector3 vB{ 0, 0, 0 };
    Vector3 vC{ 0, 0, 0 };

    int v_counter = 0; // Count vertices.
    int tc_counter = 0; // Count texture coords.

    for(int z = 0; z < chunk_size; z++) {
        for(int x = 0; x < chunk_size; x++) {
            const float xf = (float)x;
            const float zf = (float)z;


            // Left up corner triangle.

            m_mesh.vertices[v_counter+0] = xf * scale;
            m_mesh.vertices[v_counter+1] = m_get_height_at_local(x, z);
            m_mesh.vertices[v_counter+2] = zf * scale;

            m_mesh.vertices[v_counter+3] = xf * scale;
            m_mesh.vertices[v_counter+4] = m_get_height_at_local(x, z+1);
            m_mesh.vertices[v_counter+5] = (zf+1.0f) * scale;

            m_mesh.vertices[v_counter+6] = (xf+1.0f) * scale;
            m_mesh.vertices[v_counter+7] = m_get_height_at_local(x+1, z);
            m_mesh.vertices[v_counter+8] = zf * scale;

            // Right bottom corner triangle.

            m_mesh.vertices[v_counter+9]  = m_mesh.vertices[v_counter+6];
            m_mesh.vertices[v_counter+10] = m_mesh.vertices[v_counter+7];
            m_mesh.vertices[v_counter+11] = m_mesh.vertices[v_counter+8];
            
            m_mesh.vertices[v_counter+12] = m_mesh.vertices[v_counter+3];
            m_mesh.vertices[v_counter+13] = m_mesh.vertices[v_counter+4];
            m_mesh.vertices[v_counter+14] = m_mesh.vertices[v_counter+5];
            
            m_mesh.vertices[v_counter+15] = (xf+1.0f) * scale;
            m_mesh.vertices[v_counter+16] = m_get_height_at_local(x+1, z+1);
            m_mesh.vertices[v_counter+17] = (zf+1.0f) * scale;

            v_counter += 18;
        }
    }

    // Calculate normals.
    for(int idx = 0; idx < m_mesh.vertexCount*3; idx += 9) {
        vA = Vector3(m_mesh.vertices[idx + 0],
                     m_mesh.vertices[idx + 1],
                     m_mesh.vertices[idx + 2]);
        
        vB = Vector3(m_mesh.vertices[idx + 3],
                     m_mesh.vertices[idx + 4],
                     m_mesh.vertices[idx + 5]);
        
        vC = Vector3(m_mesh.vertices[idx + 6],
                     m_mesh.vertices[idx + 7],
                     m_mesh.vertices[idx + 8]);

        Vector3 vN = Vector3CrossProduct(Vector3Subtract(vB, vA), Vector3Subtract(vC, vA));
        vN = Vector3Normalize(vN);

        for(int k = 0; k < 9; k += 3) {
            m_mesh.normals[idx + k + 0] = vN.x;
            m_mesh.normals[idx + k + 1] = vN.y;
            m_mesh.normals[idx + k + 2] = vN.z;
        }
    }

    UploadMesh(&m_mesh, false);
    m_loaded = true;
}

void AM::Chunk::unload() {
    if(!m_loaded) {
        fprintf(stderr, "ERROR! Trying to unload already unloaded chunk.\n");
        return;
    }

    rlUnloadVertexArray(m_mesh.vaoId);
    if(m_mesh.vboId) {
        for(int i = 0; i < 9/*MAX_MESH_VERTEX_BUFFERS*/; i++) {
            rlUnloadVertexBuffer(m_mesh.vboId[i]);
        }
    }


    if(m_mesh.vertices) { delete[] m_mesh.vertices; }
    if(m_mesh.normals)  { delete[] m_mesh.normals; }
    if(this->height_points) { delete[] this->height_points; }

    m_mesh.vertices = NULL;
    m_mesh.normals = NULL;
    this->height_points = NULL;

    m_loaded = false;
}

void AM::Chunk::render() {
    if(!m_loaded) {
        return;
    }

    Matrix translation = MatrixTranslate(
            this->pos.x * (m_chunk_size * m_scale),
            0,
            this->pos.z * (m_chunk_size * m_scale));
    
    DrawMesh(m_mesh, *m_material, translation);
}


