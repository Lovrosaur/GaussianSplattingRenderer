#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


class Transform {
private:
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    Transform(glm::vec3 position = {0.f,0.f,0.f}, glm::vec3 rotation = {0.f,0.f,0.f}, glm::vec3 scale = {1.f,1.f,1.f}): position(position), rotation(rotation), scale(scale) {}
    glm::mat4 getModelMatrix() const {
        glm::quat qRotation = glm::quat(glm::radians(rotation)); 
        glm::mat4 rotationMatrix = glm::mat4_cast(qRotation);
        return glm::translate(glm::mat4(1.0f), position) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);
    }
};

