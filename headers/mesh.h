#pragma once
/*
Addapted form https://learnopengl.com/Model-Loading
*/
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <shader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glad/glad.h> 

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <texture.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

#define DIFFUSE_MAP  (1 << 0)
#define SPECULAR_MAP (1 << 1)
#define NORMAL_MAP   (1 << 2)
#define HEIGHT_MAP   (1 << 3)

class Mesh {
private:
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int texFlag;
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, unsigned int flag = 0);

    void Draw(Shader &shader);

    void getAABB(glm::vec3 &outMin, glm::vec3 &outMax);
private:
    void setupMesh();
};

class Model {
public:
    std::vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    Model(std::string const &path, bool gamma = false);
    void Draw(Shader &shader);
    
private:
    void loadModel(std::string const &path);
    // recursive
    void processNode(aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};
