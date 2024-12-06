#include <Fl/Fl.H>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "MyGLCanvas.h"


MyGLCanvas::MyGLCanvas(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x, y, w, h, l) {
	mode(FL_OPENGL3 | FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);

	// eyePosition = glm::vec3(0.0f, 0.0f, 3.0f);
	// lookatPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	eyePosition = glm::vec3(0.0f, 3.0f, 0.0f);
	lookatPoint = glm::vec3(0.0f, 3.0f, -1.0f);
	lookVec = glm::normalize(lookatPoint - eyePosition);
	rotVec = glm::vec3(0.0f, 0.0f, 0.0f);
	upVec = glm::vec3(0.0f, 1.0f, 0.0f);
	lightPos = eyePosition;

	viewAngle = 60;
	clipNear = 0.01f;
	clipFar = 100.0f;
	scaleFactor = 1.0f;
	lightAngle = 0.0f;
	textureBlend = 0.0f;

	useDiffuse = false;

	firstTime = true;

	myTextureManager = new TextureManager();
	myShaderManager = new ShaderManager();

	myObjectPLY = new ply("./data/cube.ply");
	myEnvironmentPLY = new ply("./data/sphere.ply");
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	delete myObjectPLY;
	delete myEnvironmentPLY;
}

void MyGLCanvas::initShaders() {
	myTextureManager->loadTexture("environMap", "./data/sphere-map-market.ppm");
	myTextureManager->loadTexture("objectTexture", "./data/brick.ppm");

	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object.vert", "shaders/330/object.frag");
	myObjectPLY->buildArrays();
	myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->buildArrays();
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);
}



void MyGLCanvas::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!valid()) {  //this is called when the GL canvas is set up for the first time or when it is resized...
		printf("establishing GL context\n");

		glViewport(0, 0, w(), h());
		updateCamera(w(), h());
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		/****************************************/
		/*          Enable z-buferring          */
		/****************************************/

		glEnable(GL_DEPTH_TEST);
		glPolygonOffset(1, 1);
		if (firstTime == true) {
			firstTime = false;
			initShaders();
		}
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene();
	glFlush();
}

