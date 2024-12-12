#include <array>
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
	eyePosition = glm::vec3(0.0f, 2.0f, 1.0f);
	lookatPoint = glm::vec3(0.0f, 2.0f, 2.0f);
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

	// Fog.
	fog_start = 10.0f;
	fog_end = 15.0f;

	// Lights.
	spot_light_angle_deg = 20.0f;
	spot_light_exponent = 1.0f;
	spot_light_lookat_mod = glm::vec3(0.0f, 0.0f, 0.0f);

	useDiffuse = false;

	firstTime = true;

	myTextureManager = new TextureManager();
	myShaderManager = new ShaderManager();

	// cube_ply = new ply("./data/cube.ply");
	this->cubes.reserve(CubePly::Max);
	cubes[CubePly::GalleryFloor] = new ply("./data/cube.ply");
	cubes[CubePly::GalleryWall] = new ply("./data/cube.ply");
	cubes[CubePly::Painting] = new ply("./data/cube.ply");
	cubes[CubePly::SideWallNE] = new ply("./data/cube.ply");

	myEnvironmentPLY = new ply("./data/sphere.ply");

	ArtManager* mem = (ArtManager*)mmap(
		NULL,
		sizeof(ArtManager),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1,
		0
	);
	art_init = false;

	this->art_manager = new (mem) ArtManager();
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	for (auto cube : this->cubes) {
		delete cube;
	}
	delete myEnvironmentPLY;
}

void MyGLCanvas::preprocess_shaders() {
	std::cout << "preprocessing shaders" << std::endl;
	pid_t pid = fork();
	if (pid < 0) {
		std::cout << "fork failed" << std::endl;
		exit(1);
	} else if (pid == 0) {
		const char* args[] = {"make", "shaders", nullptr};
    	execvp("make", (char* const*)args);
		std::cout << "execvp failed" << std::endl;
		exit(1);
	} else {
		wait(nullptr);
	}
}

void MyGLCanvas::initShaders() {
	myTextureManager->loadTexture("environMap", "./data/sphere-map-market.ppm");
	myTextureManager->loadTexture("gallery_floor_texture", "./data/gallery-floor.ppm");
	myTextureManager->loadTexture("gallery_floor_normal", "./data/gallery-floor-normal.ppm");
	myTextureManager->loadTexture("gallery_wall_texture", "./data/gallery-wall.ppm");
	myTextureManager->loadTexture("gallery_wall_normal", "./data/gallery-wall-normal.ppm");


	this->preprocess_shaders();

	myShaderManager->addShaderProgram("gallery_floor_shaders", "shaders/330/gallery-floor.vert", "shaders/330/gallery-floor.frag");
	cubes[CubePly::GalleryFloor]->buildArrays();
	cubes[CubePly::GalleryFloor]->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->buildArrays();
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	myShaderManager->addShaderProgram("gallery_wall_shaders", "shaders/330/gallery-wall.vert", "shaders/330/gallery-wall.frag");
	cubes[CubePly::GalleryWall]->buildArrays();
	cubes[CubePly::GalleryWall]->bindVBO(myShaderManager->getShaderProgram("gallery_wall_shaders")->programID);

	myShaderManager->addShaderProgram("painting_shaders", "shaders/330/painting.vert", "shaders/330/painting.frag");
	cubes[CubePly::Painting]->buildArrays();
	cubes[CubePly::Painting]->bindVBO(myShaderManager->getShaderProgram("painting_shaders")->programID);

	// myShaderManager->addShaderProgram("doorway_shaders", "shaders/330/doorway.vert", "shaders/330/doorway.frag");
	// cubes[CubePly::SideWallNE]->buildArrays();
	// cubes[CubePly::SideWallNE]->bindVBO(myShaderManager->getShaderProgram("doorway_shaders")->programID);
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
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// download and load initial images.
	if (!this->art_init) {
		std::cout << "binding intitial paintings" << std::endl;
		for (size_t ii = 0; ii < NUM_RENDERED_PAINTINGS; ++ii) {
			std::cout << "binding painting " << ii << std::endl;
#if DO_DOWNLOAD
			const std::string filename = "./data/new-image" + std::to_string(ii) + ".ppm";
			pid_t pid = fork();
			if (pid == -1) {
				std::cout << "fork failed" << std::endl;
				exit(1);
			} else if (pid == 0) {
				this->art_manager->download_and_convert(ii);
				exit(1);
			} else {
				wait(nullptr);
				this->art_manager->read_ppm(ii, filename);
				this->art_manager->bind(ii);
				std::remove(filename.c_str());
			}
#else
			const std::string filename = "./data/flowers.ppm";
			this->art_manager->read_ppm(ii, filename);
			this->art_manager->bind(ii);
#endif
		}
		std::cout << "done binding initial paintings" << std::endl;
		this->art_init = true;
	}

	drawScene();
	glFlush();
}

