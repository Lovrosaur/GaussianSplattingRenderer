#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cfloat>
#include <cstdio>
#include <string>
#include <vector>
#include <shader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "collision.h"
#include "glm/ext/vector_float2.hpp"
#include <mesh.h>
#include <camera.h>
#include <transform.h>
#include <object.h>
#include <mesh.h>
#include <light.h>
#include <splat.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <chrono>
#include <thread>
#include <renderer.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable-file-dialogs.h>
#include <save.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

const unsigned int SCR_WIDTH = 1959/2;
const unsigned int SCR_HEIGHT = 1090/2;

float dt = 0;
const float FPS = 60;

#define XRAY 1;

struct {
    std::string sceneName = "";
    bool modified = false;
    int screen_w = SCR_WIDTH, screen_h = SCR_HEIGHT;

    std::vector<Object *> selectable;
    Object *selected;
    Camera *active_camera;
    SceneLight *lights;
    Renderer *renderer;

    int effect = 0;
} scene;

void deleteScene() {
    for (Object* obj : scene.selectable) {
        delete obj;
    }
    scene.selectable.clear();
    scene.selected = nullptr;
    
    delete scene.active_camera; 
    scene.active_camera = nullptr;
    delete scene.lights; 
    scene.lights = nullptr;
    delete scene.renderer; 
    scene.renderer = nullptr;
}

void initScene(int width, int height) {
    scene.renderer = new Renderer();
    scene.renderer->setScreen(width, height);
    
    scene.lights = new SceneLight();
    scene.lights->addLight({
        glm::vec3(40, -40, 40),
        1., 0., 0.,
        glm::vec3(0.22),
        glm::vec3(0.78),
        glm::vec3(0.78)
    });
    
    scene.sceneName = "";
    scene.modified = false;
}

void loadDefaultCamera() {
    glm::vec3 forward = glm::vec3(0.477065f, -0.057586f, 0.876978f);
    glm::vec3 up = -glm::vec3(0.069259f, 0.997211f, 0.027805f);
    scene.active_camera = new CameraFxFy(
        glm::vec3(-3.008989f, -0.110864f, -3.752764f),
        forward, up, SCR_WIDTH * 2, SCR_HEIGHT * 2,
        1159.588073f, 1164.660128f, 0.2f, 1000.f
    );
    Collider *col = new Collider();
    col->localTransform.scale = {scene.active_camera->nearPlane, scene.active_camera->nearPlane, scene.active_camera->nearPlane};
    scene.active_camera->addCollider(col);
}

void saveSceneToFile(const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return;

    saveCamera(f, scene.active_camera);
    fprintf(f, "\n%zu\n", scene.selectable.size());
    for (Object* obj : scene.selectable) {
        saveObject(f, obj);
    }
    
    fclose(f);
    scene.sceneName = filepath;
    scene.modified = false;
}

void loadSceneFromFile(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return;
    
    scene.active_camera = loadCamera(f);
    
    size_t objCount = 0;
    fscanf(f, "\n%zu\n", &objCount);
    for (size_t i = 0; i < objCount; ++i) {
        Object* obj = loadObject(f, scene.renderer);
        if (obj) scene.selectable.push_back(obj);
    }
    
    fclose(f);
    scene.sceneName = filepath;
    scene.modified = false;
}

bool actionSaveAs() {
    pfd::save_file dialog("Save scene as", ".", { "Scene Files", "*.txt" }, pfd::opt::none);
    while(!dialog.ready(500));
    
    if (!dialog.result().empty()) {
        std::string res = dialog.result();
        if (res.find(".txt") == std::string::npos) res += ".txt";
        saveSceneToFile(res.c_str());
        return true;
    }
    return false;
}

bool actionSave() {
    if (scene.sceneName.empty()) {
        return actionSaveAs();
    } else {
        saveSceneToFile(scene.sceneName.c_str());
        return true;
    }
}

bool checkModifed() {
    if (!scene.modified) return true;
    auto choice = pfd::message("Unsaved Changes", 
                               "Save unsaved changes?", 
                               pfd::choice::yes_no_cancel).result();
                               
    if (choice == pfd::button::cancel) {
        return false;
    }
    if (choice == pfd::button::yes) {
        return actionSave();
    }
    return true;
}

void actionNew() {
    if (!checkModifed()) return;
    
    int w = scene.screen_w;
    int h = scene.screen_h;
    
    deleteScene();
    initScene(w, h);
    loadDefaultCamera();
}

void actionOpen() {
    if (!checkModifed()) return;

    pfd::open_file dialog("Choose scene to open", ".", { "Scene Files", "*.txt", "All Files", "*"}, pfd::opt::none);
    while(!dialog.ready(500));
    
    if (!dialog.result().empty()) {
        int w = scene.screen_w;
        int h = scene.screen_h;
        
        deleteScene();
        initScene(w, h);
        loadSceneFromFile(dialog.result()[0].c_str());
    }
}

