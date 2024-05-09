#include <TiffViewer.h>

#include <iostream>

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb_image_resize2.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <implot.h>

TiffViewer::TiffViewer(int width, int height)
    : Window("qshowtiff -  F. Delhoume 2024", width, height, true),
    current_directory(0),
     _textures(nullptr),
    show_borders(false),    
    full_image_mode(false),
    heatmaps(nullptr),
    maxheat(nullptr),
    visible_columns(nullptr),
    visible_rows(nullptr),
       current_columns(nullptr),
    current_rows(nullptr) {
}


void TiffViewer::display()  {
 ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
     glClear(GL_COLOR_BUFFER_BIT);

}

#define MAX(a,b) (a) > (b) ? (a) : (b)
#define MIN(a,b) (a) > (b) ? (b) : (a)

void TiffViewer::displayTiles() {
     if (the_tiff.isLoaded()) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
       if (di->isValid()) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("BCKGND", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            ImGui::SetWindowSize(display_size);
            // TODO: factorize code for both modes
            if (full_image_mode) {
                // center image, always at the same position for all directories when ratio is the same
                float scalex = display_size.x / (di->tile_width * di->image_columns);         
                float scaley = display_size.y / (di->tile_height * di->image_rows);

                float scaleuniform = scalex > scaley ? scaley : scalex;
                if (scaleuniform > 1.0) scaleuniform = 1.0;  
                scaleuniform *= 0.9; // margin around image

                float real_width = scaleuniform * di->image_width;
                float real_height = scaleuniform * di->image_height;

                float real_tile_width = scaleuniform * di->tile_width;
                float real_tile_height = scaleuniform * di->tile_height;


                int startx = (display_size.x - real_width) / 2;         
                int starty = (display_size.y - real_height) / 2;

                for (int jj = 0; jj < di->image_rows; ++jj) {
                    boolean isRowVisible = (jj >= current_rows[current_directory] && jj < (current_rows[current_directory] + visible_rows[current_directory]));
                     for (int ii = 0; ii < di->image_columns; ++ii) {
                        ImVec2 topleft(startx + ii * real_tile_width, starty + jj * real_tile_height);
                        ImVec2 bottomRight(topleft.x + real_tile_width, topleft.y + real_tile_height);
                           // but only draw visible tiles
                        boolean isColumnVisible = (ii >= current_columns[current_directory] && ii < (current_columns[current_directory] + visible_columns[current_directory]));
                       if (isRowVisible && isColumnVisible) {
                           int real_index = (jj - current_rows[current_directory]) * visible_columns[current_directory] + (ii - current_columns[current_directory]);
                            if (real_index < (visible_columns[current_directory] * visible_rows[current_directory]))
                                ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)_textures[real_index], topleft, bottomRight);
                          }
                       // // draw borders and pos for all tiles (missing last border)
                        if (show_borders) {
                            ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + ii * real_tile_width, starty), ImVec2(startx + ii * real_tile_width, starty + real_tile_height * di->image_rows), IM_COL32(255, 0, 0, 255));             
                            static char info[16];
                            snprintf(info, 16, "%d x %d", ii - current_columns[current_directory], jj - current_rows[current_directory]);
                            ImGui::GetWindowDrawList()->AddText(ImVec2(topleft.x + 5, topleft.y + 5), IM_COL32(255, 255, 255, 255), info);
                        }
 
                    }
                   // show horizontal grid minus last, and text
                    if (show_borders) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + jj * real_tile_height), ImVec2(startx + real_tile_width * di->image_columns, starty + jj * real_tile_height), IM_COL32(255, 0, 0, 255));
                    }
                }
                // finish grid
                if (show_borders) {
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + di->image_rows * real_tile_height), ImVec2(startx + di->image_columns * real_tile_width, starty + di->image_rows * real_tile_height), IM_COL32(255, 0, 0, 255));
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + di->image_columns * real_tile_width, starty), ImVec2(startx + di->image_columns * real_tile_width, starty + di->image_rows * real_tile_height), IM_COL32(255, 0, 0, 255));
                } 
            } else {
            // fit only visible tiles in window (center tiles)
             float scalex = display_size.x / (visible_columns[current_directory] * di->tile_width);         
             float scaley = display_size.y / (visible_rows[current_directory] * di->tile_height);

             float scaleuniform = scalex > scaley ? scaley : scalex;
           if (scaleuniform > 1.0) scaleuniform = 1.0;  
               scaleuniform *= 0.9; // margin around image
              int displayed_tiles_x = MIN(di->image_columns - current_columns[current_directory], visible_columns[current_directory]);
             int displayed_tiles_y = MIN(di->image_rows - current_rows[current_directory], visible_rows[current_directory]);
             int real_width = displayed_tiles_x * di->tile_width;        
             int real_height = displayed_tiles_y * di->tile_height;
             if (current_columns[current_directory] + visible_columns[current_directory] >= di->image_columns) real_width -= (di->image_columns * di->tile_width - di->image_width); // last column might go after end of image
            if (current_rows[current_directory] + visible_rows[current_directory] >= di->image_rows) real_height -= (di->image_rows * di->tile_height - di->image_height); // last row might go after end of image
            int startx = (display_size.x - (scaleuniform * real_width)) / 2;         
                int starty = (display_size.y - (scaleuniform * real_height)) / 2;
                for (int jj = 0; jj < visible_rows[current_directory]; ++jj) {
                    for (int ii = 0; ii < visible_columns[current_directory]; ++ii) {
                        ImVec2 topleft(startx + ii * scaleuniform * di->tile_width, starty + jj * scaleuniform * di->tile_height);  
                        ImVec2 bottomright(topleft.x + scaleuniform * di->tile_width, topleft.y + scaleuniform * di->tile_height);
                    if (_textures[jj * visible_columns[current_directory] + ii]) {
                            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)_textures[jj * visible_columns[current_directory] + ii], topleft, bottomright);
                            if (show_borders) {
                                static char info[16];
                                snprintf(info, 16, "%d x %d", current_columns[current_directory] + ii, current_rows[current_directory] + jj);
                                ImGui::GetWindowDrawList()->AddRect(topleft, bottomright, IM_COL32(255,0,0,255));
                                topleft.x += 5; topleft.y += 5;
                                ImGui::GetWindowDrawList()->AddText(topleft, IM_COL32(255, 255, 255, 255), info);
                            }
                        }
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
        displayTiles();
          for (int d = 0; d < the_tiff.num_directories; ++d) {
            ImGui::PushID(d);
            TiffDirectory* di = the_tiff.getDirectoryInfo(d);
            char buff[64];
            snprintf(buff, 64, "Directory %d; %d x %d  %s", d, di->image_width, di->image_height, di->subfileDescription());
            ImGui::Separator();
            if (ImGui::RadioButton(buff, &current_directory, d)) update();
            ImGui::BulletText("%s | %d bps | %d spp", di->compressionDescription(), di->bits_per_sample, di->samples_per_pixel);
            if (di->is_tiled) ImGui::BulletText("tile size %d x %d | %d x %d tiles (%d)", di->tile_width, di->tile_height, di->image_columns, di->image_rows, di->image_columns * di->image_rows);
            else ImGui::BulletText("strip height %d | %d strips", di->tile_height, di->image_rows);   
            
            int r =  di->image_width / di->image_height;
            int heat_width = 200;
            int heat_height = heat_width / r;
            if (d != current_directory) ImGui::BeginDisabled();
            ImGui::PushItemWidth(heat_width);
            if (di->is_tiled) {  
                //if (ImGui::DragIntRange2("Columns", &current_columns[d], &visible_columns[d], 1.0, 0, di->image_columns - 1, "Start: %d", "Width: %d")) update();                 
                 if(ImGui::SliderInt("Visible columns", &(visible_columns[d]), 1, di->image_columns)) update();
                if(ImGui::SliderInt("Column", &(current_columns[d]), 0, di->image_columns - 1)) update();           
          }
        // heatmap
            static ImPlotHeatmapFlags hm_flags = 0;
            static ImPlotColormap map  = ImPlotColormap_Viridis;
            ImPlot::PushColormap(map);
            static ImPlotAxisFlags axes_flag = ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;
            if (ImPlot::BeginPlot("ImPlot", ImVec2(heat_width, heat_height), ImPlotFlags_NoTitle |ImPlotFlags_NoInputs | ImPlotFlags_NoLegend | ImPlotFlags_NoFrame | ImPlotFlags_NoMouseText)) {
                ImPlot::SetupAxes(NULL, NULL, axes_flag, axes_flag);
                ImPlot::PlotHeatmap("Heatmap", heatmaps[d],di->image_rows, di->image_columns, 0, maxheat[d], 
                NULL, ImPlotPoint(0,0), ImPlotPoint(1,1), hm_flags);
                ImPlot::EndPlot();
            }
            
            ImGui::SameLine();
            if(ImGui::VSliderInt("Row", ImVec2(18,  200), &(current_rows[d]), 0, di->image_rows - 1)) update();
            ImGui::SameLine();
            if(ImGui::VSliderInt("Visible rows", ImVec2(18,  200), &(visible_rows[d]), 1,  di->image_rows)) update();
            ImGui::PopItemWidth();
            if (d == current_directory) {
                ImGui::Checkbox("Show tiles borders", &show_borders); 
               ImGui::SameLine(); ImGui::Checkbox("Full image mode", &full_image_mode); 
                int used_textures = 0;
                for (int jj = 0; jj < visible_rows[current_directory]; ++jj) {
                        for (int ii = 0; ii < visible_columns[current_directory]; ++ii) {
                            if (_textures[jj * visible_columns[current_directory] + ii] > 0)
                                used_textures++;
                        }
                }
                TiffDirectory* cdi = the_tiff.getDirectoryInfo(current_directory);
                uint64_t memory_used = used_textures * 4 * cdi->tile_height * cdi->tile_width;
                humanMemorySize(memory_used);
                ImGui::BulletText("Loaded tiles: %d, Memory usage %s", used_textures, memoryUsageStr);
            }
        if (d != current_directory) ImGui::EndDisabled();
            ImGui::PopID();
    }
    //  bool yep = true;
    //     ImGui::ShowMetricsWindow(&yep);   
    //     ImPlot::ShowMetricsWindow(&yep);
    } else {
        bool yep = true;
        ImPlot::ShowDemoWindow(&yep);
        ImGui::ShowDemoWindow(&yep);
    }
}

 // now recompute everything