void MyGLCanvas::maybe_download_art() {
	for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
		if (!this->art_manager->is_downloading_image() && this->art_manager->should_download(ii)) {
			this->art_manager->set_downloading_image();
			this->art_manager->unset_should_download(ii);
			pid_t pid = fork();
			if (pid == -1) {
				std::cout << "fork failed" << std::endl;
				exit(1);
			} else if (pid == 0) {
				this->art_manager->download_and_convert(ii);
			}
		}
	}
}

void MyGLCanvas::maybe_load_art_ppm() {
	const std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs = this->art_manager->get_buffer_idxs();
	for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
		const size_t buffer_idx = buffer_idxs[ii];
		std::string filename = "./data/new-image" + std::to_string(ii) + ".ppm";
		std::ifstream file(filename);
		if (file.good() && !this->art_manager->is_loading_ppm()) {
			// TODO this might be a bad idea.
			this->art_manager->unset_downloading_image();
			this->art_manager->set_loading_ppm();
			this->art_manager->unbind(buffer_idx);
			pid_t pid = fork();
			if (pid < 0) {
				std::cout << "fork failed" << std::endl;
				exit(1);
			} else if (pid == 0) {
				this->art_manager->read_ppm(buffer_idx, filename);
				std::remove(filename.c_str());
				this->art_manager->unset_loading_ppm();
				exit(0);
			}
		}
		file.close();
	}

}

void MyGLCanvas::maybe_bind_art() {
	const std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs = this->art_manager->get_buffer_idxs();
	for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
		const size_t buffer_idx = buffer_idxs[ii];
		if (!this->art_manager->is_loading_ppm() && !this->art_manager->is_bound(buffer_idx)) {
			this->art_manager->bind(buffer_idx);
		}
	}
}


// FIXME return instead a list of indices.
// For 2 paintings per room:
// 0 -> { 0, 1 }
// 1 -> { 2, 3 }
// etc
size_t MyGLCanvas::get_buffer_idx_from_room_number(const int n) {
	// Step 1: translate room number to range (0, BUFFER_SIZE / PAINTINGS_PER_ROOM).
	//    this is the "block" we're in.
	// Step 2: start index is PAINTINGS_PER_ROOM * block.
	// Step 3: for ii in (start_index, start_index + PAINTINGS_PER_ROOM) ...
	//    ^ return that array
	const int buffer_size = this->art_manager->get_buffer_size();
	return static_cast<size_t>(((n % buffer_size) + buffer_size) % buffer_size);
}

// FIXME rename PAINTINGS_PER_ROOM
std::array<size_t, NUM_PAINTINGS_PER_ROOM> get_buffer_idxs_from_room_number(const int n) {
	const size_t start_block = static_cast<size_t>(((n % NUM_RENDERED_ROOMS) + NUM_RENDERED_ROOMS) % NUM_RENDERED_ROOMS);
	std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs;
	for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
		buffer_idxs[ii] = NUM_PAINTINGS_PER_ROOM * start_block + ii;
	}
	return buffer_idxs;
}

