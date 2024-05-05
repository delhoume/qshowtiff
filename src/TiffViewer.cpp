#include <TiffViewer.h>

#include <iostream>

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb_image_resize2.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

TiffViewer::TiffViewer(int width, int height)
    : Window("qshowtiff -  F. Delhoume 2024", width, height, true),
    current_directory(0),
    current_column(0),
    current_row(0),
    visible_columns(1),
    visible_rows(1),
    _textures(nullptr),
    show_borders(false) {
}

TiffViewer::~TiffViewer() = default;

void TiffViewer::display()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void TiffViewer::displayImGuiContents() {
     if (the_tiff.isLoaded()) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
       if (di->isValid()) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("BCKGND", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            ImGui::SetWindowSize(display_size);
            // fit all in window
             float scalex = display_size.x / (visible_columns * di->tile_width);         
             float scaley = display_size.y / (visible_rows * di->tile_height);

             float scaleuniform = scalex > scaley ? scaley : scalex;
             if (scaleuniform > 1.0) scaleuniform = 1.0;
           int startx = (display_size.x - (scaleuniform * visible_columns * di->tile_width)) / 2;         
            int starty = (display_size.y - (scaleuniform * visible_rows * di->tile_height)) / 2;
 //           std::cout << startx << " - " << starty << std::endl;
            for (int jj = 0; jj < visible_rows; ++jj) {
                for (int ii = 0; ii < visible_columns; ++ii) {
                    ImVec2 topleft(startx + ii * scaleuniform * di->tile_width, starty + jj * scaleuniform * di->tile_height);  
                    ImVec2 bottomright(topleft.x + scaleuniform * di->tile_width, topleft.y + scaleuniform * di->tile_height);
                if (_textures[jj * visible_columns + ii]) {
                        ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)_textures[jj * visible_columns + ii], topleft, bottomright);
                        if (show_borders) ImGui::GetWindowDrawList()->AddRect(topleft, bottomright, IM_COL32(255,0,0,255));
                    }
                }  
            }
       }
        ImGui::End();
    }
 }

static char memoryUsageStr[64];
void humanMemorySize(uint64_t bytes) {
    const char *sizeNames[] = { "B", "KB", "MB", "GB", "TB" };

    uint64_t i = (uint64_t) floor(log(bytes) / log(1024));
    double humanSize = bytes / pow(1024, i);
    snprintf(memoryUsageStr, 64, "%g %s", humanSize, sizeNames[i]);
}

void TiffViewer::process_imgui() {
    if (the_tiff.isLoaded()) {
        ImGui::BulletText("%d director%s", the_tiff.num_directories, the_tiff.num_directories == 1 ? "y" : "ies");
         for (int d = 0; d < the_tiff.num_directories; ++d) {
            TiffDirectory* di = the_tiff.getDirectoryInfo(d);
            ImGui::BulletText("%d | %s image | %d x %d", d,  di->subfileDescription(), di->image_width, di->image_height);
       if (d == current_directory) { 
                ImGui::GetBackgroundDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 200, 0,255));
            }
          ImGui::Indent();
             ImGui::BulletText("%s | %d bps | %d spp", di->compressionDescription(), di->bits_per_sample, di->samples_per_pixel);
            if (di->is_tiled) ImGui::BulletText("tile size %d x %d | %d x %d tiles (%d)", di->tile_width, di->tile_height, di->image_columns, di->image_rows, di->image_columns * di->image_rows);
            else ImGui::BulletText("strip height %d | %d strips", di->tile_height, di->image_rows);
            ImGui::Unindent();
       
            }
        if(ImGui::SliderInt("Directory", &current_directory, 0, the_tiff.num_directories - 1)) {
            current_column = 0;
            current_row = 0;
            update();
        }

        TiffDirectory* cdi = the_tiff.getDirectoryInfo(current_directory);
        if (!cdi->is_tiled) ImGui::BeginDisabled(true);
        
       if(ImGui::SliderInt("Column", &current_column, 0, cdi->image_columns - 1)) update();           
       if(ImGui::SliderInt("Visible columns", &visible_columns, 1, cdi->image_columns)) update();
       if (!cdi->is_tiled) ImGui::EndDisabled();
       if(ImGui::SliderInt("Row", &current_row, 0, cdi->image_rows - 1)) update();
       if(ImGui::SliderInt("Visible rows", &visible_rows, 1,  cdi->image_rows)) update();

      ImGui::Checkbox("Show borders", &show_borders);   
      uint64_t memory_used = visible_rows * visible_columns * 4 * cdi->tile_height * cdi->tile_width;
    humanMemorySize(memory_used);
    ImGui::BulletText("Memory usage %s", memoryUsageStr);
    //   if (cdi->is_tiled)
    //     ImGui::BulletText("max visible tiles %d x %d", max_visible_columns, max_visible_rows);
    //     else
    //      ImGui::BulletText("max visible strips %d", max_visible_rows);
    }
}

 // now recompute everything
