#include <splat.h>
#include <stdio.h>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <camera.h>

int loadPly(std::string filename, std::vector<Splat> &out, unsigned objID) {
    FILE *file = fopen(filename.c_str(),"rb");
    if (file == NULL) {
        printf("PLY::failed to open file\n");
        return 1;
    }
    char buf[256];
    int check;
    char* text;
    text = fgets(buf,256,file);
    CHECK_LINE("ply");
    text = fgets(buf,256,file);
    CHECK_LINE("format binary_little_endian");
    text = fgets(buf,256,file);
    CHECK_LINE("element vertex");
    int N = -1;
    sscanf(text,"%*[^0-9]%d",&N);
    while ((text = fgets(buf,256,file))) {
        check = 0;
        sscanf(text, "end_header%n", &check);
        if (check) {
            break;
        }
        CHECK_LINE("property float");
    }
    
    PlySlpat tmp_splat[SPLAT_LOAD_AT_ONCE];
    Splat splat;
    for (;N > 0; N -= SPLAT_LOAD_AT_ONCE) {
        size_t numRead = fread(tmp_splat, sizeof(PlySlpat), SPLAT_LOAD_AT_ONCE, file);
        for (size_t i = 0; i < numRead; i++) {
            splat.center =  glm::vec4(tmp_splat[i].x, tmp_splat[i].y, tmp_splat[i].z,1);
            splat.scale =   glm::exp(glm::vec3(tmp_splat[i].scale[0], tmp_splat[i].scale[1], tmp_splat[i].scale[2]));
            splat.opacity = 1.f / (1.f + std::exp(-tmp_splat[i].opacity));
            splat.rot =     glm::normalize(glm::vec4(tmp_splat[i].rot[0], tmp_splat[i].rot[1], tmp_splat[i].rot[2], tmp_splat[i].rot[3]));
            splat.sh[0] =   glm::vec4(tmp_splat[i].f_dc[0], tmp_splat[i].f_dc[1], tmp_splat[i].f_dc[2],1);
            for (size_t j = 0; j < 15; j++){ 
                splat.sh[j+1] = glm::vec4(tmp_splat[i].f_rest[j], tmp_splat[i].f_rest[15+j], tmp_splat[i].f_rest[30+j],1);
            }
            splat.objectID = objID;
            out.push_back(splat);

        }
    }
    fclose(file);
    return 0;
}

void getSplatAABB(const Splat& splat, glm::vec3 &outMin, glm::vec3 &outMax) {
    static const glm::vec3 corner[8] = {
        glm::vec3(0.5f,0.5f,0.5f),
        glm::vec3(0.5f,0.5f,-0.5f),
        glm::vec3(0.5f,-0.5f,-0.5f),
        glm::vec3(0.5f,-0.5f,0.5f),
        glm::vec3(-0.5f,0.5f,0.5f),
        glm::vec3(-0.5f,0.5f,-0.5f),
        glm::vec3(-0.5f,-0.5f,-0.5f),
        glm::vec3(-0.5f,-0.5f,0.5f) 
    };
    static const float sigma = 1;
    glm::mat4 base = glm::translate(glm::mat4(1.f), splat.center) 
                    * glm::mat4_cast(glm::make_quat(glm::value_ptr(splat.rot))) 
                    * glm::scale(glm::mat4(1.f), splat.scale);
    glm::vec4 point = base * glm::vec4(sigma*corner[0],1);
    outMin = outMax = glm::vec3(point/point.w);
    for (int i = 1; i < 8; i++) {
        point = base * glm::vec4(sigma*corner[i],1);
        outMin = glm::min(outMin, glm::vec3(point/point.w));
        outMax = glm::max(outMax, glm::vec3(point/point.w));
    }
}

SplatCollection::SplatCollection() {
    shaders[0] = new ComputeShader("radixKeys");
    shaders[1] = new ComputeShader("radixHist");
    shaders[2] = new ComputeShader("radixPre1");
    shaders[5] = new ComputeShader("radixPre2");
    shaders[6] = new ComputeShader("radixPre3");
    shaders[3] = new ComputeShader("radixScat");
    shaders[4] = new ComputeShader("radixIndr");
    createBuffers();
}

void SplatCollection::createBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glGenBuffers(1, &ssbo_gauss);
    glGenBuffers(1, &ssbo_transforms);
    glGenBuffers(1, &ssbo_inds);
    glGenBuffers(1, &ssbo_keys);
    glGenBuffers(1, &ssbo_counter);
    glGenBuffers(1, &ssbo_tmp_inds);
    glGenBuffers(1, &ssbo_tmp_keys);
    glGenBuffers(1, &ssbo_offset);
    glGenBuffers(1, &ssbo_group_offset);
    glGenBuffers(1, &ssbo_partial_sums);
}

