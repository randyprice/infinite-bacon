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
#include "ArtManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"

// class SharedData {
// public:
// 	SharedData(const size_t buffer_size) {
// 		this->buffer_size = buffer_size;
// 		this->loading = false;
// 		this->buffer_idx = 0; // this tells MyGLCanvas which texture to load to in the buffer
// 	}
// 	size_t get_buffer_size() { return this->buffer_size; }
// 	size_t get_buffer_idx() { return this->buffer_idx; }
// 	void set_buffer_idx(const size_t buffer_idx) { this->buffer_idx = buffer_idx; }
// 	bool is_loading() { return this->loading; }
// 	void set_loading() { this->loading = true; }
// 	void unset_loading() { this->loading = false; }

// private:
// 	int buffer_size;
// 	bool loading;
// 	size_t buffer_idx;
// 	// char colors[5][4096 * 4096 * 3]
// };

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

	void set_eye_position(const glm::vec3& e);
	void maybe_load_art_ppm();
	void maybe_bind_art();
	int get_room_number(const glm::vec3& e);
	void preprocess_shaders();
	size_t get_buffer_idx_from_room_number(const int n);

	TextureManager* myTextureManager;
	ShaderManager* myShaderManager;
	ply* cube_ply;
	ply* myEnvironmentPLY;
	ArtManager* art_manager;
	// SharedData* shared_data;

	glm::mat4 perspectiveMatrix;

	bool firstTime;
	bool art_init;
};

#endif // !MYGLCANVAS_H