#ifndef ART_MANAGER_H
#define ART_MANAGER_H

#define DO_DOWNLOAD (false)
#define NUM_ROOMS_AHEAD_TO_RENDER (2)
#define NUM_RENDERED_ROOMS (2 * (NUM_ROOMS_AHEAD_TO_RENDER) + 1)
#define NUM_PAINTINGS_PER_ROOM (2)
#define NUM_RENDERED_PAINTINGS (NUM_PAINTINGS_PER_ROOM * NUM_RENDERED_ROOMS)
#define MAX_ART_DIM (1024)

#include <array>
#include <string>

class ArtManager {
public:
    ArtManager();
    void download_and_convert(const size_t ii);
	size_t get_buffer_size() { return this->buffer_size; }

	std::array<size_t, NUM_PAINTINGS_PER_ROOM> get_buffer_idxs() { return this->buffer_idxs; }
	void set_buffer_idxs(const std::array<size_t, NUM_PAINTINGS_PER_ROOM>  buffer_idxs) { this->buffer_idxs = buffer_idxs; }

    float get_aspect_ratio(const size_t idx);

	bool is_loading_ppm() { return this->loading_ppm; }
	void set_loading_ppm() { this->loading_ppm = true; }
	void unset_loading_ppm() { this->loading_ppm = false; }
    bool is_downloading_image() { return this->downloading_image; }
    void set_downloading_image() { this->downloading_image = true; }
    void unset_downloading_image() { this->downloading_image = false; }

    bool should_download(const size_t idx) { return this->should_download_[idx]; }
    void set_should_download(const size_t idx) { this->should_download_[idx] = true; }
    void unset_should_download(const size_t idx) { this->should_download_[idx] = false; }


    bool is_bound(const size_t idx) { return this->bound[idx]; }

    void set_texture_id(const size_t idx, const unsigned int id) { this->texture_ids[idx] = id; }
    unsigned int get_texture_id(const size_t idx) { return this->texture_ids[idx]; }

    void read_ppm(const size_t idx, const std::string& filename);

    // THESE MUST ONLY BE CALLED BY THE PARENT PROCESS.
    void bind(const size_t idx);
    void unbind(const size_t idx);


    void test_set(char val) {
        buffer[0][0] = val;
    }
    char test_get() {
        return buffer[0][0];
    }


private:
    bool downloading_image;
	bool loading_ppm;
    bool should_download_[NUM_PAINTINGS_PER_ROOM];
    char buffer[NUM_RENDERED_PAINTINGS][MAX_ART_DIM * MAX_ART_DIM * 3];
    int widths[NUM_RENDERED_PAINTINGS];
    int heights[NUM_RENDERED_PAINTINGS];
    bool bound[NUM_RENDERED_PAINTINGS];
    unsigned int texture_ids[NUM_RENDERED_PAINTINGS];
	int buffer_size;
	std::array<size_t, NUM_PAINTINGS_PER_ROOM> buffer_idxs;
};

#endif // ART_MANAGER_H