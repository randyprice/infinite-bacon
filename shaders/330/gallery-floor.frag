#version 330

in vec3 fragPosition;
in vec3 fragNormal;

out vec4 outputColor;

uniform mat4 myModelMatrix;
uniform mat4 myViewMatrix;
uniform mat4 myPerspectiveMatrix;
uniform sampler2D texture_map;
// uniform sampler2D environmentTextureMap;
uniform float textureBlend;
// uniform float sphereScale;
// uniform float sphereRadius;
uniform float PI;

uniform vec3 lightPos;
uniform bool useDiffuse;

// // Map a point in object space to the unit square.
// vec2 to_unit_square(vec3 p) {
//     float r = sphereRadius;
//     float th = atan(p.z, p.x);
//     float u = th < 0 ? - th / (2 * PI):  1 - th / (2 * PI);
//     float phi = asin(p.y / r);
//     float v = phi / PI + 0.5;

//     return vec2(u, v);
// }

// // Project a point onto the sphere.
// vec3 project_onto_sphere(vec3 p) {
//     const float r = 0.5;

//     return r * normalize(p);
// }

// Return `t` such that `p = td` is the closest intersect point in front of the
// camera.
float quadratic(float A, float B, float C) {
    float d = pow(B, 2) - 4 * A * C;
    if (d < 0) {
        return -1.0;
    }

    float t0 = (-B + sqrt(d)) / (2 * A);
    float t1 = (-B - sqrt(d)) / (2 * A);

    if (!(t0 < 0) && !(t1 < 0)) {
        return min(t0, t1);
    } else if (!(t0 < 0)) {
        return t0;
    } else if (!(t1 < 0)) {
        return t1;
    } else {
        return -1.0;
    }
}

// // Return isect point in world space.
// vec3 intersect_sphere(vec3 p, vec3 d) {
//     float r = sphereRadius * sphereScale;
//     float A = dot(d, d);
//     float B = 2.0 * dot(p, d);
//     float C = dot(p, p) - pow(r, 2);
//     float t = quadratic(A, B, C);
//     if (t < 0) {
//         return vec3(0.0, 0.0, 0.0);
//     }

//     return  p + t * d;
// }

const uint CUBE_SIDE_NEG_X = 0u;
const uint CUBE_SIDE_POS_X = 1u;
const uint CUBE_SIDE_NEG_Y = 2u;
const uint CUBE_SIDE_POS_Y = 3u;
const uint CUBE_SIDE_NEG_Z = 4u;
const uint CUBE_SIDE_POS_Z = 5u;
const uint CUBE_SIDE_MAX = 6u;

uint get_cube_side() {
    if (round(fragNormal.x) == -1) {
        return CUBE_SIDE_NEG_X;
    } else if (round(fragNormal.x) == 1) {
        return CUBE_SIDE_POS_X;
    } else if (round(fragNormal.y) == -1) {
        return CUBE_SIDE_NEG_Y;
    } else if (round(fragNormal.y) == 1) {
        return CUBE_SIDE_POS_Y;
    } else if (round(fragNormal.z) == -1) {
        return CUBE_SIDE_NEG_Z;
    } else if (round(fragNormal.z) == 1) {
        return CUBE_SIDE_POS_Z;
    } else {
        return CUBE_SIDE_MAX;
    }

}

vec2 cube_to_unit_square() {
    uint side = get_cube_side();
    float l = 1.0f;
    vec3 p = fragPosition;
    if (side == CUBE_SIDE_NEG_X) {
        return vec2(p.z + l / 2.0f, p.y + l / 2.0f);
    } else if (side == CUBE_SIDE_POS_X) {
        return vec2(-p.z + l / 2.0f, p.y + l / 2.0f);
    } else if (side == CUBE_SIDE_NEG_Y) {
        return vec2(p.x + l / 2.0f, p.z + l / 2.0f);
    } else if (side == CUBE_SIDE_POS_Y) {
        return vec2(p.x + l / 2.0f, -p.z + l / 2.0f);
    } else if (side == CUBE_SIDE_NEG_Z) {
        return vec2(-p.x + l / 2.0f, p.y + l / 2.0f);
    } else if (side == CUBE_SIDE_POS_Z) {
        return vec2(p.x + l / 2.0f, p.y + l / 2.0f);
    } else {
        return vec2(0.0f, 0.0f);
    }

}

void main() {
	// // Reflect onto environment to get environment color.
    // vec3 e_pw = vec3(inverse(myViewMatrix) * inverse(myPerspectiveMatrix) * vec4(0.0, 0.0, 0.0, 1.0));
    // vec3 p_pw = vec3(myModelMatrix * vec4(fragPosition, 1.0));
    // vec3 I_vw = normalize(p_pw - e_pw);
    // vec3 N_vw = normalize(vec3(transpose(inverse(myModelMatrix)) * vec4(fragNormal, 0.0)));
    // vec3 R_vw = reflect(I_vw, N_vw);
    // vec3 pi_pw = intersect_sphere(p_pw, R_vw);
    // vec3 pi_po = pi_pw / sphereScale;
    // vec2 env_uv = to_unit_square(pi_po);
    // vec4 env_color = texture(environmentTextureMap, clamp(1 - env_uv, 0.0, 1.0));

	// // Object color. Project onto sphere.
    // vec2 obj_uv = to_unit_square(project_onto_sphere(fragPosition));
    // vec4 obj_color = texture(objectTextureMap, clamp(vec2(obj_uv.x,obj_uv.y), 0.0, 1.0));

	// // Diffuse.
    // float d = 1.0;
    // if (useDiffuse) {
    //     vec3 L_vw = normalize(lightPos - p_pw);
    //     d = clamp(dot(N_vw, L_vw), 0.0, 1.0);
    // }

    // outputColor = d * (textureBlend * obj_color + (1.0 - textureBlend) * env_color);
    // outputColor = vec4(fragPosition, 1.0);
    vec2 uv = cube_to_unit_square();
    uv = uv * 3;
    outputColor = texture(texture_map, uv);
    // outputColor = vec4(fragNormal, 1.0);
    // outputColor = vec4(1.0, 0.0, 0.0, 1.0);
}