bool TiffViewer::update() {
      if (the_tiff.isLoaded()) {
        if (_textures != nullptr) {
            glDeleteTextures(num_textures, _textures);
            delete [] _textures;      
        }
       TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
         num_textures = visible_columns[current_directory] * visible_rows[current_directory];
        _textures = new GLuint[num_textures];
         for (int jj = 0; jj < visible_rows[current_directory]; ++jj) {
                 for (int ii = 0; ii < visible_columns[current_directory]; ++ii) {
                    // will not be created if invalid tile
                    unsigned char* data = the_tiff.loadTileOrStrip(current_directory, current_columns[current_directory] + ii, current_rows[current_directory] + jj);
                    GLuint text = texture(data, di->tile_width, di->tile_height);
                    _textures[jj * visible_columns[current_directory] + ii] = text;
                     if (text > 0) {
                        float count = heatmaps[current_directory][current_columns[current_directory] + ii + (current_rows[current_directory] + jj) * di->image_columns]++;
                        if (count > maxheat[current_directory])
                            maxheat[current_directory] = count;
                    }
                 }
         }
     }  
    return true;
}

void TiffViewer::clean() {
     if (heatmaps != nullptr) {
        for (int i = 0; i < the_tiff.num_directories; ++i) {
        if (heatmaps[i])
            delete [] heatmaps[i];
    }
    delete [] heatmaps;
    delete [] maxheat;    
    delete [] visible_columns;
    delete [] visible_rows;    
    delete [] maxheat;
    delete [] current_columns;
    delete [] current_rows;
}
}

void TiffViewer::init() {
    heatmaps = new float*[the_tiff.num_directories];
    maxheat = new float[the_tiff.num_directories];
    visible_columns = new int[the_tiff.num_directories];
    visible_rows = new int[the_tiff.num_directories];
    current_columns = new int[the_tiff.num_directories];
    current_rows = new int[the_tiff.num_directories];
    for (int d = 0; d < the_tiff.num_directories; ++d) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(d);
        heatmaps[d] = new float[di->image_columns * di->image_rows];
       for (int idx = 0; idx < di->image_columns * di->image_rows; ++idx) {
        heatmaps[d][idx] = 0.0;
       }
        current_columns[d] = 0;    
        current_rows[d] = 0;
        visible_columns[d] = 1 ;
        visible_rows[d] = 1;
    }
}

void TiffViewer::resize(int width, int height) {
   update();
}

bool TiffViewer::load(const char *filename) {
    boolean success = the_tiff.loadFromFile(filename);
    if (success) {
        init();
        update();
    }
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