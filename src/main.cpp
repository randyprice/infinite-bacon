/*  =================== File Information =================
	File Name: main.cpp
	Description:
	Author: Michael Shah

	Purpose: Driver for 3D program to load .ply models
	Usage:
	===================================================== */

#include <string>
#include <iostream>
#include <fstream>
#include <math.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>

#include "MyGLCanvas.h"

using namespace std;

class MyAppWindow;
MyAppWindow* win;

class MyAppWindow : public Fl_Window {
public:
	// Fl_Slider* rotXSlider;
	// Fl_Slider* rotYSlider;
	// Fl_Slider* rotZSlider;
	// Fl_Slider* lightSlider;
	// Fl_Slider* scaleSlider;
	// Fl_Button* openFileButton;
	Fl_Button* reloadButton;
	// Fl_Button* useNormalMapButton;

	Fl_Slider* fog_start_slider;
	Fl_Slider* fog_end_slider;
	Fl_Slider* spot_light_angle_slider;
	Fl_Slider* spot_light_exponent_slider;
	Fl_Slider* spot_light_lookat_mod_x_slider;
	Fl_Slider* spot_light_lookat_mod_y_slider;

	MyGLCanvas* canvas;

public:
	// APP WINDOW CONSTRUCTOR
	MyAppWindow(int W, int H, const char* L = 0);

	static void idleCB(void* userdata) {
		win->canvas->redraw();
	}

private:
	// Someone changed one of the sliders
	static void floatCB(Fl_Widget* w, void* userdata) {
		float value = ((Fl_Slider*)w)->value();
		*((float*)userdata) = value;
	}

	static void intCB(Fl_Widget* w, void* userdata) {
		int value = ((Fl_Button*)w)->value();
		printf("value: %d\n", value);
		*((int*)userdata) = value;
	}

	static void loadFileCB(Fl_Widget* w, void* data) {
		Fl_File_Chooser G_chooser("", "", Fl_File_Chooser::MULTI, "");
		G_chooser.show();
		G_chooser.directory("./data");
		while (G_chooser.shown()) {
			Fl::wait();
		}

		// Print the results
		if (G_chooser.value() == NULL) {
			printf("User cancelled file chooser\n");
			return;
		}

		cout << "Loading new PLY file from: " << G_chooser.value() << endl;
		win->canvas->loadPLY(G_chooser.value());
		win->canvas->redraw();
	}

	static void loadEnvFileCB(Fl_Widget* w, void* data) {
		Fl_File_Chooser G_chooser("", "", Fl_File_Chooser::MULTI, "");
		G_chooser.show();
		G_chooser.directory("./data");
		while (G_chooser.shown()) {
			Fl::wait();
		}

		// Print the results
		if (G_chooser.value() == NULL) {
			printf("User cancelled file chooser\n");
			return;
		}

		cout << "Loading new PPM file from: " << G_chooser.value() << endl;
		win->canvas->loadEnvironmentTexture(G_chooser.value());
		win->canvas->redraw();
	}

	static void loadTextureFileCB(Fl_Widget* w, void* data) {
		Fl_File_Chooser G_chooser("", "", Fl_File_Chooser::MULTI, "");
		G_chooser.show();
		G_chooser.directory("./data");
		while (G_chooser.shown()) {
			Fl::wait();
		}

		// Print the results
		if (G_chooser.value() == NULL) {
			printf("User cancelled file chooser\n");
			return;
		}

		cout << "Loading new PPM file from: " << G_chooser.value() << endl;
		win->canvas->loadObjectTexture(G_chooser.value());
		win->canvas->redraw();
	}


	static void reloadCB(Fl_Widget* w, void* userdata) {
		win->canvas->reloadShaders();
	}
};


