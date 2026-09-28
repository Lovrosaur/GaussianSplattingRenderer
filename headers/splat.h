#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <string>
#include <shader.h>
#include <transform.h>

struct PlySlpat
{
    float x,y,z;
    float n[3];
    float f_dc[3];
    float f_rest[45];
    float opacity;
    float scale[3];
    float rot[4];
};

struct Splat {
    glm::vec3 center;
    unsigned objectID;
    glm::vec3 scale;
    float opacity;
    glm::vec4 rot;
    glm::vec4 sh[16];
};

#define CHECK_LINE(expected) \
do { \
    if (text == NULL) {printf("PLY::failed to read line\n"); return 1;} \
    check=0; sscanf(text, expected "%n", &check); \
    if (!check) {printf("PLY::unexpected line read\n"); return 1;} \
} while(0);

#define SPLAT_LOAD_AT_ONCE 128

int loadPly(std::string filename, std::vector<Splat> &out, unsigned objID = 100);

void getSplatAABB(const Splat& splat, glm::vec3 &outMin, glm::vec3 &outMax);

static const float quad_v[] = {
    -1.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, -1.0f
};

static const int quad_f[] = {
    0, 1, 2,
    0, 2, 3
};

class Camera;

class SplatCollection {
private:
    struct DrawElementsIndirectCommand {
        GLuint count;
        GLuint instanceCount;   
        GLuint firstIndex;
        GLuint baseVertex;
        GLuint baseInstance;
    };

    struct DispatchIndirectCommand {
        GLuint num_groups_x;
        GLuint num_groups_y;
        GLuint num_groups_z;
    };
    
    void setModels();
    void setupSplats();
    void createBuffers();
public:
    std::vector<Splat> combinedSplats;
    std::vector<Transform*> transforms;
    std::vector<bool*> visible;
    size_t numModels = 0;
    unsigned int drawCount = 0;
    GLuint ssbo_gauss;
    GLuint ssbo_inds;
    GLuint ssbo_transforms;
    GLuint ssbo_keys, ssbo_counter, ssbo_tmp_inds, ssbo_tmp_keys, ssbo_offset, ssbo_group_offset, ssbo_partial_sums;
    GLuint VAO, VBO, EBO;
    ComputeShader *shaders[7];

    SplatCollection();

    ~SplatCollection();

    void draw(Shader &sh);
    void sort(Camera* camera);
    bool addSplat(std::string splatName, Transform *t = nullptr, bool* vis = nullptr);
    bool addSplat(std::vector<Splat>& splats, Transform *t = nullptr, bool* vis = nullptr);


};

#undef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES