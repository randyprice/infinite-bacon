PROJECT := a5

BIN_DIR := bin
INC_DIR := inc
SRC_DIR := src

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst %.cpp,%.o,$(patsubst $(SRC_DIR)/%,$(BIN_DIR)/%,$(SRC)))
HDR := $(wildcard $(INC_DIR)/*.h)

EXE := $(BIN_DIR)/$(PROJECT).out
OS := $(shell uname)
ifeq ($(OS), Linux)
	CXX := $(shell fltk-config --cxx) -std=c++11
	CXXFLAGS := -fPIC $(shell fltk-config --cxxflags) -Iinc
	LDFLAGS := -lGLEW -lGL -lGLU $(shell fltk-config --use-glut --use-gl --use-images --ldflags)
else ifeq ($(OS), Darwin)
	BREWPATH := $(shell brew --prefix)
	CXX := $(shell fltk-config --cxx) -std=c++11 -D_CRT_SECURE_NO_WARNINGS -DGL_SILENCE_DEPRECATION -Wno-macro-redefined -O2
	CXXFLAGS := $(shell fltk-config --cxxflags) -I$(BREWPATH)/include
	LDFLAGS := $(shell fltk-config --ldflags --use-gl --use-images) -L$(BREWPATH)/lib
	POSTBUILD := fltk-config --post
endif

$(EXE): $(OBJ)
	@$(CXX) $^ -o $@ -pthread $(LDFLAGS)
ifeq ($(OS), Darwin)
	@$(POSTBUILD) $@
endif

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if [ ! -d $(BIN_DIR) ]; then mkdir $(BIN_DIR); fi
	@$(CXX) $(CXXFLAGS) -pthread -c $^ -o $@

.PHONY: build
build: $(EXE)

.PHONY: run
run: $(EXE)
	@./$(EXE)

.PHONY: debug
debug: $(EXE)
	@ rm -f logs/debug.log
	@./$(EXE) d

.PHONY: clean
clean:
	@rm -rf $(EXE) $(BIN_DIR)/*.o *~ *.dSYM
