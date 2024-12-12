#ifndef MYGLCANVAS_H
#define MYGLCANVAS_H

#include <iostream>
#include <time.h>
#include <vector>

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
#include <GLFW/glfw3.h>

#include "gfxDefs.h"
#include "ply.h"
#include "ArtManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"

enum CubePly {
	GalleryFloor = 0,
	GalleryWall,
	Painting,
	SideWallNE,
	Max,
};

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

	// Fog.
	float fog_start;
	float fog_end;

	// Lights.
	float spot_light_angle_deg;
	float spot_light_exponent;
	glm::vec3 spot_light_lookat_mod;

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

	void set_eye_position(const glm::vec3& e);
	void maybe_load_art_ppm();
	void maybe_bind_art();
	void maybe_download_art();
	int get_room_number(const glm::vec3& e);
	void preprocess_shaders();
	size_t get_buffer_idx_from_room_number(const int n);

	TextureManager* myTextureManager;
	ShaderManager* myShaderManager;
	std::vector<ply*> cubes;
	// ply* cube_ply;
	ply* myEnvironmentPLY;
	ArtManager* art_manager;
	// SharedData* shared_data;

	glm::mat4 perspectiveMatrix;

	bool firstTime;
	bool art_init;
};

#endif // !MYGLCANVAS_H