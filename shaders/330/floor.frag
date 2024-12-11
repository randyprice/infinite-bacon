#version 330
const float FOG_RGB = 0.0f;
vec3 FOG_COLOR = vec3(FOG_RGB, FOG_RGB, FOG_RGB);
vec3 apply_fog(vec3 color, float depth, vec3 fog_color, float start, float end) {
    start = min(start, end);
    float fog_factor = clamp((end - depth) / (end - start), 0.0, 1.0);
    return mix(FOG_COLOR, color, fog_factor);
}
void main() {
}
