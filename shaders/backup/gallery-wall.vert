#version 330

in vec3 myPosition;
in vec3 myNormal;

out vec3 fragPosition;
out vec3 fragNormal;

uniform mat4 myModelMatrix;
uniform mat4 myViewMatrix;
uniform mat4 myPerspectiveMatrix;

void main()
{
	fragPosition = myPosition;
	fragNormal = myNormal;

	gl_Position = myPerspectiveMatrix * myViewMatrix * myModelMatrix * vec4(myPosition, 1.0);
}
