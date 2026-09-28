#pragma once
#include <glm/glm.hpp>
#include <shader.h>

struct Light {
    glm::vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class SceneLight{
private:
    std::vector<glm::vec3> position;
    
    std::vector<float> constant;
    std::vector<float> linear;
    std::vector<float> quadratic;  

    std::vector<glm::vec3> ambient;
    std::vector<glm::vec3> diffuse;
    std::vector<glm::vec3> specular;
public:
    void bind(Shader &shader);

    void addLight(Light light);
};

