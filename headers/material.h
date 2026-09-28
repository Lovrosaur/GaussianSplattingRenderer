#pragma once

#include <string>
#include <glm/glm.hpp>

struct Material {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float n;
};

const Material defaultMaterial = Material{glm::vec3(1.f,0.f,1.f), glm::vec3(0.5),694};

int loadMaterail(std::string const &path, Material &mat);