unsigned int get_gl_texture_id(const size_t num) {
	switch (num) {
		case 0: return GL_TEXTURE0;
		case 1: return GL_TEXTURE1;
		case 2: return GL_TEXTURE2;
		case 3: return GL_TEXTURE3;
		case 4: return GL_TEXTURE4;
		case 5: return GL_TEXTURE5;
		case 6: return GL_TEXTURE6;
		case 7: return GL_TEXTURE7;
		case 8: return GL_TEXTURE8;
		case 9: return GL_TEXTURE9;
		case 10: return GL_TEXTURE10;
		case 11: return GL_TEXTURE11;
		case 12: return GL_TEXTURE12;
		case 13: return GL_TEXTURE13;
		case 14: return GL_TEXTURE14;
		case 15: return GL_TEXTURE15;
		case 16: return GL_TEXTURE16;
		case 17: return GL_TEXTURE17;
		case 18: return GL_TEXTURE18;
		case 19: return GL_TEXTURE19;
		case 20: return GL_TEXTURE20;
		case 21: return GL_TEXTURE21;
		case 22: return GL_TEXTURE22;
		case 23: return GL_TEXTURE23;
		case 24: return GL_TEXTURE24;
		case 25: return GL_TEXTURE25;
		case 26: return GL_TEXTURE26;
		case 27: return GL_TEXTURE27;
		case 28: return GL_TEXTURE28;
		case 29: return GL_TEXTURE29;

		default: { std::cout << "texture num too big!" << std::endl; exit(1); }
	}
}

