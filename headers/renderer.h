#pragma once
#include <shader.h>
#include <object.h>
#include <splat.h>
#include <camera.h>
#include <light.h>

class Renderer{
public:
    Shader *meshSh, *splatSh, *wireSh;
    std::vector<MeshObject*> solidObjects;
    SplatCollection *splatCollection;
    int screenWidth, screenHeight;
    bool dispayColliders;
    Renderer();
    
    ~Renderer();
    void setScreen(int width, int height);

    void init();

    void render(Camera *camera, std::vector<Object *> &selectable, Object *selected, SceneLight *lights, int effect = 0);
};