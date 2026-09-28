#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <transform.h>
#include <shader.h>
#include <mesh.h>
#include <vector>

// --- Singleton for Rendering ---
class OBBWireframe {
private:
    const float vertices[24] = {
        1,1,1, 1,1,-1, 1,-1,-1, 1,-1,1,
        -1,1,1, -1,1,-1, -1,-1,-1, -1,-1,1  
    };
          
    const unsigned int indices[36] = {
        0, 1, 2, 0, 2, 3,
        0, 3, 7, 0, 7, 4,
        0, 4, 5, 0, 5, 1,
        6, 2, 1, 6, 1, 5,
        6, 5, 4, 6, 4, 7,
        6, 7, 3, 6, 3, 2
    };
    GLuint VAO, VBO, EBO; 
    
    OBBWireframe();
public:
    OBBWireframe(OBBWireframe &other) = delete;
    void operator=(const OBBWireframe &) = delete;
    static OBBWireframe *Get();
    void draw();
};

class Collider {
public:
    Transform* parentTransform;
    Transform localTransform;
    bool active = true;

    Collider() : parentTransform(nullptr), localTransform() {}
    Collider(Transform* parent) : parentTransform(parent), localTransform() {}

    glm::mat4 getWorldMatrix();
    glm::vec3 getSupport(glm::vec3 direction);
    bool check(glm::vec3 point);

    bool check(Collider& other);
    void draw(Shader& sh);

private:
    glm::vec3 minkowskiSum(Collider& other, glm::vec3 dir);

    bool doSimplex(std::vector<glm::vec3>& s, glm::vec3& d);
};
