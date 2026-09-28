#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

class Shader {
private:
    std::unordered_map<std::string, GLint> uniformCache;
protected:
    virtual void compile();
    std::string name;
public:
    int status;
    GLuint ID = -1;
    Shader(){status = 1;}
    Shader(std::string name): name(name){
        status = 0;
        compile();
    }
    virtual ~Shader(){glDeleteProgram(ID);};

    void use() {
        glUseProgram(ID);
    }

    GLint getLocation(std::string_view name) {
        std::string stringname = std::string(name);
        auto it = uniformCache.find(stringname);
        if (it != uniformCache.end())
            return it->second;

        GLint loc = glGetUniformLocation(ID, stringname.c_str());
        uniformCache[stringname] = loc;
        return loc;
    }

    void setInt(std::string_view name, int x) {
        glUniform1i(getLocation(name), x);
    }
    void setUint(std::string_view name, unsigned int x) {
        glUniform1ui(getLocation(name), x);
    }
    void setFloat(std::string_view name, float x) {
        glUniform1f(getLocation(name), x);
    }
    void setVec2(std::string_view name, glm::vec2 v) {
        glUniform2fv(getLocation(name), 1, glm::value_ptr(v));
    }
    void setVec3(std::string_view name, glm::vec3 v) {
        glUniform3fv(getLocation(name), 1, glm::value_ptr(v));
    }
    void setVec4(std::string_view name, glm::vec4 v) {
        glUniform4fv(getLocation(name), 1, glm::value_ptr(v));
    }
    void setMat4(std::string_view name,  glm::mat4 m) {
        glUniformMatrix4fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setFloat(std::string_view name, std::vector<float> x) {
        glUniform1fv(getLocation(name), x.size(), x.data());
    }
    void setVec3(std::string_view name, std::vector<glm::vec3> v) {
        glUniform3fv(getLocation(name), v.size(), glm::value_ptr(v[0]));
    }
};

class ComputeShader : public Shader {
public:
    ComputeShader(std::string name) {
        Shader::name = name;
        status = 0;
        compile();
    }

    void compile() override;

    void dispatchWait(int numGroups) {
        glDispatchCompute(numGroups, 1, 1);    
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    void dispatchWaitIndirect(int offset) {
        glDispatchComputeIndirect(offset);    
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
    }
};