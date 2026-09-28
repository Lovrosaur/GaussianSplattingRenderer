#pragma once
#include <shader.h>
#include <object.h>
#include <string>
#include <glm/glm.hpp>

#define POS transform.position
#define YAW transform.rotation.y
#define PITCH transform.rotation.z

class Camera : public Object {
    friend void saveCamera(FILE* f, Camera* cam);
    friend Camera* loadCamera(FILE* f);
protected:
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;

    glm::vec3 worldUp;

    float width, height;
private:
    const float distance = 5.f;
    float fov;

    void updateVectors();
public:
    float nearPlane;
    float farPlane;
    
    Camera(glm::vec3 position, glm::vec3 forward, glm::vec3 up, float width, float height, float fov = 90, float nearPlane = 0.1, float farPlane = 1000, std::string name = "Camera");
    glm::mat4 getViewMatrix();
    virtual glm::mat4 getProjectionMatrix();
    virtual void bind(Shader &shader);
    void move(glm::vec3 dir, bool yLock = false);
    void rotate(glm::vec2 angle, bool orbit);
protected:
    Camera(Transform t, glm::vec3 worldUp, float width, float height, float fov, float nearPlane, float farPlane, std::string name = "Camera") 
        : Object("", name), worldUp(worldUp), width(width), height(height), fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
        this->transform = t;
        updateVectors();
    }
};


class CameraFxFy : public Camera {
    friend void saveCamera(FILE* f, Camera* cam);
    friend Camera* loadCamera(FILE* f);
private:
float fx,fy;
public:
    CameraFxFy(glm::vec3 position, glm::vec3 forward, glm::vec3 up, float width, float height, float fx, float fy, float nearPlane = 0.1, float farPlane = 1000);
    glm::mat4 getProjectionMatrix() override;
    void bind(Shader &shader) override;
protected:
    CameraFxFy(Transform t, glm::vec3 worldUp, float width, float height, float fx, float fy, float nearPlane, float farPlane)
        : Camera(t, worldUp, width, height, 0.f, nearPlane, farPlane), fx(fx), fy(fy) {}
};
