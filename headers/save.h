#pragma once
#include <transform.h>
#include <camera.h>
#include <object.h>
#include <renderer.h>

void saveTransform(FILE* f, Transform& t);
void loadTransform(FILE* f, Transform& t);

void saveCollider(FILE* f, Collider* c);
Collider* loadCollider(FILE* f);


void saveCamera(FILE* f, Camera* cam);
Camera* loadCamera(FILE* f);

void saveObject(FILE* f, Object* obj);
Object* loadObject(FILE* f, Renderer* renderer);