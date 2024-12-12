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
uniform sampler2D environmentTextureMap;
uniform vec3 lightPos;
uniform bool useDiffuse;
uniform float PI;
in vec3 fragPosition;
out vec4 outputColor;
vec2 textureLocation(vec3 p) {
 float r = 0.5;
    float th = atan(p.z, p.x);
    float u = th < 0 ? - th / (2 * PI): 1 - th / (2 * PI);
    float phi = asin(p.y / r);
    float v = phi / PI + 0.5;
 return vec2(1 - u, 1 - v);
}
void main()
{
 vec3 p_pw = fragPosition * 30.0;
 float d = 1.0;
 bool environment_diffuse = false;
 if (useDiffuse && environment_diffuse) {
  vec3 L_vw = normalize(lightPos - p_pw);
  vec3 N_vw = -1.0 * normalize(p_pw);
  d = clamp(dot(N_vw, L_vw), 0.0, 1.0);
 }
 vec2 uv = textureLocation(fragPosition);
 outputColor = vec4(FOG_COLOR, 1.0);
}
