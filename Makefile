
#CXX=g++-14
CXX=clang

IMGUI_DIR=/Users/fredericdelhoume/Documents/GitHub/imgui
IMPLOT_DIR=/Users/fredericdelhoume/Documents/GitHub/implot
GLAD_DIR=glad
SRC_DIR=src
INCLUDE_DIR=include
BIN_DIR=bin
DATA_DIR=data
HOMEBREW_DIR=/opt/homebrew
HOMEBREW_LIB_DIR=$(HOMEBREW_DIR)/lib
HOMEBREW_INCLUDE_DIR=$(HOMEBREW_DIR)/include
GLEW_DIR=
COMPILER_INCLUDE_DIR=
FILE_TO_BINARY=$(IMGUI_DIR)/misc/fonts/binary_to_compressed_c

EXE=qshowtiff
EXE_STATIC=qshowtiff.static

all: $(BIN_DIR)/$(EXE)

OBJECTS = $(BIN_DIR)/imgui.o $(BIN_DIR)/imgui_demo.o $(BIN_DIR)/imgui_draw.o $(BIN_DIR)/imgui_tables.o $(BIN_DIR)/imgui_widgets.o $(BIN_DIR)/imgui_impl_glfw.o $(BIN_DIR)/imgui_impl_opengl3.o $(BIN_DIR)/implot.o $(BIN_DIR)/implot_items.o $(BIN_DIR)/implot_demo.o $(BIN_DIR)/Window.o $(BIN_DIR)/qshowtiff.o $(BIN_DIR)/TiffViewer.o $(BIN_DIR)/Tiff.o 

TIFF_LIBS_DYNAMIC = -ltiff -lturbojpeg -lz-ng -lzstd -llzma
TIFF_LIBS_STATIC= $(HOMEBREW_LIB_DIR)/libtiff.a $(HOMEBREW_LIB_DIR)/libturbojpeg.a $(HOMEBREW_LIB_DIR)/libz-ng.a $(HOMEBREW_LIB_DIR)/liblzma.a $(HOMEBREW_LIB_DIR)/libzstd.a		
GL_LIBS_DYNAMIC = -lglew -lglfw3 
GL_LIBS_STATIC = $(HOMEBREW_LIB_DIR)/libGLEW.a $(HOMEBREW_LIB_DIR)/libglfw3.a
GL_FLAGS=-DGL_SILENCE_DEPRECATION

#CXXFLAGS = -std=c++17 -Wall -Wformat -g $(GL_FLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR) -I$(GLAD_DIR)/include -I$(INCLUDE_DIR) -I$(HOMEBREW_INCLUDE_DIR)
CXXFLAGS = -std=c++17 -Wall -Wformat -O2 $(GL_FLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR) -I$(GLAD_DIR)/include -I$(INCLUDE_DIR) -I$(HOMEBREW_INCLUDE_DIR)

DATA_HEADERS = $(INCLUDE_DIR)/space_font.h $(INCLUDE_DIR)/nomem.h $(INCLUDE_DIR)/background.h


LIBS_DYNAMIC= $(TIFF_LIBS_DYNAMIC) $(GL_LIBS_DYNAMIC) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lc++
LIBS_STATIC = $(TIFF_LIBS_STATIC) $(GL_LIBS_STATIC) 

LDFLAGS_DYNAMIC = -L$(HOMEBREW_LIB_DIR) -L/usr/local/lib
LDFLAGS_STATIC = 


$(INCLUDE_DIR)/space_font.h: $(DATA_DIR)/space_invaders.ttf
	$(FILE_TO_BINARY) $(DATA_DIR)/space_invaders.ttf SpaceFont > $(INCLUDE_DIR)/space_font.h 

$(INCLUDE_DIR)/nomem.h: $(DATA_DIR)/nomem.jpg
	$(FILE_TO_BINARY) -nocompress $(DATA_DIR)/nomem.jpg NoMem > $(INCLUDE_DIR)/nomem.h 

$(INCLUDE_DIR)/background.h: $(DATA_DIR)/background.png
	$(FILE_TO_BINARY) -nocompress $(DATA_DIR)/background.png Background > $(INCLUDE_DIR)/background.h 

$(BIN_DIR)/imgui.o: $(IMGUI_DIR)/imgui.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/imgui.cpp -o $(BIN_DIR)/imgui.o

$(BIN_DIR)/imgui_demo.o: $(IMGUI_DIR)/imgui_demo.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/imgui_demo.cpp -o $(BIN_DIR)/imgui_demo.o

$(BIN_DIR)/imgui_draw.o: $(IMGUI_DIR)/imgui_draw.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/imgui_draw.cpp -o $(BIN_DIR)/imgui_draw.o

$(BIN_DIR)/imgui_tables.o: $(IMGUI_DIR)/imgui_tables.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/imgui_tables.cpp -o $(BIN_DIR)/imgui_tables.o

$(BIN_DIR)/imgui_widgets.o: $(IMGUI_DIR)/imgui_widgets.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/imgui_widgets.cpp -o $(BIN_DIR)/imgui_widgets.o

$(BIN_DIR)/implot.o: $(IMPLOT_DIR)/implot.cpp
	$(CXX) -c $(CXXFLAGS) $(IMPLOT_DIR)/implot.cpp -o $(BIN_DIR)/implot.o

$(BIN_DIR)/implot_demo.o: $(IMPLOT_DIR)/implot_demo.cpp
	$(CXX) -c $(CXXFLAGS) $(IMPLOT_DIR)/implot_demo.cpp -o $(BIN_DIR)/implot_demo.o

$(BIN_DIR)/implot_items.o: $(IMPLOT_DIR)/implot_items.cpp
	$(CXX) -c $(CXXFLAGS) $(IMPLOT_DIR)/implot_items.cpp -o $(BIN_DIR)/implot_items.o

$(BIN_DIR)/imgui_impl_glfw.o: $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp -o $(BIN_DIR)/imgui_impl_glfw.o

$(BIN_DIR)/imgui_impl_opengl3.o: $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
	$(CXX) -c $(CXXFLAGS) $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp -o $(BIN_DIR)/imgui_impl_opengl3.o

$(BIN_DIR)/qshowtiff.o: $(SRC_DIR)/qshowtiff.cpp
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/qshowtiff.cpp -o $(BIN_DIR)/qshowtiff.o

$(BIN_DIR)/Tiff.o: $(SRC_DIR)/Tiff.cpp
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/Tiff.cpp -o $(BIN_DIR)/Tiff.o

$(BIN_DIR)/TiffViewer.o: $(SRC_DIR)/TiffViewer.cpp
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/TiffViewer.cpp -o $(BIN_DIR)/TiffViewer.o
	
$(BIN_DIR)/Window.o: $(SRC_DIR)/Window.cpp 
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/Window.cpp -o $(BIN_DIR)/Window.o

$(BIN_DIR)/$(EXE): $(DATA_HEADERS) $(OBJECTS)
	$(CXX) -v -o $(BIN_DIR)/$(EXE) $(OBJECTS) $(LDFLAGS_DYNAMIC) $(LIBS_DYNAMIC)
	
$(BIN_DIR)/$(EXE_STATIC): $(DATA_HEADERS) $(OBJECTS)
	$(CXX) -o $(BIN_DIR)/$(EXE_STATIC) $(OBJECTS) $(LDFLAGS_STATIC) $(LIBS_STATIC)

clean:
	rm -f $(BIN_DIR)/$(EXE) $(BIN_DIR)/qshowtiff $(BIN_DIR)/*.o $(DATA_HEADERS)
