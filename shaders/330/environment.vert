#version 330
in vec3 myNormal;
in vec3 myPosition;
out vec3 fragPosition;
uniform mat4 myModelMatrix;
uniform mat4 myViewMatrix;
uniform mat4 myPerspectiveMatrix;
uniform mat4 M;
uniform float sphereScale;
uniform float fog_end;
void main()
{
 vec4 pos = myPerspectiveMatrix * myViewMatrix * M * vec4(4.0 * fog_end * myPosition, 1.0);
 fragPosition = myPosition;
 gl_Position = pos;
}