// TODO:MAYBE cache !
bool TiffViewer::update() {
    if (_textures != nullptr) {
        glDeleteTextures(num_textures, _textures);
        delete [] _textures;      
    }
    if (the_tiff.isLoaded()) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        max_visible_columns = di->image_columns; // display_size.x / di->tile_width;       
        max_visible_rows = di->image_rows; // display_size.y / di->tile_height;
        if (max_visible_columns * di->tile_width < display_size.x)
            max_visible_columns++;
        if (max_visible_rows * di->tile_height < display_size.y)
        max_visible_rows++;       
        if (visible_columns >= max_visible_columns) 
            visible_columns = max_visible_columns;
        if (visible_rows >= max_visible_rows) 
            visible_rows = max_visible_rows;
        if (visible_columns > di->image_columns)
            visible_columns = di->image_columns;
        if (visible_rows > di->image_rows)
            visible_rows = di->image_rows;
        if (visible_columns < 1) visible_columns = 1;        
        if (visible_rows < 1) visible_rows = 1;

        num_textures = visible_columns * visible_rows;
        _textures = new GLuint[num_textures];
#if 0
           for (int jj = 0; jj < visible_rows; ++jj) {
            for (int ii = 0; ii < visible_columns; ++ii) {
                unsigned char* data = the_tiff.loadTileOrStrip(current_directory, current_column + ii, current_row + jj);
                TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
                _textures[jj * visible_columns + ii] = texture(data, di->tile_width, di->tile_height);
            }
        }
#else
           int startj  = (visible_rows % 2) == 0 ? (-visible_rows / 2) + 1 : - (visible_rows - 1)/ 2;
         for (int jj = 0; jj < visible_rows; ++jj, ++startj) {
            int starti  = (visible_columns % 2) == 0 ? (-visible_columns / 2) + 1 : - (visible_columns - 1)/ 2;
                 for (int ii = 0; ii < visible_columns; ++ii, ++starti) {
                    unsigned char* data = the_tiff.loadTileOrStrip(current_directory, current_column + starti, current_row + startj);
                    TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
                    _textures[jj * visible_columns + ii] = texture(data, di->tile_width, di->tile_height);
            }
        }
 #endif   
    }
    return true;
}

void TiffViewer::resize(int width, int height) {
   update();
}

bool TiffViewer::load(const char *filename) {
    boolean success = the_tiff.loadFromFile(filename);
    if (success) update();
    return success;
}

int min_height = 32;
int max_width = 2048;
int max_height = 2048;
int min_width = 32;
int default_tile_width = 256;
int default_tile_height = 256;

uint32_t TiffViewer::texture(const unsigned char *data, int width, int height) {
    if (data == nullptr)
        return 0;
    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

    int final_width = width > max_width ? max_width : width;
    if (final_width <= min_width)
        final_width = min_width;
    int final_height = height > max_height ? max_height : height;
    if (final_height <= min_height)
        final_height = min_height;

if (final_width != width || final_height != height) {
        unsigned char *newdata = new unsigned char[final_width * final_height * 4];
        stbir_resize_uint8_linear(data, width, height, 0, newdata, final_width, final_height, 0, (stbir_pixel_layout)4);
        delete[] data;
        data = newdata;
    }
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, final_width, final_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    delete[] data;
    return texture_id;
}