SplatCollection::~SplatCollection() {
    delete shaders[0];
    delete shaders[1];
    delete shaders[2];
    delete shaders[5];
    delete shaders[6];
    delete shaders[3];
    delete shaders[4];
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &ssbo_gauss);
    glDeleteBuffers(1, &ssbo_inds);
    glDeleteBuffers(1, &ssbo_transforms);
    glDeleteBuffers(1, &ssbo_keys);
    glDeleteBuffers(1, &ssbo_counter);
    glDeleteBuffers(1, &ssbo_tmp_inds);
    glDeleteBuffers(1, &ssbo_tmp_keys);
    glDeleteBuffers(1, &ssbo_offset);
    glDeleteBuffers(1, &ssbo_group_offset);
    glDeleteBuffers(1, &ssbo_partial_sums);
}

void SplatCollection::draw(Shader &sh) {
    (void)sh;
    // setModels();
    glBindVertexArray(VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ssbo_counter);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*) sizeof(unsigned int));
    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void SplatCollection::sort(Camera* camera) {
    const GLuint zero = 0;
    const int offset = sizeof(unsigned) + sizeof(DrawElementsIndirectCommand);
    const int offset2 = sizeof(unsigned) + sizeof(DrawElementsIndirectCommand) + sizeof(DispatchIndirectCommand);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counter);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &zero);
    shaders[0]->use();
    camera->bind(*shaders[0]);
    shaders[0]->setUint("num_splats", combinedSplats.size());
    setModels();
    shaders[0]->dispatchWait((combinedSplats.size()+255)/256);
    
    shaders[4]->use();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, ssbo_counter);
    
    for(int pass=0; pass<8; pass++) {

        shaders[1]->use();
        shaders[1]->setUint("pass",pass);
        shaders[1]->dispatchWaitIndirect(offset);
        shaders[2]->use();
        shaders[2]->dispatchWaitIndirect(offset2);
        shaders[5]->use();
        shaders[5]->dispatchWait(1);
        shaders[6]->use();
        shaders[6]->dispatchWaitIndirect(offset2);
        shaders[3]->use();
        shaders[3]->setUint("pass",pass);
        shaders[3]->dispatchWaitIndirect(offset);
        GLint tmp;
        tmp = ssbo_inds;
        ssbo_inds = ssbo_tmp_inds;
        ssbo_tmp_inds = tmp;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_inds);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_tmp_inds);

        tmp = ssbo_keys;
        ssbo_keys = ssbo_tmp_keys;
        ssbo_tmp_keys = tmp;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_keys);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_tmp_keys);

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }

}

bool SplatCollection::addSplat(std::string splatName, Transform *t, bool *vis) {
    if (!!loadPly(splatName, combinedSplats, numModels)) return false;
    transforms.push_back(t);
    visible.push_back(vis);
    numModels++;
    setupSplats();
    return true;
}

bool SplatCollection::addSplat(std::vector<Splat>& splats, Transform *t, bool *vis) {
    for (Splat &splat : splats) {
        splat.objectID = numModels;
    }
    combinedSplats.insert(combinedSplats.end(), splats.begin(), splats.end());
    transforms.push_back(t);
    visible.push_back(vis);
    numModels++;
    setupSplats();
    return true;
} 

void SplatCollection::setModels() {
    std::vector<glm::mat4> models;
    for (size_t i = 0; i < numModels; i++) {
        if (!visible[i] || *visible[i]) 
            models.push_back(transforms[i] ? transforms[i]->getModelMatrix() : glm::mat4(1.));
        else{
            models.push_back(glm::mat4(0.));
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transforms);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numModels*sizeof(glm::mat4), models.data());
}

void SplatCollection::setupSplats(){
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    drawCount = combinedSplats.size();

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_v), quad_v, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_f), quad_f, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gauss);
    glBufferData(GL_SHADER_STORAGE_BUFFER, combinedSplats.size() * sizeof(Splat), combinedSplats.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_gauss);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_inds);
    glBufferData(GL_SHADER_STORAGE_BUFFER, combinedSplats.size() * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_inds);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transforms);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numModels* sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_transforms);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counter);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) + sizeof(DrawElementsIndirectCommand) + 2 * sizeof(DispatchIndirectCommand), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_counter);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_keys);
    glBufferData(GL_SHADER_STORAGE_BUFFER, combinedSplats.size() * 2 * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_keys);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_tmp_inds);
    glBufferData(GL_SHADER_STORAGE_BUFFER, combinedSplats.size() * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_tmp_inds);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_tmp_keys);
    glBufferData(GL_SHADER_STORAGE_BUFFER, combinedSplats.size() * 2 * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_tmp_keys);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_group_offset);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (combinedSplats.size()+255)/256 * 256 * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_group_offset);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_offset);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_partial_sums);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ((combinedSplats.size()+255)/256 + 15) / 16 * 256 * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_partial_sums);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