void MyGLCanvas::drawScene() {
	// glutWarpPointer(this->w() / 2, this->h() / 2);
	// glfwSetCursorPos(this, this->w() / 2, this->h() / 2);
	glEnable(GL_DEPTH_TEST);
#if DO_DOWNLOAD
	this->maybe_download_art();
	// std::cout << "drawScene - maybe_bind_art" << std::endl;
	this->maybe_bind_art();
	// std::cout << "drawScene - maybe_load_art_ppm" << std::endl;
	this->maybe_load_art_ppm();
#endif
	// Do we have a new painting to load in?
	// std::cout << "drawing" << std::endl;
	glm::mat4 viewMatrix = glm::lookAt(eyePosition, eyePosition + lookVec, upVec);
	glm::mat4 M_environment = glm::translate(glm::mat4(1.0f), eyePosition);
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
	const float wall_w = 0.3 * floor_w;
	const float wall_h = wall_w * 2.0;
	glm::mat4 S_wall = glm::scale(glm::mat4(1.0f), glm::vec3(wall_w, wall_h, wall_l));
	glm::mat4 T_wall = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			0.0f,
			wall_h / 2.0f,
			0.0f
		)
	);
	glm::mat4 M_wall = T_wall * S_wall;

	const float doorway_w = 0.2 * floor_l;
	const float sidewall_w = (floor_l - doorway_w) / 2.0;
	const float sidewall_l = wall_l;
	const float sidewall_h = wall_h;
	glm::mat4 S_sidewall = glm::scale(glm::mat4(1.0f), glm::vec3(sidewall_w, sidewall_h, sidewall_l));
	glm::mat4 R_sidewall = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 T_sidewallNE = glm::translate(glm::mat4(1.0f), glm::vec3(-(floor_w - sidewall_l) / 2.0f, sidewall_h / 2.0f, -(floor_l - sidewall_w) / 2.0f));
	glm::mat4 T_sidewallSE = glm::translate(glm::mat4(1.0f), glm::vec3(-(floor_w - sidewall_l) / 2.0f, sidewall_h / 2.0f, (floor_l - sidewall_w) / 2.0f));
	glm::mat4 T_sidewallNW = glm::translate(glm::mat4(1.0f), glm::vec3((floor_w - sidewall_l) / 2.0f, sidewall_h / 2.0f, -(floor_l - sidewall_w) / 2.0f));
	glm::mat4 T_sidewallSW = glm::translate(glm::mat4(1.0f), glm::vec3((floor_w - sidewall_l) / 2.0f, sidewall_h / 2.0f, (floor_l - sidewall_w) / 2.0f));
	glm::mat4 M_sidewallNE = T_sidewallNE * R_sidewall * S_sidewall;
	glm::mat4 M_sidewallSE = T_sidewallSE * R_sidewall * S_sidewall;
	glm::mat4 M_sidewallNW = T_sidewallNW * R_sidewall * S_sidewall;
	glm::mat4 M_sidewallSW = T_sidewallSW * R_sidewall * S_sidewall;

	const float ceiling_l = floor_l;
	const float ceiling_w = floor_w;
	const float ceiling_h = wall_l;
	glm::mat4 S_ceiling = glm::scale(glm::mat4(1.0f), glm::vec3(ceiling_w, ceiling_h, ceiling_l));
	glm::mat4 T_ceiling = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, wall_h + ceiling_h / 2.0f, 0.0f));
	glm::mat4 M_ceiling = T_ceiling * S_ceiling;

	const int current_room_number = this->get_room_number(this->eyePosition);
	const float painting_l = 0.1f;
	const float painting_w = 0.5 * wall_w;
	std::array<glm::mat4, NUM_RENDERED_PAINTINGS> M_painting;
	for (size_t ii = 0; ii < NUM_RENDERED_ROOMS; ++ii) {
		const int room_number = current_room_number - NUM_ROOMS_AHEAD_TO_RENDER + static_cast<int>(ii);
		const std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs = get_buffer_idxs_from_room_number(room_number);
		for (size_t jj = 0; jj < NUM_PAINTINGS_PER_ROOM; ++jj) {
			const size_t buffer_idx = buffer_idxs[jj];
			const float painting_h = painting_w / this->art_manager->get_aspect_ratio(buffer_idx);
			const glm::mat4 S_painting = glm::scale(glm::mat4(1.0f), glm::vec3(painting_w, painting_h, painting_l));
			glm::mat4 T_painting;
			if (jj == 0) {
				T_painting = glm::translate(T_wall,glm::vec3(0.0f, -(wall_h - painting_h) / 2.0f + eyePosition.y - painting_h / 2.0f, -(painting_l + wall_l) / 2.0f));
			} else if (jj == 1) {
				T_painting = glm::translate(T_wall,glm::vec3(0.0f, -(wall_h - painting_h) / 2.0f + eyePosition.y - painting_h / 2.0f, (painting_l + wall_l) / 2.0f));
			}
			const size_t painting_idx = NUM_PAINTINGS_PER_ROOM * ii + jj;
			M_painting[painting_idx] = T_painting * S_painting;
		}
	}

	// BIND TEXTURES ===========================================================
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("environMap"));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_wall_texture"));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_normal"));
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_wall_normal"));
	const size_t painting_texture_offset = 5;
	const unsigned int gl_texture_start = GL_TEXTURE5;
	for (size_t ii = 0; ii < NUM_RENDERED_ROOMS; ++ii) {
		const int room_number = current_room_number - NUM_ROOMS_AHEAD_TO_RENDER + static_cast<int>(ii);
		const std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs = get_buffer_idxs_from_room_number(room_number);
		for (size_t jj = 0; jj < NUM_PAINTINGS_PER_ROOM; ++jj) {
			const size_t buffer_idx = buffer_idxs[jj];
			const size_t painting_idx = NUM_PAINTINGS_PER_ROOM * ii + jj;
			const size_t texture_num = painting_texture_offset + painting_idx;
			glActiveTexture(get_gl_texture_id(texture_num));
			if (this->art_manager->is_bound(buffer_idx)) {
				glBindTexture(GL_TEXTURE_2D, this->art_manager->get_texture_id(buffer_idx));
			} else {
				glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("gallery_floor_texture"));
			}
		}
	}

	// LIGHTS ==================================================================
	constexpr unsigned int num_diffuse_lights = 2;
	glm::vec3 p_diffuse_light = glm::vec3(0.0f, 3.0f, 0.0f);
	glm::mat4 M_diffuse_light1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, floor_l / 4.0f));
	glm::mat4 M_diffuse_light2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -floor_l / 4.0f));

	constexpr unsigned int num_spot_lights = 2;
	glm::vec3 p_spot_light = glm::vec3(0.0f, 3.0f, 0.0f);
	glm::mat4 M_spot_light1 = M_diffuse_light1;
	glm::mat4 M_spot_light2 = M_diffuse_light2;
	glm::vec3 p_lookat_spot_light1 = glm::vec3(spot_light_lookat_mod.x, eyePosition.y + spot_light_lookat_mod.y, wall_l / 2.0f + painting_l);
	glm::vec3 p_lookat_spot_light2 = glm::vec3(spot_light_lookat_mod.x, eyePosition.y + spot_light_lookat_mod.y, -(wall_l / 2.0f + painting_l));
	glm::vec3 v_spot_lights[] = {
		glm::normalize(p_lookat_spot_light1 - glm::vec3(M_spot_light1 * glm::vec4(p_spot_light, 1.0f))),
		glm::normalize(p_lookat_spot_light2 - glm::vec3(M_spot_light2 * glm::vec4(p_spot_light, 1.0f)))
	};

	float th_spot_light = glm::radians(spot_light_angle_deg);

	// ENVIRONMENT ==============================================================
	const GLuint environment_shader = myShaderManager->getShaderProgram("environmentShaders")->programID;
	glUseProgram(environment_shader);
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myModelMatrix"), 1, false, glm::value_ptr(modelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));
	glUniformMatrix4fv(glGetUniformLocation(environment_shader, "M"), 1, false, glm::value_ptr(M_environment));
	// glUniform1f(glGetUniformLocation(environment_shader, "sphereScale"), 7.0);
	// glUniform1f(glGetUniformLocation(environment_shader, "sphereRadius"), 0.5);
	glUniform1f(glGetUniformLocation(environment_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(environment_shader, "environmentTextureMap"), 0);
	// Fog.
	glUniform1f(glGetUniformLocation(environment_shader, "fog_start"), fog_start);
	glUniform1f(glGetUniformLocation(environment_shader, "fog_end"), fog_end);
	glUniform3fv(glGetUniformLocation(environment_shader, "lightPos"), 1,  glm::value_ptr(lightPos));
	glUniform1i(glGetUniformLocation(environment_shader, "useDiffuse"), useDiffuse ? 1 : 0);
	myEnvironmentPLY->renderVBO(environment_shader);

	// GALLERY FLOOR ===========================================================
	const GLuint gallery_floor_shader = myShaderManager->getShaderProgram("gallery_floor_shaders")->programID;
	glUseProgram(gallery_floor_shader);
	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));
	glUniform1f(glGetUniformLocation(gallery_floor_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "texture_map"), 1);
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "normal_map"), 3);
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(gallery_floor_shader, "textureBlend"), textureBlend);
	// Diffuse lighting.
	glUniform1i(glGetUniformLocation(gallery_floor_shader, "useDiffuse"), useDiffuse ? 1 : 0);
	// Fog.
	glUniform1f(glGetUniformLocation(gallery_floor_shader, "fog_start"), fog_start);
	glUniform1f(glGetUniformLocation(gallery_floor_shader, "fog_end"), fog_end);

	glUniform1ui(glGetUniformLocation(gallery_floor_shader, "num_spot_lights"), num_spot_lights);
	glUniform3fv(glGetUniformLocation(gallery_floor_shader, "v_spot_lights"), num_spot_lights,  glm::value_ptr(v_spot_lights[0]));

	glUniform1ui(glGetUniformLocation(gallery_floor_shader, "num_diffuse_lights"), num_diffuse_lights);
	for (size_t ii = 0; ii < NUM_RENDERED_ROOMS; ++ii) {
		const int room_number = current_room_number - NUM_ROOMS_AHEAD_TO_RENDER + static_cast<int>(ii);
		glm::vec3 diffuse_lights[num_diffuse_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light1 * glm::vec4(p_diffuse_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light2 * glm::vec4(p_diffuse_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(gallery_floor_shader, "diffuse_lights"), num_diffuse_lights,  glm::value_ptr(diffuse_lights[0]));
		glm::vec3 p_spot_lights[num_spot_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light1 * glm::vec4(p_spot_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light2 * glm::vec4(p_spot_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(gallery_floor_shader, "p_spot_lights"), num_spot_lights,  glm::value_ptr(p_spot_lights[0]));
		glUniformMatrix4fv(glGetUniformLocation(gallery_floor_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_floor));
		cubes[CubePly::GalleryFloor]->renderVBO(gallery_floor_shader);
	}

	// WALL ====================================================================
	const GLuint gallery_wall_shader = myShaderManager->getShaderProgram("gallery_wall_shaders")->programID;
	glUseProgram(gallery_wall_shader);
	// Vertex stuff.
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myViewMatrix"), 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myPerspectiveMatrix"), 1, false, glm::value_ptr(perspectiveMatrix));
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "PI"), PI);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "texture_map"), 2);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "normal_map"), 4);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "environmentTextureMap"), 0);
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "textureBlend"), textureBlend);
	// Diffuse lighting.
	// glUniform3fv(glGetUniformLocation(gallery_wall_shader, "lightPos"), 1,  glm::value_ptr(test_light_pos));
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "useDiffuse"), useDiffuse ? 1 : 0);
	glUniform1i(glGetUniformLocation(gallery_wall_shader, "transparent"), false);
	// Fog.
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "fog_start"), fog_start);
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "fog_end"), fog_end);

	glUniform1ui(glGetUniformLocation(gallery_wall_shader, "num_diffuse_lights"), num_diffuse_lights);

	glUniform1ui(glGetUniformLocation(gallery_wall_shader, "num_spot_lights"), num_spot_lights);
	glUniform3fv(glGetUniformLocation(gallery_wall_shader, "v_spot_lights"), num_spot_lights,  glm::value_ptr(v_spot_lights[0]));
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "th_spot_light"), th_spot_light);
	glUniform1f(glGetUniformLocation(gallery_wall_shader, "e_spot_light"), spot_light_exponent);

	for (size_t ii = 0; ii < NUM_RENDERED_ROOMS; ++ii) {
		const int room_number = current_room_number - NUM_ROOMS_AHEAD_TO_RENDER + static_cast<int>(ii);
		glm::vec3 diffuse_lights[num_diffuse_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light1 * glm::vec4(p_diffuse_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light2* glm::vec4(p_diffuse_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(gallery_wall_shader, "diffuse_lights"), num_diffuse_lights, glm::value_ptr(diffuse_lights[0]));
		glm::vec3 p_spot_lights[num_spot_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light1 * glm::vec4(p_spot_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light2 * glm::vec4(p_spot_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(gallery_wall_shader, "p_spot_lights"), num_spot_lights,  glm::value_ptr(p_spot_lights[0]));
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_wall));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_sidewallNE));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_sidewallSE));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_sidewallNW));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_sidewallSW));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
		glUniformMatrix4fv(glGetUniformLocation(gallery_wall_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_ceiling));
		cubes[CubePly::GalleryWall]->renderVBO(gallery_wall_shader);
	}

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
	glUniform1i(glGetUniformLocation(painting_shader, "useDiffuse"), useDiffuse ? 1 : 0);
	// Fog.
	glUniform1f(glGetUniformLocation(painting_shader, "fog_start"), fog_start);
	glUniform1f(glGetUniformLocation(painting_shader, "fog_end"), fog_end);
	glUniform1ui(glGetUniformLocation(painting_shader, "num_diffuse_lights"), num_diffuse_lights);

	glUniform1ui(glGetUniformLocation(painting_shader, "num_spot_lights"), num_spot_lights);
	glUniform3fv(glGetUniformLocation(painting_shader, "v_spot_lights"), num_spot_lights,  glm::value_ptr(v_spot_lights[0]));
	glUniform1f(glGetUniformLocation(painting_shader, "th_spot_light"), th_spot_light);
	glUniform1f(glGetUniformLocation(painting_shader, "e_spot_light"), spot_light_exponent);

	for (size_t ii = 0; ii < NUM_RENDERED_ROOMS; ++ii) {
		const int room_number = current_room_number - NUM_ROOMS_AHEAD_TO_RENDER + static_cast<int>(ii);
		glm::vec3 diffuse_lights[num_diffuse_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light1 * glm::vec4(p_diffuse_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_diffuse_light2 * glm::vec4(p_diffuse_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(painting_shader, "diffuse_lights"), num_diffuse_lights, glm::value_ptr(diffuse_lights[0]));
		glm::vec3 p_spot_lights[num_spot_lights] = {
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light1 * glm::vec4(p_spot_light, 1.0f)),
			glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_spot_light2 * glm::vec4(p_spot_light, 1.0f)),
		};
		glUniform3fv(glGetUniformLocation(painting_shader, "p_spot_lights"), num_spot_lights,  glm::value_ptr(p_spot_lights[0]));
		glUniform1i(glGetUniformLocation(painting_shader, "room"), room_number);
		for (size_t jj = 0; jj < NUM_PAINTINGS_PER_ROOM; ++jj) {
			const size_t painting_idx = NUM_PAINTINGS_PER_ROOM * ii + jj;
			glUniform1i(glGetUniformLocation(painting_shader, "texture_map"), static_cast<int>(painting_texture_offset + painting_idx));
			glUniformMatrix4fv(glGetUniformLocation(painting_shader, "myModelMatrix"), 1, false, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, room_number * floor_l)) * M_painting[painting_idx]));
			cubes[CubePly::Painting]->renderVBO(painting_shader);
		}
	}
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
		const int room_number_to_buffer = new_room_number + (
			new_room_number > old_room_number
			? NUM_ROOMS_AHEAD_TO_RENDER
			: -NUM_ROOMS_AHEAD_TO_RENDER
		);
		const std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs = get_buffer_idxs_from_room_number(room_number_to_buffer);
		this->art_manager->set_buffer_idxs(buffer_idxs);
		for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
			this->art_manager->set_should_download(ii);
		}
		std::cout << "loading to buffer idxs ";
		for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
			std::cout << buffer_idxs[ii] << ",";
		}
		std::cout << std::endl;
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
	const float look_speed_mouse = 1.0f;
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
	this->preprocess_shaders();

	myShaderManager->addShaderProgram("gallery_floor_shaders", "shaders/330/gallery-floor.vert", "shaders/330/gallery-floor.frag");
	cubes[CubePly::GalleryFloor]->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment.vert", "shaders/330/environment.frag");
	myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	myShaderManager->addShaderProgram("gallery_wall_shaders", "shaders/330/gallery-wall.vert", "shaders/330/gallery-wall.frag");
	cubes[CubePly::GalleryWall]->bindVBO(myShaderManager->getShaderProgram("gallery_wall_shaders")->programID);

	myShaderManager->addShaderProgram("painting_shaders", "shaders/330/painting.vert", "shaders/330/painting.frag");
	cubes[CubePly::Painting]->bindVBO(myShaderManager->getShaderProgram("painting_shaders")->programID);

	// myShaderManager->addShaderProgram("doorway_shaders", "shaders/330/doorway.vert", "shaders/330/doorway.frag");
	// cubes[CubePly::SideWallNE]->bindVBO(myShaderManager->getShaderProgram("doorway_shaders")->programID);

	invalidate();
}

void MyGLCanvas::loadPLY(std::string filename) {
	// for (auto cube : this->cubes) {
	// 	delete cube;
	// }
	// cube_ply = new ply(filename);
	// cube_ply->buildArrays();
	// cube_ply->bindVBO(myShaderManager->getShaderProgram("gallery_floor_shaders")->programID);
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