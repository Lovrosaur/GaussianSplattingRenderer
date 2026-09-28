#include <object.h>

Object::~Object() {
    for (size_t i = 0; i < colliders.size(); ++i) {
        delete colliders[i];
        colliders[i] = nullptr;
    }
}
    
void Object::addCollider(Collider* col) {
    col->parentTransform = &transform;
    colliders.push_back(col);
}
    
Object::Object(std::string path, std::string name) : path(path), name(name){};


MeshObject::MeshObject(Model *model) : Object("","Object"), model(model), material(defaultMaterial){}

MeshObject::MeshObject(std::string path, std::string name): Object(path,name), material(defaultMaterial) {
    model = new Model(path);
    loadMaterail(path, material);
}

MeshObject::~MeshObject() {
    delete model;
}

void MeshObject::Draw(Shader &shader) {
    if (!visible) return;
    shader.setVec3("def_diff", material.diffuse);
    shader.setVec3("def_spec", material.specular);
    shader.setFloat("def_Ns", material.n);
    shader.setMat4("model", transform.getModelMatrix());

    model->Draw(shader);
}

    
void MeshObject::addDefaultCollider() {
    glm::vec3 min, max;
    glm::vec3 meshMin, meshMax;
    model->meshes[0].getAABB(meshMin,meshMax);
    min = meshMin;
    max = meshMax;
    for (Mesh &mesh : model->meshes)  {
        mesh.getAABB(meshMin, meshMax);
        min = glm::min(min, meshMin);
        max = glm::max(max, meshMax);
    }

    Collider *col = new Collider();

    col->localTransform.position = (min + max) * 0.5f;
    col->localTransform.scale = (max - min) * 0.5f;
    addCollider(col);
}
   
SplatObject::SplatObject(SplatCollection *collection, std::string path, std::string name) : Object(path,name) {
    setToCollection(collection);
}

void SplatObject::setToCollection(SplatCollection *collection) {
    owner = collection;
    indexInCollection = owner->numModels;
    startIndex = owner->combinedSplats.size();
    if (!owner->addSplat(path, &transform, &visible)) {
        throw 1;
    }
    size = owner->combinedSplats.size() - startIndex;
}

void SplatObject::addDefaultCollider() {
    glm::vec3 min, max;
    glm::vec3 splatMin, splatMax;
    getSplatAABB(owner->combinedSplats[startIndex], splatMin, splatMax);
    min = splatMin;
    max = splatMax;
    for (unsigned int i = startIndex + 1; i < startIndex + size; i++) {
        getSplatAABB(owner->combinedSplats[i], splatMin, splatMax);
        min = glm::min(min, splatMin);
        max = glm::max(max, splatMax);
    }

    Collider *col = new Collider();
    col->localTransform.position = (min + max) * 0.5f;
    col->localTransform.scale = (max - min) * 0.5f;

    addCollider(col);
}