void actionAdd() {
    pfd::open_file dialog("Choose object to add", ".",
                            { "All Files", "*" ,
                                "Mesh Object (.obj)", "*.obj",
                                "Splat Object (.ply)", "*.ply"},
                            pfd::opt::none);
                    
    while(!dialog.ready(500));

    if (dialog.result().size() > 0) {
        std::string filePath = dialog.result()[0];
        std::replace(filePath.begin(), filePath.end(), '\\', '/');
        if (filePath.find(".ply") != std::string::npos) {
            try{
                SplatObject *object = new SplatObject(scene.renderer->splatCollection, filePath);
                object->addDefaultCollider();
                scene.selectable.push_back(object);
                scene.modified = true;
            }catch (int err) {
                printf("Failed to load spalt: %s\n", filePath.c_str());
            }
        }
        if (filePath.find(".obj") != std::string::npos) {
            try {
                MeshObject *object = new MeshObject(filePath);
                object->addDefaultCollider();
                scene.renderer->solidObjects.push_back(object);
                scene.selectable.push_back(object);
                scene.modified = true;
            }catch (int err) {
                printf("Failed to load model: %s\n", filePath.c_str());
            }
        }
    }
}

void actionExit(GLFWwindow* window) {
    if (!checkModifed()) return;
    glfwSetWindowShouldClose(window, true);
}

