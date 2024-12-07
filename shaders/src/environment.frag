uniform sampler2D environmentTextureMap;
// uniform float sphereScale;
// uniform float sphereRadius;
uniform vec3 lightPos;
uniform bool useDiffuse;
uniform float PI;

in vec3 fragPosition;

out vec4 outputColor;

vec2 textureLocation(vec3 p) {
	float r = 0.5;
    float th = atan(p.z, p.x);
    float u = th < 0 ? - th / (2 * PI):  1 - th / (2 * PI);
    float phi = asin(p.y / r);
    float v = phi / PI + 0.5;

	// Flip them around so we aren't upside-down!
	return vec2(1 - u, 1 - v);
}

void main()
{
	vec3 p_pw = fragPosition * 30.0; // FIXME remove magic number
	float d = 1.0;
	bool environment_diffuse = false;
	if (useDiffuse && environment_diffuse) {
		vec3 L_vw = normalize(lightPos - p_pw);
		vec3 N_vw = -1.0 * normalize(p_pw);
		d = clamp(dot(N_vw, L_vw), 0.0, 1.0);
	}
	vec2 uv = textureLocation(fragPosition);
	// outputColor = d * texture(environmentTextureMap, uv);
	outputColor = vec4(0.7, 0.7, 0.7, 1.0);
}
