#ifndef ART_MANAGER_H
#define ART_MANAGER_H

#define BUFFER_SIZE (5)
#define MAX_ART_DIM (1024)

#include <string>

class ArtManager {
public:
    ArtManager();
    void download();
	size_t get_buffer_size() { return this->buffer_size; }

	size_t get_buffer_idx() { return this->buffer_idx; }
	void set_buffer_idx(const size_t buffer_idx) { this->buffer_idx = buffer_idx; }

	bool is_loading() { return this->loading; }
	void set_loading() { this->loading = true; }
	void unset_loading() { this->loading = false; }

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
	bool loading;
    char buffer[BUFFER_SIZE][MAX_ART_DIM * MAX_ART_DIM * 3];
    int widths[BUFFER_SIZE];
    int heights[BUFFER_SIZE];
    bool bound[BUFFER_SIZE];
    unsigned int texture_ids[BUFFER_SIZE];
	int buffer_size;
	size_t buffer_idx;
};

#endif // ART_MANAGER_H