#ifndef FOG_GLSL
#define FOG_GLSL

vec3 FOG_COLOR = vec3(0.8f, 0.8f, 0.8f);

vec3 apply_fog(vec3 color, float depth, vec3 fog_color, float start, float end) {
    start = min(start, end);
    float fog_factor = clamp((end - depth) / (end - start), 0.0, 1.0);
    return mix(FOG_COLOR, color, fog_factor);
}

#endif // FOG_GLSL
