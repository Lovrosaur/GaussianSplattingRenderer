#version 430 core

layout (location = 0) in vec2 quadPosition; 

struct Splat {
    vec3 center;
    uint id;
    vec3 scale;
    float opacity;
    vec4 rot;
    vec4 sh[16];
};

layout (std430, binding=1) buffer gaussians_index {
	uint sortedIdx[];
};

layout(std430, binding=2) readonly buffer splat_buffer {
    Splat splats[];
};

layout(std430, binding = 3) buffer Models {
    mat4 models[];
};

uniform mat4 view;
uniform vec3 cam_pos;
uniform mat4 projection;
uniform vec2 focal;
uniform vec2 viewport;
uniform int sh_degree;

out vec4 outColor;
out vec2 uv;

const float SH_C0 = 0.28209479177387814;
const float SH_C1 = 0.4886025119029199;
const float SH_C2[5] = float[5](
    1.0925484305920792,
    -1.0925484305920792,
    0.31539156525252005,
    -1.0925484305920792,
    0.5462742152960396
);
const float SH_C3[7] = float[7](
    -0.5900435899266435,
    2.890611442640554,
    -0.4570457994644658,
    0.3731763325901154,
    -0.4570457994644658,
    1.445305721320277,
    -0.5900435899266435
);

vec3 get_rgb(vec4 sh[16], vec3 dir) {
    vec3 rgb = vec3(0.5);
    rgb += SH_C0 * sh[0].rgb;

    if (sh_degree >= 1) {
        rgb +=
            - SH_C1 * dir.y * sh[1].rgb
            + SH_C1 * dir.z * sh[2].rgb
            - SH_C1 * dir.x * sh[3].rgb;
    }

    if (sh_degree >= 2) {
        float xx = dir.x * dir.x;
        float yy = dir.y * dir.y;
        float zz = dir.z * dir.z;
        float xy = dir.x * dir.y;
        float yz = dir.y * dir.z;
        float xz = dir.x * dir.z;
        rgb +=
            SH_C2[0] * xy * sh[4].rgb +
            SH_C2[1] * yz * sh[5].rgb +
            SH_C2[2] * (2.0 * zz - xx - yy) * sh[6].rgb +
            SH_C2[3] * xz * sh[7].rgb +
            SH_C2[4] * (xx - yy) * sh[8].rgb;

        if (sh_degree >= 3) {
            rgb +=
                SH_C3[0] * dir.y * (3.0 * xx - yy) * sh[9].rgb +
                SH_C3[1] * dir.z * xy * sh[10].rgb +
                SH_C3[2] * dir.y * (4.0 * zz - xx - yy) * sh[11].rgb +
                SH_C3[3] * dir.z * (2.0 * zz - 3.0 * xx - 3.0 * yy) * sh[12].rgb +
                SH_C3[4] * dir.x * (4.0 * zz - xx - yy) * sh[13].rgb +
                SH_C3[5] * dir.z * (xx - yy) * sh[14].rgb +
                SH_C3[6] * dir.x * (xx - 3.0 * yy) * sh[15].rgb;
        }
    }
    return clamp(rgb, 0.0, 1.0);
}

mat3 computeCov(vec4 rot, vec3 scale) {
    mat3 R = mat3(
        vec3(1.f -  2.f*(rot.z * rot.z + rot.w * rot.w), 
                    2.f*(rot.y * rot.z - rot.x * rot.w), 
                    2.f*(rot.y * rot.w + rot.x * rot.z)),
        vec3(       2.f*(rot.y * rot.z + rot.x * rot.w), 
             1.f -  2.f*(rot.y * rot.y + rot.w * rot.w), 
                    2.f*(rot.z * rot.w - rot.x * rot.y)),
        vec3(       2.f*(rot.y * rot.w - rot.x * rot.z), 
                    2.f*(rot.z * rot.w + rot.x * rot.y), 
             1.f -  2.f*(rot.y * rot.y + rot.z * rot.z))
    );
    
    mat3 S = mat3(
        scale.x, 0, 0,
        0, scale.y, 0,
        0, 0, scale.z
    );

    mat3 M = transpose(R) * S; //transpores jer je mat3 inicializacija column major
    return M * transpose(M);
}

#define N_SIGMA 3


void main () {
    uint splatInd = sortedIdx[gl_InstanceID];
    const Splat splat = splats[splatInd];
    mat4 model = models[splat.id];
    vec4 splat_pos = model * vec4(splat.center,1);
    splat_pos = splat_pos/splat_pos.w;
    vec4 camspace = view * model * vec4(splat.center,1);
    vec4 pos2d = projection * camspace;
    pos2d /= pos2d.w;
    // float bounds = 1.3 * pos2d.w;
    if (pos2d.z < -1
        // || pos2d.x < -bounds
        // || pos2d.x > bounds
        // || pos2d.y < -bounds
        // || pos2d.y > bounds
        ) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    mat3 cov3d = computeCov(splat.rot, splat.scale);

    const float limx = 1.3f * viewport.x/(2*focal.x);
	const float limy = 1.3f * viewport.y/(2*focal.y);
	const float txtz = camspace.x / camspace.z;
	const float tytz = camspace.y / camspace.z;
	camspace.x = min(limx, max(-limx, txtz)) * camspace.z;
	camspace.y = min(limy, max(-limy, tytz)) * camspace.z;

    mat3 J = mat3(
        vec3(focal.x / camspace.z, 0.0, 0.0),            
        vec3(0.0, focal.y / camspace.z, 0.0),          
        vec3(-(focal.x * camspace.x) / (camspace.z * camspace.z), 
            -(focal.y * camspace.y) / (camspace.z * camspace.z), 
            0.0)                                     
    );


    mat3 T = J * mat3(view) * mat3(model);
    mat3 cov2d = T * cov3d * transpose(T);
    vec3 cov = vec3(cov2d[0][0], cov2d[0][1], cov2d[1][1]);
    cov.x += 0.3;
    cov.z += 0.3;
    float det = cov.x*cov.z - cov.y*cov.y;
    // cov = clamp(cov,-2e6,2e6);
    if (abs(det) < 1e-4) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }

    float mid = 0.5 * (cov.x + cov.z);
    float radius = length(vec2((cov.x - cov.z) / 2.0, cov.y));
    float lambda1 = mid + radius;
    float lambda2 = mid - radius;

    if (sqrt(lambda1 * lambda2 * 6) < 1) {  
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    vec2 v1 = vec2(1.0, 0.0);
    vec2 v2 = vec2(0.0, 1.0);

    if (abs(cov.y) > 0.0001) {
        v1 = normalize(vec2(cov.y, lambda1 - cov.x));
        v2 = vec2(-v1.y, v1.x);
    }

    v1 *= min(sqrt(lambda1),1024) * 2.f / viewport;
    v2 *= min(sqrt(lambda2),1024) * 2.f / viewport;
    
    vec2 center2d = vec2(pos2d);
    
    uv = N_SIGMA * quadPosition;
    vec3 dir = normalize(vec3(splat_pos) - cam_pos);
    outColor = vec4(get_rgb(splat.sh, dir), splat.opacity);

    gl_Position = vec4(
      center2d
        + N_SIGMA * quadPosition.x * v1
        + N_SIGMA * quadPosition.y * v2, 
        pos2d.z, 1.0);
}
