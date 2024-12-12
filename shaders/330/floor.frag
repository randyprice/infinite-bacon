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
void main() {
}
