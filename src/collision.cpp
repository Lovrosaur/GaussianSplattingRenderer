#include <collision.h>

    
OBBWireframe::OBBWireframe() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

OBBWireframe *OBBWireframe::Get() {
    static OBBWireframe instance;
    return &instance;
}
void OBBWireframe::draw() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

glm::mat4 Collider::getWorldMatrix() {
    if (!parentTransform) return localTransform.getModelMatrix();
    return parentTransform->getModelMatrix() * localTransform.getModelMatrix();
}

glm::vec3 Collider::getSupport(glm::vec3 direction) {
    glm::mat4 worldMatrix = getWorldMatrix();

    glm::vec3 localDir = glm::vec3(glm::inverse(worldMatrix) * glm::vec4(direction, 0.0f));

    glm::vec3 result;
    result.x = (localDir.x > 0) ? 1.0f : -1.0f;
    result.y = (localDir.y > 0) ? 1.0f : -1.0f;
    result.z = (localDir.z > 0) ? 1.0f : -1.0f;

    return glm::vec3(worldMatrix * glm::vec4(result, 1.0f));
}

bool Collider::check(glm::vec3 point) {
    if (!active) return false;
    
    glm::vec3 localPoint = glm::vec3(glm::inverse(getWorldMatrix()) * glm::vec4(point, 1.0f));
    return (std::abs(localPoint.x) <= 1.0f && 
            std::abs(localPoint.y) <= 1.0f && 
            std::abs(localPoint.z) <= 1.0f);
}

bool Collider::check(Collider& other) {
    if (!active || !other.active) return false;

    glm::vec3 d = glm::vec3(1, 0, 0); 
    std::vector<glm::vec3> simplex;
    simplex.push_back(minkowskiSum(other, d));
    
    d = -simplex[0];

    for (int i = 0; i < 32; i++) {
        glm::vec3 A = minkowskiSum(other, d);

        if (glm::dot(A, d) < 0) return false; 

        simplex.insert(simplex.begin(), A);
        
        if (doSimplex(simplex, d)) return true;
    }
    return false;
}

void Collider::draw(Shader& sh) {
    if (!active) return;
    sh.setMat4("model", getWorldMatrix());
    OBBWireframe::Get()->draw();
}

glm::vec3 Collider::minkowskiSum(Collider& other, glm::vec3 dir) {
    return this->getSupport(dir) - other.getSupport(-dir);
}

bool line(std::vector<glm::vec3>& s, glm::vec3& d) {
    glm::vec3 a = s[0]; 
    glm::vec3 b = s[1];
    glm::vec3 ab = b - a;
    glm::vec3 ao = -a;

    if (glm::dot(ab, ao) > 0) {
        d = glm::cross(glm::cross(ab, ao), ab);
    } else {
        s.pop_back();
        d = ao;
    }
    return false;
}


bool triangle(std::vector<glm::vec3>& s, glm::vec3& d) {
    glm::vec3 a = s[0], b = s[1], c = s[2];
    glm::vec3 ab = b - a, ac = c - a, ao = -a;
    glm::vec3 abc = glm::cross(ab, ac);

    if (glm::dot(glm::cross(abc, ac), ao) > 0) {
        if (glm::dot(ac, ao) > 0) {
            s.erase(s.begin() + 1);
            d = glm::cross(glm::cross(ac, ao), ac);
        } else {
            s.erase(s.begin() + 2);
            return line(s, d);
        }
    } else {
        if (glm::dot(glm::cross(ab, abc), ao) > 0) {
            s.erase(s.begin() + 2);
            return line(s, d);
        } else {
            if (glm::dot(abc, ao) > 0) {
                d = abc;
            } else {
                std::swap(s[1], s[2]);
                d = -abc;
            }
        }
    }
    return false;
}

bool tetrahedron(std::vector<glm::vec3>& s, glm::vec3& d) {
    glm::vec3 a = s[0], b = s[1], c = s[2], d_pt = s[3];
    glm::vec3 ab = b - a, ac = c - a, ad = d_pt - a, ao = -a;
    glm::vec3 abc = glm::cross(ab, ac), acd = glm::cross(ac, ad), adb = glm::cross(ad, ab);

    if (glm::dot(abc, ao) > 0) {
        s.erase(s.begin() + 3);
        return triangle(s, d);
    }
    if (glm::dot(acd, ao) > 0) {
        s.erase(s.begin() + 1); 
        return triangle(s, d);
    }
    if (glm::dot(adb, ao) > 0) {
        s.erase(s.begin() + 2); 
        return triangle(s, d);
    }
    return true; 
}

bool Collider::doSimplex(std::vector<glm::vec3>& s, glm::vec3& d) {
    switch (s.size()) {
        case 2: return line(s, d);
        case 3: return triangle(s, d);
        case 4: return tetrahedron(s, d);
    }
    return false;
}

