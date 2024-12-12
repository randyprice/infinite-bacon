#include "fog.glsl"

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

uniform uint num_diffuse_lights;
uniform vec3 diffuse_lights[2];

uniform uint num_spot_lights;
uniform vec3 p_spot_lights[2];
uniform vec3 v_spot_lights[2];
uniform float th_spot_light;
uniform float e_spot_light;

uniform bool useDiffuse;

uniform float fog_start;
uniform float fog_end;

uniform bool transparent;

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
    vec3 p_pw = vec3(myModelMatrix * vec4(fragPosition, 1.0));
    vec3 N_vw = normalize(vec3(transpose(inverse(myModelMatrix)) * vec4(fragNormal, 0.0)));
    float d = 0.0f;
    for (uint ii = 0u; ii < num_diffuse_lights; ++ii) {
        vec3 L_vw = normalize(diffuse_lights[ii] - p_pw);
        d = d + clamp(dot(N_vw, L_vw), 0.0f, 1.0f);
    }
    for (uint ii = 0u; ii < 2u; ++ii) {
        d += get_spot_light_intensity(p_spot_lights[ii], v_spot_lights[ii], th_spot_light, e_spot_light, p_pw);
    }
    d = clamp(d, 0.0f, 1.0f);

    vec2 uv = cube_to_unit_square();
    vec3 color = vec3(texture(texture_map, uv));
    float depth = -(myViewMatrix * vec4(p_pw, 1.0f)).z;
    vec3 final_color = apply_fog(
        color,
        depth,
        vec3(0.7, 0.7, 0.7),
        fog_start,
        fog_end
    );

    outputColor = vec4( d * final_color, 1.0f);

    // outputColor = vec4(0.7 * d * final_color, 1.0f);
}
