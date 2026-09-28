#pragma once
#include <transform.h>
#include <string>
#include <material.h>
#include <memory>
#include <shader.h>
#include <collision.h>
#include <mesh.h>
#include <splat.h>
class Object {
    static int IDcount;
public:
    Transform transform;
    std::vector<Collider*> colliders;
    std::string path;
    std::string name;
    bool visible = true;
    
    virtual ~Object();
    
    void addCollider(Collider* col);
    
    Object(std::string path, std::string name = "Object");
};

class MeshObject : public Object {
public:
    Model *model;
    Material material;

    MeshObject(Model *model);
    MeshObject(std::string path, std::string name = "Object");

    ~MeshObject() override;

    void Draw(Shader &shader);

    void addDefaultCollider();
};

class SplatObject : public Object {
public:
    SplatCollection *owner;
    unsigned int indexInCollection;
    unsigned int startIndex;
    unsigned int size;
    
    SplatObject(SplatCollection *collection, std::string path, std::string name = "Object");

    void setToCollection(SplatCollection *collection);

    void addDefaultCollider();
};

