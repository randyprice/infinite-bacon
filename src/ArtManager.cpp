#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <FL/gl.h>

#include "ArtManager.h"

ArtManager::ArtManager() {
    this->buffer_size = NUM_RENDERED_PAINTINGS;
    this->loading_ppm = false;
    // this tells MyGLCanvas which texture to load to in the buffer
    for (size_t ii = 0; ii < NUM_PAINTINGS_PER_ROOM; ++ii) {
        this->buffer_idxs[ii] = ii;
        this->should_download_[ii] = false;
    }
    for (size_t ii = 0; ii < NUM_RENDERED_PAINTINGS; ++ii) {
        this->bound[ii] = false;
    }
}

float ArtManager::get_aspect_ratio(const size_t idx) {
    if (this->heights[idx] == 0) {
        return 1.0f;
    }

    return this->widths[idx] / static_cast<float>(this->heights[idx]);
}

void ArtManager::read_ppm(const size_t idx, const std::string& filename) {
    // Open an input file stream for reading a file
    std::ifstream file(filename.c_str());
    // If our file successfully opens, begin to process it.
    if (file.is_open()) {
        // line will store one line of input
        std::string line;
        // Our loop invariant is to continue reading input until
        // we reach the end of the file and it reads in a NULL character
        std::cout << "reading ppm file " << filename << std::endl;
        // Our delimeter pointer which is used to store a single token in a given
        // string split up by some delimeter(s) specified in the strtok function
        char* delimeter_pointer;
        int iteration = 0;
        int pos = 0;
        while (getline(file, line)) {
            char* copy = new char[line.length() + 1];
            strcpy(copy, line.c_str());
            delimeter_pointer = strtok(copy, " ");

            if (copy[0] == '#') {
                continue;
            }
            // Read in the magic number
            if (iteration == 0) {
                std::string magic_number = delimeter_pointer;
                magic_number.erase(std::remove_if(magic_number.begin(), magic_number.end(), ::isspace), magic_number.end());
                if (magic_number.compare("P3") != 0) {
                    std::cout <<  "Incorrect image file format.Cannot load texutre" << std::endl;
                    break;
                }
            }
            // Read in dimensions
            else if (iteration == 1) {
                int width = atoi(delimeter_pointer);
                this->widths[idx] = width;
                delimeter_pointer = strtok(NULL, " ");
                int height = atoi(delimeter_pointer);
                this->heights[idx] = height;
                if (width > MAX_ART_DIM || height > MAX_ART_DIM) {
                    std::cout << "image too large" << std::endl;
                    break;
                } else if (width < 0 || height < 0) {
                    std::cout << "bad ard dimensions" << std::endl;
                    break;
                }
            }
            else if (iteration == 2) {
                int num = this->widths[idx] * this->heights[idx] * 3;
                for (int i = 0; i < num; i++) {
                    int value;
                    file >> value;
                    //std::cout << i << ": " << value << std::endl;
                    this->buffer[idx][i] = value;
                }
            }
            delete [] copy;
            iteration++;
        }
        file.close();
    }
    else{
        std::cout << "Unable to open ppm file: " << filename << std::endl;
    }
}

// bind/unbind MUST ONLY BE CALLED BY THE PARENT PROCESS.

// bind with GL, record bound ID, then set bound to true.
void ArtManager::bind(const size_t idx) {
    std::cout << "binding to buffer idx " << idx << std::endl;
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        this->widths[idx],
        this->heights[idx],
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        this->buffer[idx]
    );
    this->texture_ids[idx] = id;
    this->bound[idx] = true;
}

// Unbind in GL and unset bound, only if already bound.
void ArtManager::unbind(const size_t idx) {
    if (!this->bound[idx]) {
        return;
    }

    std::cout << "unbinding from buffer idx " << idx << std::endl;
    glDeleteTextures(1, &this->texture_ids[idx]);
    this->bound[idx] = false;
}

void ArtManager::download_and_convert(const size_t ii) {
    const char* args[] = {"python3", "./python/download-image3.py", std::to_string(ii).c_str(), nullptr};
    execvp("python3", (char* const*)args);
    std::cout << "execvp failed" << std::endl;
    exit(1);
}
