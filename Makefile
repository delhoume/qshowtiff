#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CXX = clang++


IMGUI_DIR = imgui
IMPLOT_DIR = ../implot
GLAD_DIR = glad
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
DATA_DIR = data

FILE_TO_BINARY=$(IMGUI_DIR)/misc/fonts/binary_to_compressed_c

EXE=$(BIN_DIR)/qshowtiff

SOURCES = $(SRC_DIR)/qshowtiff.cpp 
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES+= $(IMPLOT_DIR)/implot.cpp $(IMPLOT_DIR)/implot_items.cpp $(IMPLOT_DIR)/implot_demo.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
#SOURCES += $(GFLAD_DIR)/src/glad.c
SOURCES += $(SRC_DIR)/Window.cpp $(SRC_DIR)/qshowtiff.cpp $(SRC_DIR)/TiffViewer.cpp $(SRC_DIR)/Tiff.cpp 

OBJS = $(addprefix $(BIN_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
UNAME_S := $(shell uname -s)
GL_LIBS = -lglew

TIFF_LIBS = -ltiff -lturbojpeg -lz-ng -lzstd -llzma
GL_LIBS = -lglew -lglfw 
GL_FLAGS=-DGL_SILENCE_DEPRECATION

CXXFLAGS = -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR) -I$(GLAD_DIR)/include -I$(INCLUDE_DIR)
CXXFLAGS += -g -Wall -Wformat
CXXFLAGS += $(GL_FLAGS)


DATA_HEADERS = $(INCLUDE_DIR)/space_font.h $(INCLUDE_DIR)/nomem.h $(INCLUDE_DIR)/background.h

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/homebrew/lib
	#LIBS += -lglfw3
	LIBS += $(TIFF_LIBS) $(GL_LIBS) 


	CXXFLAGS += -I/usr/local/include -I/opt/local/include -I/opt/homebrew/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

$(BIN_DIR)/%.o:$(SRC_DIR)/%.cpp $(DATA_HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


$(BIN_DIR)/%.o:$(IMPLOT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


$(BIN_DIR)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o:$(GLAD_DIR)/src/glad.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<


all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(INCLUDE_DIR)/space_font.h: $(DATA_DIR)/space_invaders.ttf
	$(FILE_TO_BINARY) $(DATA_DIR)/space_invaders.ttf SpaceFont > $(INCLUDE_DIR)/space_font.h 

$(INCLUDE_DIR)/nomem.h:$(DATA_DIR)/nomem.jpg
	$(FILE_TO_BINARY) -nocompress $(DATA_DIR)/nomem.jpg NoMem > $(INCLUDE_DIR)/nomem.h 

$(INCLUDE_DIR)/background.h: $(DATA_DIR)/background.png
	$(FILE_TO_BINARY) -nocompress $(DATA_DIR)/background.png Background > $(INCLUDE_DIR)/background.h 

$(EXE): $(OBJS) 
	mkdir -p $(BIN_DIR)
	$(CXX) -v -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS) $(DATA_HEADERS)
