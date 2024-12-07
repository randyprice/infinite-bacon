#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <Fl/Fl.H>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "MyGLCanvas.h"


MyGLCanvas::MyGLCanvas(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x, y, w, h, l) {
	mode(FL_OPENGL3 | FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);

	// eyePosition = glm::vec3(0.0f, 0.0f, 3.0f);
	// lookatPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	eyePosition = glm::vec3(0.0f, 2.0f, 0.0f);
	lookatPoint = glm::vec3(0.0f, 2.0f, 1.0f);
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

	// // TODO everything in TextureManager needs to come out to be allocated
	// // as shared memory.
	// TextureManager* tmem = (TextureManager*)mmap(
	// 	NULL,
	// 	sizeof(TextureManager),
	// 	PROT_READ | PROT_WRITE,
	// 	MAP_SHARED | MAP_ANONYMOUS,
	// 	-1,
	// 	0
	// );
	// myTextureManager = new (tmem) TextureManager();

	myTextureManager = new TextureManager();
	myShaderManager = new ShaderManager();

	cube_ply = new ply("./data/cube.ply");
	myEnvironmentPLY = new ply("./data/sphere.ply");

	ArtManager* mem = (ArtManager*)mmap(
		NULL,
		sizeof(ArtManager),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1,
		0
	);

	this->art_manager = new (mem) ArtManager();
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	delete cube_ply;
	delete myEnvironmentPLY;
}

void MyGLCanvas::initShaders() {
	myTextureManager->loadTexture("environMap", "./data/sphere-map-market.ppm");
	myTextureManager->loadTexture("gallery_floor_texture", "./data/gallery-floor.ppm");
	myTextureManager->loadTexture("gallery_wall_texture", "./data/gallery-wall.ppm");

	myShaderManager->addShaderProgram("gallery_floor_shaders", "shaders/330/gallery-floor.vert", "shaders/330/gallery-floor.frag");
	cube_ply->buildArrays();
	cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->buildArrays();
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	myShaderManager->addShaderProgram("gallery_wall_shaders", "shaders/330/gallery-wall.vert", "shaders/330/gallery-wall.frag");
	cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_wall_shaders")->programID);

	myShaderManager->addShaderProgram("painting_shaders", "shaders/330/painting.vert", "shaders/330/painting.frag");
	cube_ply->bindVBO(myShaderManager->getShaderProgram("painting_shaders")->programID);
}

constexpr float ROOM_L = 10.0f;

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

		// download and load initial images.
		const std::string filename = "./data/new-image.ppm";
		std::cout << "binding intitial paintings" << std::endl;
		for (size_t ii = 0; ii < BUFFER_SIZE; ++ii) {
			pid_t pid = fork();
			if (pid == -1) {
				std::cout << "fork failed" << std::endl;
				exit(1);
			} else if (pid == 0) {
				this->art_manager->download_and_convert();
				exit(0);
			} else {
				wait(nullptr);
			}
			// this->art_manager->download_and_convert();
			this->art_manager->read_ppm(ii, filename);
			this->art_manager->bind(ii);
			std::remove(filename.c_str());
		}
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene();
	glFlush();
}

void MyGLCanvas::maybe_load_art_ppm() {
	std::string filename = "./data/new-image.ppm";
	std::ifstream file(filename);
	if (file.good() && !this->art_manager->is_loading()) {
		this->art_manager->set_loading();
		const size_t buffer_idx = this->art_manager->get_buffer_idx();
		this->art_manager->unbind(buffer_idx);
		pid_t pid = fork();
		if (pid < 0) {
			std::cout << "fork failed" << std::endl;
			exit(1);
		} else if (pid == 0) {
			this->art_manager->read_ppm(buffer_idx, filename);
			std::remove(filename.c_str());
			this->art_manager->unset_loading();
			exit(0);
		}
	}

	file.close();
}

void MyGLCanvas::maybe_bind_art() {
	// TODO this can be less restrictive - don't bind art on the active buffer idx.
	if (this->art_manager->is_loading()) {
		return;
	}
	for (size_t ii = 0; ii < BUFFER_SIZE; ++ii) {
		if (!this->art_manager->is_bound(ii)) {
			this->art_manager->bind(ii);
		}
	}
}

