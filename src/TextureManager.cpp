/*  =================== File Information =================
	File Name: object.h
	Description:
	Author: Michael Shah
	Last Modified: 4/2/14

	Purpose:
	Usage:

	Further Reading resources:
	===================================================== */

#include <iostream>
#include <cmath>
#include "TextureManager.h"
#include <glm/gtc/constants.hpp>

#define PI glm::pi<float>()

using namespace::std;

TextureManager::TextureManager(){
}

TextureManager ::~TextureManager(){
	for (auto const& it : textures) {
		delete it.second;  //delete the memory of the ShaderProgram object
	}
	textures.clear();
}

/*	===============================================
Desc:
Precondition:
Postcondition:
=============================================== */
void TextureManager::loadTexture(string textureName, string fileName) {
	// std::cout << "TextureManager::loadTexture" << std::endl;
	ppm* curPPM = NULL;
	auto it = textures.find(textureName);
	if (it == textures.end()) {  //ppm not found
		curPPM = new ppm(fileName);
	}
	else {
		curPPM = it->second;
	}
	curPPM->bindTexture();
	textures[textureName] = curPPM;  //add the newly added texture to the map
}

void TextureManager::deleteTexture(string textureName) {
	// std::cout << "TextureManager::deleteTexture" << std::endl;
	ppm* curPPM = NULL;
	// std::cout << "TextureManager::deleteTexture : finding texture " << textureName << std::endl;
	auto it = textures.find(textureName);
	if (it != textures.end()) {  //ppm found
		// std::cout << "TextureManager::deleteTexture : texture found" << std::endl;
		delete it->second;
		// std::cout << "TextureManager::deleteTexture : texture freed" << std::endl;
		textures.erase(it); //erasing the entry by key
	}
}


unsigned int TextureManager::getTextureID(std::string textureName) {
	auto it = textures.find(textureName);
	if (it == textures.end()) {  //ppm not found
		// cout << "texutre name not found!!!" << endl;
		return -1;
	}
	return it->second->getTextureID();
}

