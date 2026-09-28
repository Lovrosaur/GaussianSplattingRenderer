#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragNormal;
in vec3 FragPos;

uniform uint texFlag;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;

uniform vec3 def_diff;
uniform vec3 def_spec;
uniform float def_Ns;
uniform vec3 cam_pos;

#define MAX_POINT_LIGHTS 128
uniform vec3 lightPosition[MAX_POINT_LIGHTS];

uniform float lightConstant[MAX_POINT_LIGHTS];
uniform float lightLinear[MAX_POINT_LIGHTS];
uniform float lightQuadratic[MAX_POINT_LIGHTS];

uniform vec3 lightAmbient[MAX_POINT_LIGHTS];
uniform vec3 lightDiffuse[MAX_POINT_LIGHTS];
uniform vec3 lightSpecular[MAX_POINT_LIGHTS];

uniform int lightCount;

#define DIFFUSE_MAP  1U
#define SPECULAR_MAP 2U
#define NORMAL_MAP   4U
#define HEIGHT_MAP   8U

vec3 CalcPointLight(int li, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol){
    vec3 lightDir = normalize(lightPosition[li] - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), def_Ns);
    float distance    = length(lightPosition[li] - fragPos);
    float attenuation = 1.0 / (lightConstant[li] + lightLinear[li] * distance + 
  			     lightQuadratic[li] * (distance * distance));
    vec3 ambient  = lightAmbient[li] * diffCol;
    vec3 diffuse  = lightDiffuse[li]  * diff * diffCol;
    vec3 specular = lightSpecular[li] * spec * specCol;
    return attenuation * (ambient + diffuse + specular);
} 

void main()
{   
    vec3 normal = FragNormal;
    if ((texFlag & NORMAL_MAP) != 0U) {
        normal = vec3(texture(texture_normal, TexCoords));
    } 
    vec3 diff = def_diff;
    if ((texFlag & 1U) != 0U) {
        diff = vec3(texture(texture_diffuse, TexCoords));
    }

    vec3 spec = def_spec;
    if ((texFlag & SPECULAR_MAP) != 0U) {
        spec = vec3(texture(texture_specular, TexCoords));
    }

    vec3 viewDir = normalize(cam_pos - FragPos);
    vec3 result = vec3(0);
    for(int i = 0; i < lightCount; i++) 
  	    result += CalcPointLight(i, normal, vec3(FragPos), viewDir, diff, spec);
    
    FragColor = vec4(result,1.f);
}