#ifndef MYGLCANVAS_H
#define MYGLCANVAS_H

#include <iostream>
#include <time.h>

#include <glm/glm.hpp>
#if defined(__APPLE__)
#  include <OpenGL/gl3.h> // defines OpenGL 3.0+ functions
#else
#  if defined(WIN32)
#    define GLEW_STATIC 1
#  endif
#  include <GL/glew.h>
#endif
#include <FL/glut.H>
#include <FL/glu.h>

#include "gfxDefs.h"
#include "ply.h"
// #include "Room.h"
#include "ShaderManager.h"
#include "TextureManager.h"

class MyGLCanvas : public Fl_Gl_Window {
public:

	// Length of our spline (i.e how many points do we randomly generate)


	glm::vec3 eyePosition;
	glm::vec3 rotVec;
	glm::vec3 lookatPoint;
	glm::vec3 lightPos;
	glm::vec3 rotWorldVec;
	glm::vec3 lookVec;
	glm::vec3 upVec;

	int useDiffuse;
	float lightAngle; //used to control where the light is coming from
	int viewAngle;
	float clipNear;
	float clipFar;
	float scaleFactor;
	float textureBlend;

	MyGLCanvas(int x, int y, int w, int h, const char* l = 0);
	~MyGLCanvas();

	void loadPLY(std::string filename);
	void loadEnvironmentTexture(std::string filename);
	void loadObjectTexture(std::string filename);
	void reloadShaders();

private:
	void draw();
	void drawScene();

	void initShaders();

	int handle(int);
	void resize(int x, int y, int w, int h);
	void updateCamera(int width, int height);

	TextureManager* myTextureManager;
	ShaderManager* myShaderManager;
	ply* myObjectPLY;
	ply* myEnvironmentPLY;

	glm::mat4 perspectiveMatrix;

	bool firstTime;
};

#endif // !MYGLCANVAS_H