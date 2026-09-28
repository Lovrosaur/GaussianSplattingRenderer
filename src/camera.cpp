#include <camera.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_float3.hpp>

void Camera::updateVectors() {
    forward.x = cos(glm::radians(YAW)) * cos(glm::radians(PITCH));
    forward.y = sin(glm::radians(PITCH));
    forward.z = sin(glm::radians(YAW)) * cos(glm::radians(PITCH));
    forward = glm::normalize(forward);
    right = glm::normalize(glm::cross(forward, worldUp));
    up = glm::normalize(glm::cross(right, forward));
}

Camera::Camera(glm::vec3 position, glm::vec3 forward, glm::vec3 up, float width, float height, float fov, float nearPlane, float farPlane, std::string name) : Object("",name),
    forward(forward), worldUp(up), width(width), height(height), fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
    POS = position;

    PITCH  = glm::degrees(std::asin(forward.y));
    YAW = glm::degrees(std::atan2(forward.z, forward.x));

    updateVectors();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(POS, POS+forward, up);
}
glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspectiveFov(glm::radians(fov), width, height, nearPlane, farPlane);
}

void Camera::bind(Shader &shader) {
    shader.setMat4("view", getViewMatrix());
    shader.setMat4("projection", getProjectionMatrix());
    shader.setVec2("viewport", glm::vec2(width, height));
    shader.setVec3("cam_pos", POS);
    float fx = width / (2 * tan(glm::radians(fov) / 2)), fy = height / (2 * tan(glm::radians(fov) / 2));
    (void)fy;
    shader.setVec2("focal", glm::vec2(fx,fx));
}

void Camera::move(glm::vec3 dir, bool yLock) { // x = desno, y = gore, z = naprjed
    if (yLock) {
        glm::vec3 XZForward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
        POS += dir.z * XZForward;
        POS += dir.x * right;
        POS += dir.y * worldUp;
    } else {
        POS += dir.x * right + dir.y * up + dir.z * forward;
    }
}

void Camera::rotate(glm::vec2 angle, bool orbit) {
    glm::vec3 target = POS + forward * distance;

    float mappingRot = glm::sign(worldUp.y);

    YAW += angle.x * mappingRot;
    PITCH += angle.y * mappingRot;
    
    if (YAW >= 360.f) YAW -= 360;
    if (YAW < 0.f) YAW += 360;
    if (PITCH > 89.0f)  PITCH = 89.0f;
    if (PITCH < -89.0f) PITCH = -89.0f;

    updateVectors();

    if (orbit) {
        POS = target - (forward * distance);
    }
}


CameraFxFy::CameraFxFy(glm::vec3 position, glm::vec3 forward, glm::vec3 up, float width, float height, float fx, float fy, float nearPlane , float farPlane): 
    Camera(position, forward, up, width, height, 0.f, nearPlane, farPlane), fx(fx), fy(fy) {}

glm::mat4 CameraFxFy::getProjectionMatrix() {
    float left   = -nearPlane * width  / (2.0f * fx);
    float right  =  nearPlane * width  / (2.0f * fx);
    float bottom = -nearPlane * height / (2.0f * fy);
    float top    =  nearPlane * height / (2.0f * fy);
    
    // float fovY = 2.0f * atan(height / (2.0f * fy));
    // return glm::perspectiveFov(fovY, width, height, nearPlane, farPlane);
    return glm::frustum(left, right, bottom, top, nearPlane, farPlane);
}

void CameraFxFy::bind(Shader &shader) {
    Camera::bind(shader);
    shader.setVec2("focal", glm::vec2(fx,fy));
}

