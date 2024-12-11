#version 330
vec3 applyFog(vec3 color, float depth, vec3 fog_color, float fog_start, float fog_end) {
    float fog_factor = clamp((fog_end - depth) / (fog_end - fog_start), 0.0, 1.0);
    return mix(fog_color, color, fog_factor);
}
void main() {
}
