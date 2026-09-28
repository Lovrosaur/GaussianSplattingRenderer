#include <renderer.h>

Renderer::Renderer() {
    meshSh = new Shader("raster");
    splatSh = new Shader("splat");
    wireSh = new Shader("wire");
    splatCollection = new SplatCollection();
    init();
}

Renderer::~Renderer(){
    delete splatCollection;
    delete meshSh;
    delete splatSh;
}
void Renderer::setScreen(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    splatSh->setInt("sh_degree", 3);

    dispayColliders = true;
}

void Renderer::render(Camera *camera, std::vector<Object *> &selectable, Object *selected, SceneLight *lights, int effect) {
    glViewport(0, 0, screenWidth, screenHeight);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    meshSh->use();
    camera->bind(*meshSh);
    lights->bind(*meshSh);
    for (MeshObject* obj : solidObjects) {
        obj->Draw(*meshSh);
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    
    splatCollection->sort(camera);
    splatSh->use();
    camera->bind(*splatSh); 
    splatSh->setInt("effect", effect);
    splatCollection->draw(*splatSh); 
   
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    
    if (dispayColliders) {
        wireSh->use();
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        camera->bind(*wireSh);
        for (Object *obj : selectable) {
            if (obj == selected) {
                wireSh->setVec3("color", {0.3f, 1.0f, 0.5f});
            } else {
                wireSh->setVec3("color", {1.0f, 0.3f, 0.3f});
            }
            for (Collider* c : obj->colliders) {
                c->draw(*wireSh);
            }
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
}