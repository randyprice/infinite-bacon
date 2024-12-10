#version 330
const float FOG_RGB = 0.8f;
vec3 FOG_COLOR = vec3(FOG_RGB, FOG_RGB, FOG_RGB);
vec3 apply_fog(vec3 color, float depth, vec3 fog_color, float start, float end) {
    start = min(start, end);
    float fog_factor = clamp((end - depth) / (end - start), 0.0, 1.0);
    return mix(FOG_COLOR, color, fog_factor);
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