size_t MyGLCanvas::get_buffer_idx_from_room_number(const int n) {
	const int buffer_size = this->art_manager->get_buffer_size();
	return static_cast<size_t>(((n % buffer_size) + buffer_size) % buffer_size);
}

void MyGLCanvas::drawScene() {
	this->maybe_bind_art();
	this->maybe_load_art_ppm();
	// Do we have a new painting to load in?
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

	const float floor_l = ROOM_L;
	const float floor_w = 10.0f;
	const float floor_h = 2.0f;
	glm::mat4 S_floor = glm::scale(glm::mat4(1.0f), glm::vec3(floor_w, floor_h, floor_l));
	glm::mat4 T_floor = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -floor_h / 2.0f, 0.0f));
	glm::mat4 M_floor = T_floor * S_floor;

	const float wall_l = 0.5f;
	const float wall_w = 0.6 * floor_w;
	const float wall_h = 10.0f;
	glm::mat4 S_wall = glm::scale(glm::mat4(1.0f), glm::vec3(wall_w, wall_h, wall_l));
	glm::mat4 T_wall = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			(floor_w - wall_w) / 2.0f,
			wall_h / 2.0f,
			(floor_l - wall_l) / 2.0f
		)
	);
	glm::mat4 M_wall = T_wall * S_wall;

	// TODO fix aspect ratio.
	const float painting_l = 0.1f;
	const float painting_w = 0.5 * wall_w;
	const float painting_h = painting_w;
	glm::mat4 S_painting = glm::scale(glm::mat4(1.0f), glm::vec3(painting_w, painting_h, painting_l));
	glm::mat4 T_painting = glm::translate(
		T_wall,
		glm::vec3(
			// (wall_w - painting_w) / 2.0f,
			0.0f,
			-(wall_h - painting_h) / 2.0f + 0.5f,
			-(painting_l + wall_l) / 2.0f
		)
	);
	glm::mat4 M_painting = T_painting * S_painting;

	glm::vec4 lookVec(0.0f, 0.0f, -1.0f, 0.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	//Pass first texture info to our shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("environMap"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_wall_texture"));

	const int n = this->get_room_number(this->eyePosition);
	const int painting_texture_offset = 3;

	// Bind painting textures.
	size_t buffer_idx = get_buffer_idx_from_room_number(n);
	glActiveTexture(GL_TEXTURE3);
	if (this->art_manager->is_bound(buffer_idx)) {
		// std::cout << "binding an art" << std::endl;
		glBindTexture(GL_TEXTURE_2D, this->art_manager->get_texture_id(buffer_idx));
	} else {
		glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
	}

	buffer_idx = get_buffer_idx_from_room_number(n - 1);
	glActiveTexture(GL_TEXTURE4);
	if (this->art_manager->is_bound(buffer_idx)) {
		// std::cout << "binding an art" << std::endl;
		glBindTexture(GL_TEXTURE_2D, this->art_manager->get_texture_id(buffer_idx));
	} else {
		glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
	}

	buffer_idx = get_buffer_idx_from_room_number(n + 1);
	glActiveTexture(GL_TEXTURE5);
	if (this->art_manager->is_bound(buffer_idx)) {
		// std::cout << "binding an art" << std::endl;
		glBindTexture(GL_TEXTURE_2D, this->art_manager->get_texture_id(buffer_idx));
	} else {
		glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
	}
	// std::cout << "gallery wall texture id " << myTextureManager->getTextureID("gallery_wall_texture") << std::endl;

	//first draw the object sphere
	const GLuint gallery_floor_shader = myShaderManager->getShaderProgram("gallery_floor_shaders")->programID;
	glUseProgram(gallery_floor_shader);

	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	glUniform1f(glGetUniformLocation(gallery_floor_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "texture_map"), 1);
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(gallery_floor_shader, "textureBlend"), textureBlend);

	// Diffuse lighting.
	glUniform3fv(glGetUniformLocation(gallery_floor_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// Center room.


	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, n * floor_l))* M_floor));
	cube_ply->renderVBO(gallery_floor_shader);

	// -Z room.
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n - 1) * floor_l)) * M_floor));
	cube_ply->renderVBO(gallery_floor_shader);

	// +Z room.
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n + 1) * floor_l)) * M_floor));
	cube_ply->renderVBO(gallery_floor_shader);

	// WALL ====================================================================
	const GLuint gallery_wall_shader = myShaderManager->getShaderProgram("gallery_wall_shaders")->programID;
	glUseProgram(gallery_wall_shader);

	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	glUniform1f(glGetUniformLocation(gallery_wall_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "texture_map"), 2);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "textureBlend"), textureBlend);

	// Diffuse lighting.
	glUniform3fv(glGetUniformLocation(gallery_wall_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// Center room.
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, n * floor_l))* M_wall));
	cube_ply->renderVBO(gallery_wall_shader);

	// -Z room.
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n - 1) * floor_l)) * M_wall));
	cube_ply->renderVBO(gallery_wall_shader);

	// +Z room.
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n + 1) * floor_l)) * M_wall));
	cube_ply->renderVBO(gallery_wall_shader);

	// PAINTING ====================================================================
	const GLuint painting_shader = myShaderManager->getShaderProgram("painting_shaders")->programID;
	glUseProgram(painting_shader);

	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	glUniform1f(glGetUniformLocation(painting_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(painting_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(painting_shader, "textureBlend"), textureBlend);

	// Diffuse lighting.
	glUniform3fv(glGetUniformLocation(painting_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(painting_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// Center room.
	glUniform1i(glGetUniformLocation(painting_shader, "room"), n);
	glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, n * floor_l))* M_painting));
	glUniform1i(glGetUniformLocation(painting_shader, "texture_map"), painting_texture_offset);
	cube_ply->renderVBO(painting_shader);

	// -Z room.
	glUniform1i(glGetUniformLocation(painting_shader, "room"), n - 1);
	glUniform1i(glGetUniformLocation(painting_shader, "texture_map"), painting_texture_offset + 1);
	glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n - 1) * floor_l)) * M_painting));
	cube_ply->renderVBO(painting_shader);

	// +Z room.
	glUniform1i(glGetUniformLocation(painting_shader, "room"), n + 1);
	glUniform1i(glGetUniformLocation(painting_shader, "texture_map"), painting_texture_offset + 2);
	glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (n + 1) * floor_l)) * M_painting));
	cube_ply->renderVBO(painting_shader);


	// //second draw the enviroment sphere
	// const GLuint environment_shader = myShaderManager->getShaderProgram("environmentShaders")->programID;
	// glUseProgram(environment_shader);

	// //TODO: add variable binding
	// glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myModelMatrix"), 1, false, glm::value_ptr(modelMatrix));
	// glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	// glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));

	// glUniform1f(glGetUniformLocation(environment_shader, "sphereScale"), sphere_scale);
	// glUniform1f(glGetUniformLocation(environment_shader, "sphereRadius"), sphere_radius);
	// glUniform1f(glGetUniformLocation(environment_shader, "PI"), PI);
	// glUniform1i(glGetUniformLocation(environment_shader, "environmentTextureMap"), 0);

	// glUniform3fv(glGetUniformLocation(environment_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	// glUniform1i(glGetUniformLocation(environment_shader, "useDiffuse"), useDiffuse ? 1 : 0);

	// myEnvironmentPLY->renderVBO(environment_shader);
}


