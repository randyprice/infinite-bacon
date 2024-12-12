#version 330
const float FOG_RGB = 0.6f;
vec3 FOG_COLOR = vec3(0.6, 0.7, 0.9);
vec3 apply_fog(vec3 color, float depth, vec3 fog_color, float start, float end) {
    start = min(start, end);
    float fog_factor = clamp((end - depth) / (end - start), 0.0, 1.0);
    return mix(FOG_COLOR, color, fog_factor);
}
float get_spot_light_intensity(vec3 _light_pw, vec3 _light_vw, float _light_angle_rad, float _light_exponent, vec3 _surface_pw) {
    vec3 D = _light_vw;
    vec3 L = normalize(vec3(_light_pw - _surface_pw));
    float f = dot(D, -1.0f * L);
    if (f < 0.0f) {
        return 0.0f;
    }
    float th = acos(f);
    if (th > _light_angle_rad) {
        return 0.0f;
    } else {
        return pow(f, _light_exponent);
    }
}
in vec3 fragPosition;
in vec3 fragNormal;
out vec4 outputColor;
uniform mat4 myModelMatrix;
uniform mat4 myViewMatrix;
uniform mat4 myPerspectiveMatrix;
uniform sampler2D texture_map;
uniform float textureBlend;
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
}