void MyGLCanvas::drawScene() {
	// std::cout << "drawing" << std::endl;
	glm::mat4 viewMatrix = glm::lookAt(eyePosition, eyePosition + lookVec, upVec);
		// glm::lookAt(eyePosition, lookatPoint, glm::vec3(0.0f, 1.0f, 0.0f));

	// std::cout << "made view matrix" << std::endl;
	viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.x), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.y), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.z), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 modelMatrix = glm::mat4(1.0);
	modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.z), glm::vec3(0.0f, 0.0f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

	const float floor_l = 10.0f;
	const float floor_w = 20.0f;
	const float floor_h = 2.0f;
	glm::mat4 S_floor = glm::scale(glm::mat4(1.0f), glm::vec3(floor_w, floor_h, floor_l));
	glm::mat4 T_floor = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -floor_h / 2.0f, 0.0f));
	glm::mat4 M_floor = T_floor * S_floor;

	glm::vec4 lookVec(0.0f, 0.0f, -1.0f, 0.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	//Pass first texture info to our shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("environMap"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("objectTexture"));

	//first draw the object sphere
	const GLuint object_shader = myShaderManager->getShaderProgram("objectShaders")->programID;
	glUseProgram(object_shader);

	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(object_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(object_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	static constexpr float sphere_scale = 7.0f;
	glUniform1f(glGetUniformLocation(object_shader, "sphereScale"), sphere_scale);
	static constexpr float sphere_radius = 0.5f;
	glUniform1f(glGetUniformLocation(object_shader, "sphereRadius"), sphere_radius);
	glUniform1f(glGetUniformLocation(object_shader, "PI"), PI);

	glUniform1i(glGetUniformLocation(object_shader, "objectTextureMap"), 1);
	glUniform1i(glGetUniformLocation(object_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(object_shader, "textureBlend"), textureBlend);

	// Diffuse lighting.
	glUniform3fv(glGetUniformLocation(object_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(object_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// Center room.
	const int n = static_cast<int>(floor((eyePosition.z + floor_l / 2.0f) / floor_l));

	glUniformMatrix4fv(glGetUniformLocation(object_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, n * floor_l))* M_floor));
	myObjectPLY->renderVBO(object_shader);

	// -Z room.
	glUniformMatrix4fv(glGetUniformLocation(object_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n - 1) * floor_l)) * M_floor));
	myObjectPLY->renderVBO(object_shader);

	// +Z room.
	glUniformMatrix4fv(glGetUniformLocation(object_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n + 1) * floor_l)) * M_floor));
	myObjectPLY->renderVBO(object_shader);


	//second draw the enviroment sphere
	const GLuint environment_shader = myShaderManager->getShaderProgram("environmentShaders")->programID;
	glUseProgram(environment_shader);

	//TODO: add variable binding
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myModelMatrix"), 1, false, glm::value_ptr(modelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	glUniform1f(glGetUniformLocation(environment_shader, "sphereScale"), sphere_scale);
	glUniform1f(glGetUniformLocation(environment_shader, "sphereRadius"), sphere_radius);
	glUniform1f(glGetUniformLocation(environment_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(environment_shader, "environmentTextureMap"), 0);

	glUniform3fv(glGetUniformLocation(environment_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(environment_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// myEnvironmentPLY->renderVBO(environment_shader);
}


void MyGLCanvas::updateCamera(int width, int height) {
	float xy_aspect;
	xy_aspect = (float)width / (float)height;

	perspectiveMatrix = glm::perspective(TO_RADIANS(viewAngle), xy_aspect, clipNear, clipFar);
}


int MyGLCanvas::handle(int e) {
	//static int first = 1;
#ifndef __APPLE__
	if (firstTime && e == FL_SHOW && shown()) {
		firstTime = 0;
		make_current();
		GLenum err = glewInit(); // defines pters to functions of OpenGL V 1.2 and above
		if (GLEW_OK != err) {
			/* Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		}
		else {
			//SHADER: initialize the shader manager and loads the two shader programs
			initShaders();
		}
	}
#endif
	// printf("Event was %s (%d)\n", fl_eventnames[e], e);
	std::cout << "event " << e << std::endl;
	static int mouse_x;
	static int mouse_y;
	const float movement_speed = 1.0;
	const float look_speed_mouse = 0.5f;
	const float look_speed_key= 2.0f;
	static int center_x = x() + w() / 2;
    static int center_y = y() + h() / 2;
	switch (e) {
	case FL_DRAG:
	case FL_MOVE: {
		int cur_x = Fl::event_x();
		int cur_y = Fl::event_y();
		int dx = mouse_x - cur_x;
		int dy = mouse_y - cur_y;
		const glm::mat4 R_x = glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_mouse * dx), upVec);
		glm::mat4 R_y;
		if ((dy > 0 && glm::dot(upVec, lookVec) < 0.99f) || (dy < 0 && glm::dot(-upVec, lookVec) < 0.99f)) {
			R_y = glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_mouse * dy), glm::normalize(glm::cross(lookVec, upVec)));
		} else {
			R_y = glm::mat4(1.0f);
		}
		// const glm::mat4 R_y = glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_mouse * dy), glm::vec3(1.0f, 0.0f, 0.0f));

		lookVec = glm::normalize(glm::vec3(R_y * R_x * glm::vec4(lookVec, 1.0f)));

		mouse_x = cur_x;
		mouse_y = cur_y;

		glutWarpPointer(center_x, center_y);
		// glfwSetMousePos
		return 1;
	}
	case FL_PUSH:
	case FL_RELEASE:
	case FL_KEYUP:
	case FL_KEYDOWN: {
		const char c = Fl::event_key();
		switch (c) {
			case 'w': {
				std::cout << "w pressed" << std::endl;
				eyePosition += movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z));
				return 1;
			}
			case 'a': {
				std::cout << "w pressed" << std::endl;
				eyePosition += movement_speed * glm::normalize(glm::cross(upVec, lookVec));
				return 1;
			}
			case 's': {
				std::cout << "s pressed" << std::endl;
				eyePosition -= movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z)) ;
				return 1;
			}
			case 'd': {
				std::cout << "w pressed" << std::endl;
				eyePosition -= movement_speed * glm::normalize(glm::cross(upVec, lookVec));
				return 1;
			}
			case 'i': {
				std::cout << "up pressed" << std::endl;
				if (glm::dot(upVec, lookVec) < 0.99f) {
					glm::vec3 axis = glm::cross(lookVec, upVec);
					lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_key), axis) * glm::vec4(lookVec, 1.0f)));
				} else {
					std::cout << "too close!" << std::endl;
				}
				return 1;
			}
			case 'j': {
				std::cout << "j pressed" << std::endl;
				lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_key), upVec) * glm::vec4(lookVec, 1.0f)));
				return 1;
			}
			case 'k': {
				std::cout << "down pressed" << std::endl;
				if (glm::dot(-upVec, lookVec) < 0.99f) {
					glm::vec3 axis = glm::cross(lookVec, upVec);
					lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(-look_speed_key), axis) * glm::vec4(lookVec, 1.0f)));
				} else {
					std::cout << "too close!" << std::endl;
				}
				return 1;
			}
			case 'l': {
				std::cout << "l pressed" << std::endl;
				lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(-look_speed_key), upVec) * glm::vec4(lookVec, 1.0f)));
				return 1;
			}
			default: {
				break;
			}
		}
		break;
	}
	case FL_MOUSEWHEEL:
		break;
	}
	return Fl_Gl_Window::handle(e);
}

void MyGLCanvas::resize(int x, int y, int w, int h) {
	Fl_Gl_Window::resize(x, y, w, h);
	puts("resize called");
}

void MyGLCanvas::reloadShaders() {
	myShaderManager->resetShaders();

	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object.vert", "shaders/330/object.frag");
	myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	invalidate();
}

void MyGLCanvas::loadPLY(std::string filename) {
	delete myObjectPLY;
	myObjectPLY = new ply(filename);
	myObjectPLY->buildArrays();
	myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);
}

void MyGLCanvas::loadEnvironmentTexture(std::string filename) {
	std::cout << "MyGLCanvas::loadEnvironmentTexture" << std::endl;
	myTextureManager->deleteTexture("environMap");
	myTextureManager->loadTexture("environMap", filename);
}

void MyGLCanvas::loadObjectTexture(std::string filename) {
	myTextureManager->deleteTexture("objectTexture");
	myTextureManager->loadTexture("objectTexture", filename);
}