void window_close_callback(GLFWwindow* window)
{
    glfwSetWindowShouldClose(window, GLFW_FALSE); 
    actionExit(window);
}


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pogon za prikazivenje - Diplomski rad", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    IMGUI_CHECKVERSION();   
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true); 
    ImGui_ImplOpenGL3_Init("#version 430"); 

    double startTime = glfwGetTime(), endTime;
    double frameTime;
    const double targetTime = 1./FPS;
    
    initScene(SCR_WIDTH, SCR_HEIGHT);
    loadDefaultCamera();    

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N)) {
            actionNew();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O)) {
            actionOpen();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
            actionSave();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
            actionSaveAs();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl + ImGuiKey_A)) {
            actionAdd();
        }
        if (ImGui::BeginMainMenuBar()) {
            
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) { 
                    actionNew();
                }
                if (ImGui::MenuItem("Open", "Ctrl+O")) { 
                    actionOpen();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S" /*, false, !scene.sceneName.empty()*/)) { 
                    actionSave();
                }
                if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) { 
                    actionSaveAs();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Add", "Ctrl+A")) { 
                    actionAdd();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Esc")) {
                    actionExit(window);
                    continue;
                }
                
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetTextLineHeightWithSpacing()), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0.3*io.DisplaySize.x, 12 * ImGui::GetTextLineHeightWithSpacing()), ImGuiCond_FirstUseEver); 
        ImGui::Begin("Object List");
        if (ImGui::BeginListBox("##Object List", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
            for (size_t i = 0; i < scene.selectable.size(); i++)
            {
                Object* current_item = scene.selectable[i];
                
                bool is_selected = (scene.selected == current_item);
                ImGui::PushID(i);
                if (ImGui::Selectable(current_item->name.c_str(), is_selected))
                {
                    scene.selected = current_item;
                }
                ImGui::PopID();

            }
            ImGui::EndListBox();
        }
        ImGui::Checkbox("Display colliders",&scene.renderer->dispayColliders);
        ImGui::End();

        static char renameBuffer[256] = "";

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x, ImGui::GetTextLineHeightWithSpacing()), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::Begin("Properties");
        ImGui::Text("FPS: %0.f", 1./dt);
        if (scene.selected) {
            ImGui::Checkbox("Visible", &scene.selected->visible);
            ImGui::SeparatorText("Transform");
            
            if (ImGui::DragFloat3("Position", &(scene.selected->transform.position.x), 0.05f) 
            | ImGui::DragFloat3("Rotation", &(scene.selected->transform.rotation.x), 1.f, 0,  0,  "%.1f")
            | ImGui::DragFloat3("Scale", &(scene.selected->transform.scale.x), 0.01f, 0.0001f, 100.0f)) {
                scene.modified = true;
            }

            if (ImGui::Button("Reset Transform")) {
                scene.selected->transform = Transform();
                scene.modified = true;
            }

            ImGui::SeparatorText("Rename");
            if (ImGui::InputText("##Rename", renameBuffer, IM_ARRAYSIZE(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (strlen(renameBuffer) > 0) {
                    scene.selected->name = renameBuffer;
                    renameBuffer[0] = '\0';
                    scene.modified = true;
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                if (strlen(renameBuffer) > 0) {
                    scene.selected->name = renameBuffer;
                    renameBuffer[0] = '\0';
                    scene.modified = true;
                }
            }
            ImGui::SeparatorText("Colliders");
            for (size_t i = 0; i < scene.selected->colliders.size(); i++) {
                if (ImGui::CollapsingHeader(("Collider " + std::to_string(i+1)).c_str())) {
                    ImGui::PushID(i);
                    Collider *col = scene.selected->colliders[i];
                    if (ImGui::Checkbox("Active", &(col->active))) {
                        scene.modified = true;
                    }
                    if (ImGui::DragFloat3("Position##Col", &(col->localTransform.position.x), 0.05f) 
                    | ImGui::DragFloat3("Rotation##Col", &(col->localTransform.rotation.x), 1.f, 0,  0,  "%.1f")
                    | ImGui::DragFloat3("Scale##Col", &(col->localTransform.scale.x), 0.01f, 0.0001f, 100.0f)) {
                        scene.modified = true;
                    }
                    if (ImGui::Button("Duplicate")) {
                        Collider *dup = new Collider(&scene.selected->transform);
                        dup->localTransform = col->localTransform;
                        dup->active = col->active;
                        scene.selected->addCollider(dup);
                        scene.modified = true;
                    }
                    ImGui::PopID();
                }
            }

        } 
        else {
            ImGui::Text("No object selected");
        }

        ImGui::End();
        ImGui::EndFrame();

        // double t1 = glfwGetTime();
        scene.renderer->render(scene.active_camera, scene.selectable, scene.selected, scene.lights, scene.effect);
        glFinish();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // printf("%f  |", glfwGetTime()-t1);
        // double t2 = glfwGetTime();
        glfwSwapBuffers(window);
        // printf("%f  |", glfwGetTime()-t2);

        glfwPollEvents();

        endTime = glfwGetTime();
        frameTime = endTime - startTime;
        // if (FPS > 0 && frameTime < targetTime) {
        //     std::this_thread::sleep_for(std::chrono::microseconds((int)(1000000 * ((targetTime-frameTime)))));
        //     // printf("%f - %f = %f, %d||",targetTime, frameTime, targetTime-frameTime, (int)(1000000 * ((targetTime-frameTime))));
        // }
        startTime = glfwGetTime();
        dt = frameTime  + (startTime - endTime);
        // printf("%f, %f, %.0f FPS\n", startTime - endTime, frameTime, 1./dt);
        fflush(stdout);
    }

    deleteScene();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


const float m_speed = 3;
const float m_speed_f = 15;
float sensitivity = 0.1f;
const float r_speed = 90./2;

void processInput(GLFWwindow *window)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        return; 
    }

    float speed = m_speed;  
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed = m_speed_f;
    
    bool yLock = true;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
        yLock = false;

    glm::vec3 moveVec(0);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(0,0,1);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(0,0,-1);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(1,0,0);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(-1,0,0);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(0,1,0);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        moveVec += speed * dt * glm::vec3(0,-1,0);

    
    

    static glm::vec2 last = glm::vec2(SCR_WIDTH / 2.0, SCR_HEIGHT / 2.0);
    static bool dragging = false;
    bool rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS; // orbit
    glm::vec2 rotateVec(0);
    if (rmb || mmb) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (!dragging) {
            last.x = xpos;
            last.y = ypos;
            dragging = true;
        }

        float xdiff = xpos - last.x;
        float ydiff = last.y - ypos;
        last.x = xpos;
        last.y = ypos;
        rotateVec.x = xdiff * sensitivity;
        rotateVec.y = ydiff * sensitivity;
    } else {
        dragging = false;
    }
    if (!dragging) {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            rotateVec += r_speed * dt * glm::vec2(0,1);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            rotateVec += r_speed * dt * glm::vec2(0,-1);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            rotateVec += r_speed * dt * glm::vec2(1,0);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            rotateVec += r_speed * dt * glm::vec2(-1,0);
    }
    scene.active_camera->move(moveVec, yLock);
    for (Object *obj  : scene.selectable) {
        for (Collider* c : obj->colliders) {
            if (c->check(*scene.active_camera->colliders[0])){
                scene.active_camera->move(-moveVec, yLock);
                
            }
        }
    }

    
    scene.active_camera->rotate(rotateVec, mmb);
    for (Object *obj  : scene.selectable) {
        for (Collider* c : obj->colliders) {
            if (c->check(*scene.active_camera->colliders[0])){
                scene.active_camera->rotate(-rotateVec, mmb);
            }
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        actionExit(window);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        return; 
    }
    
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        scene.effect ^= 1; 
    }
}
void framebuffer_size_callback(GLFWwindow* /* window */, int width, int height)
{
    scene.renderer->setScreen(width, height);
    scene.screen_w = width;
    scene.screen_h = height;
}

