#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <stb_image.h>

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

GLuint TextureFromFile(const char *path, const std::string &directory/*, bool gamma = false*/);