MyAppWindow::MyAppWindow(int W, int H, const char* L) : Fl_Window(W, H, L) {
	begin();
	// OpenGL window

	canvas = new MyGLCanvas(10, 10, w() - 320, h() - 20);

	Fl_Pack* pack = new Fl_Pack(w() - 310, 30, 150, h(), "");
	pack->box(FL_DOWN_FRAME);
	pack->type(Fl_Pack::VERTICAL);
	pack->spacing(30);
	pack->begin();

	Fl_Pack* envpack = new Fl_Pack(w() - 100, 30, 100, h(), "Spotlight");
	envpack->box(FL_DOWN_FRAME);
	envpack->labelfont(1);
	envpack->type(Fl_Pack::VERTICAL);
	envpack->spacing(0);
	envpack->begin();

	Fl_Box* spot_light_angle_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "angle");
	spot_light_angle_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	spot_light_angle_slider->align(FL_ALIGN_TOP);
	spot_light_angle_slider->type(FL_HOR_SLIDER);
	spot_light_angle_slider->bounds(0.0, 180.0);
	spot_light_angle_slider->step(1.0);
	spot_light_angle_slider->value(canvas->spot_light_angle_deg);
	spot_light_angle_slider->callback(floatCB, (void*)(&(canvas->spot_light_angle_deg)));

	Fl_Box* spot_light_exponent_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "exponent");
	spot_light_exponent_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	spot_light_exponent_slider->align(FL_ALIGN_TOP);
	spot_light_exponent_slider->type(FL_HOR_SLIDER);
	spot_light_exponent_slider->bounds(1.0, 100.0);
	spot_light_exponent_slider->step(1.0);
	spot_light_exponent_slider->value(canvas->spot_light_exponent);
	spot_light_exponent_slider->callback(floatCB, (void*)(&(canvas->spot_light_exponent)));


	Fl_Box* spot_light_lookat_x_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "x");
	spot_light_lookat_mod_x_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	spot_light_lookat_mod_x_slider->align(FL_ALIGN_TOP);
	spot_light_lookat_mod_x_slider->type(FL_HOR_SLIDER);
	spot_light_lookat_mod_x_slider->bounds(-10.0, 10.0);
	spot_light_lookat_mod_x_slider->step(0.1);
	spot_light_lookat_mod_x_slider->value(canvas->spot_light_lookat_mod.x);
	spot_light_lookat_mod_x_slider->callback(floatCB, (void*)(&(canvas->spot_light_lookat_mod.x)));

	Fl_Box* spot_light_lookat_y_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "y");
	spot_light_lookat_mod_y_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	spot_light_lookat_mod_y_slider->align(FL_ALIGN_TOP);
	spot_light_lookat_mod_y_slider->type(FL_HOR_SLIDER);
	spot_light_lookat_mod_y_slider->bounds(-10.0, 10.0);
	spot_light_lookat_mod_y_slider->step(0.1);
	spot_light_lookat_mod_y_slider->value(canvas->spot_light_lookat_mod.y);
	spot_light_lookat_mod_y_slider->callback(floatCB, (void*)(&(canvas->spot_light_lookat_mod.y)));

	// Fl_Box* texTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "Sphere Map");
	// openFileButton = new Fl_Button(0, 0, pack->w() - 20, 20, "Load PPM");
	// openFileButton->callback(loadEnvFileCB, (void*)this);
	// envpack->end();


	// Fl_Pack* objectpack = new Fl_Pack(w() - 100, 30, 100, h(), "Object Model");
	// objectpack->box(FL_DOWN_FRAME);
	// objectpack->labelfont(1);
	// objectpack->type(Fl_Pack::VERTICAL);
	// objectpack->spacing(0);
	// objectpack->begin();

	// //slider for controlling rotation
	// Fl_Box* rotXTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateX");
	// rotXSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotXSlider->align(FL_ALIGN_TOP);
	// rotXSlider->type(FL_HOR_SLIDER);
	// rotXSlider->bounds(-359, 359);
	// rotXSlider->step(1);
	// rotXSlider->value(canvas->rotVec.x);
	// rotXSlider->callback(floatCB, (void*)(&(canvas->rotVec.x)));

	// Fl_Box* rotYTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateY");
	// rotYSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotYSlider->align(FL_ALIGN_TOP);
	// rotYSlider->type(FL_HOR_SLIDER);
	// rotYSlider->bounds(-359, 359);
	// rotYSlider->step(1);
	// rotYSlider->value(canvas->rotVec.y);
	// rotYSlider->callback(floatCB, (void*)(&(canvas->rotVec.y)));

	// Fl_Box* rotZTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateZ");
	// rotZSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotZSlider->align(FL_ALIGN_TOP);
	// rotZSlider->type(FL_HOR_SLIDER);
	// rotZSlider->bounds(-359, 359);
	// rotZSlider->step(1);
	// rotZSlider->value(canvas->rotVec.z);
	// rotZSlider->callback(floatCB, (void*)(&(canvas->rotVec.z)));

	// Fl_Box* scaleTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "Scale");
	// scaleSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// scaleSlider->align(FL_ALIGN_TOP);
	// scaleSlider->type(FL_HOR_SLIDER);
	// scaleSlider->bounds(0.1, 5);
	// scaleSlider->step(0.1);
	// scaleSlider->value(canvas->scaleFactor);
	// scaleSlider->callback(floatCB, (void*)(&(canvas->scaleFactor)));

	// objectpack->end();

	// Fl_Pack* worldpack = new Fl_Pack(w() - 100, 130, 100, h(), "World Model");
	// worldpack->box(FL_DOWN_FRAME);
	// worldpack->labelfont(1);
	// worldpack->type(Fl_Pack::VERTICAL);
	// worldpack->spacing(0);
	// worldpack->begin();


	//slider for controlling rotation
	// rotXTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateX");
	// rotXSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotXSlider->align(FL_ALIGN_TOP);
	// rotXSlider->type(FL_HOR_SLIDER);
	// rotXSlider->bounds(-359, 359);
	// rotXSlider->step(1);
	// rotXSlider->value(canvas->rotWorldVec.x);
	// rotXSlider->callback(floatCB, (void*)(&(canvas->rotWorldVec.x)));

	// rotYTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateY");
	// rotYSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotYSlider->align(FL_ALIGN_TOP);
	// rotYSlider->type(FL_HOR_SLIDER);
	// rotYSlider->bounds(-359, 359);
	// rotYSlider->step(1);
	// rotYSlider->value(canvas->rotWorldVec.y);
	// rotYSlider->callback(floatCB, (void*)(&(canvas->rotWorldVec.y)));

	// rotZTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "RotateZ");
	// rotZSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// rotZSlider->align(FL_ALIGN_TOP);
	// rotZSlider->type(FL_HOR_SLIDER);
	// rotZSlider->bounds(-359, 359);
	// rotZSlider->step(1);
	// rotZSlider->value(canvas->rotWorldVec.z);
	// rotZSlider->callback(floatCB, (void*)(&(canvas->rotWorldVec.z)));

	// worldpack->end();
	pack->end();


	Fl_Pack* packCol2 = new Fl_Pack(w() - 155, 30, 150, h(), "");
	packCol2->box(FL_DOWN_FRAME);
	packCol2->type(Fl_Pack::VERTICAL);
	packCol2->spacing(30);
	packCol2->begin();

	// Fl_Pack* packObj = new Fl_Pack(w() - 100, 30, 100, h(), "Object");
	// packObj->box(FL_DOWN_FRAME);
	// packObj->labelfont(1);
	// packObj->type(Fl_Pack::VERTICAL);
	// packObj->spacing(0);
	// packObj->begin();

	// Fl_Box* plyTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "Object File");
	// openFileButton = new Fl_Button(0, 0, pack->w() - 20, 20, "Load PLY");
	// openFileButton->callback(loadFileCB, (void*)this);

	// Fl_Box* textureTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "Texture File");
	// openFileButton = new Fl_Button(0, 0, pack->w() - 20, 20, "Load Texture");
	// openFileButton->callback(loadTextureFileCB, (void*)this);


	// Fl_Box* lightTextbox = new Fl_Box(0, 0, pack->w() - 20, 20, "Texture Blend");
	// lightSlider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	// lightSlider->align(FL_ALIGN_TOP);
	// lightSlider->type(FL_HOR_SLIDER);
	// lightSlider->bounds(0, 1);
	// lightSlider->step(0.01);
	// lightSlider->value(canvas->textureBlend);
	// lightSlider->callback(floatCB, (void*)(&(canvas->textureBlend)));

	// useNormalMapButton = new Fl_Check_Button(0, 100, pack->w() - 20, 20, "Diffuse Shading");
	// useNormalMapButton->callback(intCB, (void*)(&(canvas->useDiffuse)));
	// useNormalMapButton->value(canvas->useDiffuse);
	// packObj->end();


	Fl_Pack* packShaders = new Fl_Pack(w() - 100, 30, 100, h(), "Shaders");
	packShaders->box(FL_DOWN_FRAME);
	packShaders->labelfont(1);
	packShaders->type(Fl_Pack::VERTICAL);
	packShaders->spacing(0);
	packShaders->begin();

	reloadButton = new Fl_Button(0, 0, pack->w() - 20, 20, "Reload");
	reloadButton->callback(reloadCB, (void*)this);
	packShaders->end();


	Fl_Pack* packFog = new Fl_Pack(w() - 100, 30, 100, h(), "Fog");
	packFog->box(FL_DOWN_FRAME);
	packFog->labelfont(1);
	packFog->type(Fl_Pack::VERTICAL);
	packFog->spacing(0);
	packFog->begin();

	Fl_Box* fog_start_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "start");
	fog_start_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	fog_start_slider->align(FL_ALIGN_TOP);
	fog_start_slider->type(FL_HOR_SLIDER);
	fog_start_slider->bounds(0.0, 20.0);
	fog_start_slider->step(1.0);
	fog_start_slider->value(canvas->fog_start);
	fog_start_slider->callback(floatCB, (void*)(&(canvas->fog_start)));

	Fl_Box* fog_end_text_box = new Fl_Box(0, 0, pack->w() - 20, 20, "end");
	fog_end_slider = new Fl_Value_Slider(0, 0, pack->w() - 20, 20, "");
	fog_end_slider->align(FL_ALIGN_TOP);
	fog_end_slider->type(FL_HOR_SLIDER);
	fog_end_slider->bounds(5.0, 30.0);
	fog_end_slider->step(1.0);
	fog_end_slider->value(canvas->fog_end);
	fog_end_slider->callback(floatCB, (void*)(&(canvas->fog_end)));

	packFog->end();
	packCol2->end();

	end();
}


/**************************************** main() ********************/
int main(int argc, char** argv) {
	win = new MyAppWindow(850, 475, "Environment Mapping");
	win->resizable(win);
	Fl::add_idle(MyAppWindow::idleCB);
	win->show();
	return(Fl::run());
}