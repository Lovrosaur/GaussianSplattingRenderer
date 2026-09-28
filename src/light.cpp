#include <light.h>

void SceneLight::bind(Shader &shader) {
    shader.setVec3("lightPosition", position);
    shader.setFloat("lightConstant", constant);
    shader.setFloat("lightLinear", linear);
    shader.setFloat("lightQuadratic", quadratic);
    shader.setVec3("lightAmbient", ambient);
    shader.setVec3("lightDiffuse", diffuse);
    shader.setVec3("lightSpecular", specular);
    shader.setInt("lightCount", position.size());
}

void SceneLight::addLight(Light light) {
    position.push_back(light.position);
    constant.push_back(light.constant);
    linear.push_back(light.linear);
    quadratic.push_back(light.quadratic);
    ambient.push_back(light.ambient);
    diffuse.push_back(light.diffuse);
    specular.push_back(light.specular);
}
