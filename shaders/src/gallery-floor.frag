#include "fog.glsl"

in vec3 fragPosition;
in vec3 fragNormal;

out vec4 outputColor;

uniform mat4 myModelMatrix;
uniform mat4 myViewMatrix;
uniform mat4 myPerspectiveMatrix;
uniform sampler2D texture_map;
uniform sampler2D normal_map;
// uniform sampler2D environmentTextureMap;
uniform float textureBlend;
// uniform float sphereScale;
// uniform float sphereRadius;
uniform float PI;

uniform bool useDiffuse;

uniform uint num_spot_lights;
uniform vec3 p_spot_lights[2];
uniform vec3 v_spot_lights[2];
uniform float th_spot_light;
uniform float e_spot_light;

uniform float fog_start;
uniform float fog_end;

uniform uint num_diffuse_lights;
uniform vec3 diffuse_lights[2];

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

void main() {
    // Normal map.
    vec2 uv = cube_to_unit_square(fragPosition, fragNormal);
    vec3 n_vt = 2.0f * texture(normal_map, 3.0 * vec2(uv.y, uv.x)).rgb - 1.0f;
    vec3 t_vo = get_cube_tangent(fragNormal);
    mat3 t2o = get_tbn(fragNormal, t_vo);
    vec3 n_vo = normalize(t2o * n_vt);

	// // Reflect onto environment to get environment color.
    vec3 p_pw = vec3(myModelMatrix * vec4(fragPosition, 1.0));
    // vec3 N_vw = normalize(vec3(transpose(inverse(myModelMatrix)) * vec4(fragNormal, 0.0)));
    vec3 N_vw;
    if (true) {
        N_vw = normalize(vec3(transpose(inverse(myModelMatrix)) * vec4(n_vo, 0.0)));
    } else {
        N_vw = normalize(vec3(transpose(inverse(myModelMatrix)) * vec4(fragNormal, 0.0)));

    }
    float d = 0.0f;
    for (uint ii = 0u; ii < num_diffuse_lights; ++ii) {
        vec3 L_vw = normalize(diffuse_lights[ii] - p_pw);
        d = d + clamp(dot(N_vw, L_vw), 0.0f, 1.0f);
    }
    d = 0.5 * d;
    float ds = 0.0f;
    for (uint ii = 0u; ii < num_spot_lights; ++ii) {
        ds += get_spot_light_intensity(p_spot_lights[ii], v_spot_lights[ii], th_spot_light, e_spot_light, p_pw);
    }
    d = clamp(d + ds, 0.0f, 1.0f);
    vec3 color = vec3(texture(texture_map, 3.0 * vec2(uv.y, uv.x)));
    float depth = -(myViewMatrix * vec4(p_pw, 1.0f)).z;
    vec3 final_color = apply_fog(
        color,
        depth,
        vec3(0.7, 0.7, 0.7),
        fog_start,
        fog_end
    );
    outputColor = vec4(0.7 * d * final_color, 1.0f);
    // outputColor = vec4(d * final_color, 1.0f);
}
