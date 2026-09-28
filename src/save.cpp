#include <save.h>
void saveTransform(FILE* f,  Transform& t) {
    fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
            t.position.x, t.position.y, t.position.z,
            t.rotation.x, t.rotation.y, t.rotation.z,
            t.scale.x, t.scale.y, t.scale.z);
}

void loadTransform(FILE* f, Transform& t) {
    fscanf(f, "%f %f %f %f %f %f %f %f %f\n",
           &t.position.x, &t.position.y, &t.position.z,
           &t.rotation.x, &t.rotation.y, &t.rotation.z,
           &t.scale.x, &t.scale.y, &t.scale.z);
}

void saveCollider(FILE* f, Collider* c) {
    saveTransform(f, c->localTransform);
    fprintf(f, "%d\n", c->active ? 1 : 0);
}

Collider* loadCollider(FILE* f) {
    int active;
    Collider* c = new Collider();
    loadTransform(f, c->localTransform);
    fscanf(f, "%d\n", &active);
    c->active = (active != 0);
    return c;
}


void saveCamera(FILE* f, Camera* cam) {
    if (CameraFxFy* cfx = dynamic_cast<CameraFxFy*>(cam)) {
        (void)cfx;
        fprintf(f, "CAMERA_FXFY\n");
    } else {
        fprintf(f, "CAMERA\n");
    }
    saveTransform(f, cam->transform);
    if (CameraFxFy* cfx = dynamic_cast<CameraFxFy*>(cam)) {
        fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
            cam->worldUp.x, cam->worldUp.y, cam->worldUp.z,
            cam->width, cam->height, cfx->fx, cfx->fy,
            cam->nearPlane, cam->farPlane);
    } else {
        fprintf(f, "%f %f %f %f %f %f %f %f\n",
            cam->worldUp.x, cam->worldUp.y, cam->worldUp.z,
            cam->width, cam->height, cam->fov,
            cam->nearPlane, cam->farPlane);
    }
    
    fprintf(f, "%zu\n", cam->colliders.size());
    for (Collider* c : cam->colliders) {
        saveCollider(f, c);
    }
}

Camera* loadCamera(FILE* f) {
    char type[32];
    fscanf(f, "%31s\n", type);
    Camera* cam = nullptr;
    
    Transform t;
    loadTransform(f, t);

    float ux, uy, uz, w, h, np, fp;
    if (strcmp(type, "CAMERA_FXFY") == 0) {
        float cam_fx, cam_fy;
        fscanf(f, "%f %f %f %f %f %f %f %f %f\n",
            &ux, &uy, &uz, &w, &h, &cam_fx, &cam_fy, &np, &fp);
        cam = new CameraFxFy(t, glm::vec3(ux,uy,uz), w, h, cam_fx, cam_fy, np, fp);
    } else {
        float cam_fov;
        fscanf(f, "%f %f %f %f %f %f %f %f\n",
            &ux, &uy, &uz, &w, &h, &cam_fov, &np, &fp);
        cam = new Camera(t, glm::vec3(ux,uy,uz), w, h, cam_fov, np, fp);
    }

    size_t colCount;
    fscanf(f, "%zu\n", &colCount);
    for(size_t i = 0; i < colCount; ++i) {
        cam->addCollider(loadCollider(f));
    }
    return cam;
}

void saveObject(FILE* f, Object* obj) {
    if (MeshObject* ro = dynamic_cast<MeshObject*>(obj)) {
        (void)ro;
        fprintf(f, "MESH \"%s\" \"%s\"\n%d\n", obj->path.c_str(), obj->name.c_str(), obj->visible ? 1 : 0);
    } else if (SplatObject* so = dynamic_cast<SplatObject*>(obj)) {
        (void)so;
        fprintf(f, "SPLAT \"%s\" \"%s\"\n%d\n", obj->path.c_str(), obj->name.c_str(), obj->visible ? 1 : 0);
    } else {
        return;
    }
    
    saveTransform(f, obj->transform);
    fprintf(f, "%zu\n", obj->colliders.size());
    for (Collider* c : obj->colliders) {
        saveCollider(f, c);
    }
}

Object* loadObject(FILE* f, Renderer* renderer) {
    char type[32];
    char path[256] = "";
    char name[256] = "";
    int visible;
    
    if (fscanf(f, "%31s \"%[^\"]\" \"%[^\"]\"\n%d\n", type, path, name, &visible) != 4) {
        return nullptr;
    }

    Object* obj = nullptr;
    if (strcmp(type, "MESH") == 0) {
        try {
            MeshObject* ro = new MeshObject(path, name);
            obj = ro;
            renderer->solidObjects.push_back(ro);
        } catch(...) { printf("Failed to load MESH %s\n", path); }
    } else if (strcmp(type, "SPLAT") == 0) {
        try {
            SplatObject* so = new SplatObject(renderer->splatCollection, path, name);
            obj = so;
        } catch(...) { printf("Failed to load SPLAT %s\n", path); }
    }

    Transform t;
    loadTransform(f, t);
    
    size_t colCount = 0;
    fscanf(f, "%zu\n", &colCount);
    
    if (obj) {
        obj->visible = (visible != 0);
        obj->transform = t;
        for (size_t i = 0; i < colCount; ++i) {
            obj->addCollider(loadCollider(f));
        }
    } else {
        for (size_t i = 0; i < colCount; ++i) {
            Collider* c = loadCollider(f);
            delete c;
        }
    }
    return obj;
}