void MyGLCanvas::updateCamera(int width, int height) {
	float xy_aspect;
	xy_aspect = (float)width / (float)height;

	perspectiveMatrix = glm::perspective(TO_RADIANS(viewAngle), xy_aspect, clipNear, clipFar);
}

int MyGLCanvas::get_room_number(const glm::vec3& e) {
	return static_cast<int>(floor((e.z + ROOM_L / 2.0f) / ROOM_L));
}

void MyGLCanvas::set_eye_position(const glm::vec3& e) {
	const int old_room_number = this->get_room_number(this->eyePosition);
	const int new_room_number = this->get_room_number(e);
	if (new_room_number != old_room_number) {
		std::cout << "room change: " << old_room_number << " -> " << new_room_number << std::endl;
		const int buffer_size = static_cast<int>(this->art_manager->get_buffer_size());
		const int room_number_to_buffer = (
			new_room_number > old_room_number
			? new_room_number + 2
			: new_room_number - 2
		);
		const int buffer_idx = ((room_number_to_buffer % buffer_size) + buffer_size) % buffer_size;
		std::cout << "loading to buffer " << buffer_idx << std::endl;
		this->art_manager->set_buffer_idx(buffer_idx);

		pid_t pid = fork();
		if (pid == -1) {
			std::cout << "fork failed" << std::endl;
		} else if (pid == 0) {
			this->art_manager->download_and_convert();
			std::cout << "execvp failed somewhere" << std::endl;
			exit(1);
		}
	}

	this->eyePosition = e;
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
	// std::cout << "event " << e << std::endl;
	static int mouse_x;
	static int mouse_y;
	const float movement_speed = 0.2;
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
				this->set_eye_position(eyePosition + movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z)));
				// eyePosition += movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z));
				return 1;
			}
			case 'a': {
				this->set_eye_position(eyePosition + movement_speed * glm::normalize(glm::cross(upVec, lookVec)));
				// eyePosition += movement_speed * glm::normalize(glm::cross(upVec, lookVec));
				return 1;
			}
			case 's': {
				this->set_eye_position(eyePosition - movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z)));
				// eyePosition -= movement_speed * glm::normalize(glm::vec3(lookVec.x, 0.0f, lookVec.z)) ;
				return 1;
			}
			case 'd': {
				this->set_eye_position(eyePosition - movement_speed * glm::normalize(glm::cross(upVec, lookVec)));
				// eyePosition -= movement_speed * glm::normalize(glm::cross(upVec, lookVec));
				return 1;
			}
			case 'i': {
				if (glm::dot(upVec, lookVec) < 0.99f) {
					glm::vec3 axis = glm::cross(lookVec, upVec);
					lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_key), axis) * glm::vec4(lookVec, 1.0f)));
				} else {
					std::cout << "too close!" << std::endl;
				}
				return 1;
			}
			case 'j': {
				lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(look_speed_key), upVec) * glm::vec4(lookVec, 1.0f)));
				return 1;
			}
			case 'k': {
				if (glm::dot(-upVec, lookVec) < 0.99f) {
					glm::vec3 axis = glm::cross(lookVec, upVec);
					lookVec = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(-look_speed_key), axis) * glm::vec4(lookVec, 1.0f)));
				} else {
					std::cout << "too close!" << std::endl;
				}
				return 1;
			}
			case 'l': {
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

	myShaderManager->addShaderProgram("gallery_floor_shaders", "shaders/330/gallery-floor.vert", "shaders/330/gallery-floor.frag");
	cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	myShaderManager->addShaderProgram("gallery_wall_shaders", "shaders/330/gallery-wall.vert", "shaders/330/gallery-wall.frag");
	cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_wall_shaders")->programID);

	myShaderManager->addShaderProgram("painting_shaders", "shaders/330/painting.vert", "shaders/330/painting.frag");
	cube_ply->bindVBO(myShaderManager->getShaderProgram("painting_shaders")->programID);

	invalidate();
}

void MyGLCanvas::loadPLY(std::string filename) {
	delete cube_ply;
	cube_ply = new ply(filename);
	cube_ply->buildArrays();
	cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);
}

void MyGLCanvas::loadEnvironmentTexture(std::string filename) {
	std::cout << "MyGLCanvas::loadEnvironmentTexture" << std::endl;
	myTextureManager->deleteTexture("environMap");
	myTextureManager->loadTexture("environMap", filename);
}

void MyGLCanvas::loadObjectTexture(std::string filename) {
	myTextureManager->deleteTexture("gallery_floor_texture");
	myTextureManager->loadTexture("gallery_floor_texture", filename);
}