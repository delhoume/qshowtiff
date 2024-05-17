#include <TiffViewer.h>

#include <iostream>
#include <vector>
#include <utility>
#include <random>

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb_image_resize2.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <implot.h>


TiffViewer::TiffViewer(int width, int height)
    : Window("qshowtiff -  F. Delhoume 2024", width, height, true),
    current_directory(0),
     tiles(nullptr),
    show_borders(false),      
    show_numbers(false),    
    full_image_mode(false),
    show_over_limit_tiles(false),
    show_image_rect(true),
    heatmaps(nullptr),
    mini_heatmaps(nullptr),
    mini_heatmaps_widths(nullptr),
    mini_heatmaps_heights(nullptr),
    maxheats(nullptr),
    visible_columns(nullptr),
    visible_rows(nullptr),
    current_columns(nullptr),
    current_rows(nullptr),
    nomem(0),
    memory_limit(1000),
    loaded_tiles(0),
    shuffle_mode(true) {
  add_help_item("D", "Open Debug Windows");
}


void TiffViewer::display()  {
 ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
     glClear(GL_COLOR_BUFFER_BIT);

}

#define MAX(a,b) (a) > (b) ? (a) : (b)
#define MIN(a,b) (a) > (b) ? (b) : (a)

#define TILE_BORDER_COLOR IM_COL32(255, 0, 0, 255)
#define IMAGE_BORDER_COLOR IM_COL32(0, 0, 255, 255)

const int checker_size = 256;

void TiffViewer::displayTiles() {
    if (the_tiff.isLoaded()) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
       if (di->isValid()) {
           ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("BCKGND", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            ImGui::SetWindowSize(display_size);
            for (int y = 0; y < display_size.y; y += checker_size) {
                for (int x = 0; x < display_size.x; x += checker_size) {
                    ImVec2 topleft(x, y);
                    ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)nomem, ImVec2(x, y), ImVec2(x + checker_size, y + checker_size));
              }
            }
                       
           // TODO: factorize code for both modes
            int displayed_tiles_x = MIN(di->image_columns - current_columns[current_directory], visible_columns[current_directory]);
            int displayed_tiles_y = MIN(di->image_rows - current_rows[current_directory], visible_rows[current_directory]);
            if (full_image_mode) {
                 boolean  isimagefit = false;// center real image or center tilesdims ? 
                int total_width = isimagefit ? di->image_width : di->tile_width * di->image_columns;
                int total_height = isimagefit ? di->image_height : di->tile_height * di->image_rows;

                float scalex = display_size.x / total_width;
                float scaley = display_size.y / total_height;

                float scale = scalex > scaley ? scaley : scalex;

                scale *= 0.95;

                float real_tile_width = scale * di->tile_width;
                float real_tile_height = scale* di->tile_height;

                float real_width = scale * total_width;
                float real_height = scale * total_height;

                int startx = (display_size.x - real_width) / 2;         
                int starty = (display_size.y - real_height) / 2;

                GLuint* textures = tiles[current_directory];
                for (int jj = 0; jj < di->image_rows; ++jj) {
                    boolean isRowVisible = (jj >= current_rows[current_directory] && jj < (current_rows[current_directory] +  displayed_tiles_y));
                     for (int ii = 0; ii < di->image_columns; ++ii) {
                        ImVec2 top_left(startx + ii * real_tile_width, starty + jj * real_tile_height);
                        ImVec2 bottom_right(top_left.x + real_tile_width, top_left.y + real_tile_height);
                           // but only draw visible tiles
                        boolean isColumnVisible = (ii >= current_columns[current_directory] && ii < (current_columns[current_directory] + displayed_tiles_x));
                        if (isRowVisible && isColumnVisible) {
                            int tile_index = jj * di->image_columns + ii;
                            if (textures[tile_index] == 0) {
                                if (show_over_limit_tiles == true) {
                                    ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)nomem, top_left, bottom_right);
                                }
                            } else {          
                                 ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)textures[tile_index], top_left, bottom_right);
                            }
                        // // draw borders and pos for all tiles (missing last border)
                            if (show_numbers) {
                                static char info[16];
                                snprintf(info, 16, "%dx%d", ii, jj);
                                ImGui::GetWindowDrawList()->AddText(ImVec2(top_left.x + 5, top_left.y + 5), IM_COL32(255, 255, 255, 255), info);
                            }
                        }
                        if (show_borders) {
                            ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + ii * real_tile_width, starty), ImVec2(startx + ii * real_tile_width, starty + real_tile_height * di->image_rows), TILE_BORDER_COLOR);             
                        }
                    }
                   // show horizontal grid minus last, and text
                    if (show_borders) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + jj * real_tile_height), ImVec2(startx + real_tile_width * di->image_columns, starty + jj * real_tile_height),TILE_BORDER_COLOR);
                    }
                }
                // finish grid
                if (show_borders) {
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + di->image_rows * real_tile_height), ImVec2(startx + di->image_columns * real_tile_width, starty + di->image_rows * real_tile_height), TILE_BORDER_COLOR);
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + di->image_columns * real_tile_width, starty), ImVec2(startx + di->image_columns * real_tile_width, starty + di->image_rows * real_tile_height), TILE_BORDER_COLOR);
                }
                if (show_image_rect) {
                    ImGui::GetWindowDrawList()->AddRect(ImVec2(startx, starty), ImVec2(startx + scale * di->image_width , starty + scale * di->image_height), IMAGE_BORDER_COLOR);
                } 
            } else {
                int total_width = di->tile_width * displayed_tiles_x;
                int total_height = di->tile_height * displayed_tiles_y;

                float scalex = display_size.x / total_width;
                float scaley = display_size.y / total_height;

                float scale = scalex > scaley ? scaley : scalex;

                scale *= 0.95;

                float real_tile_width = scale * di->tile_width;
                float real_tile_height = scale* di->tile_height;

                float real_width = scale * total_width;
                float real_height = scale * total_height;

                int startx = (display_size.x - real_width) / 2;         
                int starty = (display_size.y - real_height) / 2;
                             
                GLuint* textures = tiles[current_directory];
               for (int jj = 0; jj < displayed_tiles_y; ++jj) {
                    int tiley = current_rows[current_directory] + jj;
                    for (int ii = 0; ii < displayed_tiles_x; ++ii) {
                        int tilex = current_columns[current_directory] + ii;
                        ImVec2 top_left(startx + ii * real_tile_width, starty + jj * real_tile_height);
                        ImVec2 bottom_right(top_left.x + real_tile_width, top_left.y +real_tile_height);
                        int tile_index = tiley * di->image_columns + tilex;
                        if (textures[tile_index] == 0) {
                            if (show_over_limit_tiles == true) {
                                ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)nomem, top_left, bottom_right);
                            }
                        } else {          
                            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)textures[tile_index], top_left, bottom_right);
                        }
                        if(show_numbers) {
                                    static char info[16];
                                snprintf(info, 16, "%dx%d", current_columns[current_directory] + ii, current_rows[current_directory] + jj);
                                ImGui::GetWindowDrawList()->AddText(ImVec2(top_left.x + 5, top_left.y + 5), IM_COL32(255, 255, 255, 255), info);    
                        }
                        if (show_borders) {
                            ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + ii * real_tile_width, starty), ImVec2(startx + ii * real_tile_width, starty + real_tile_height * displayed_tiles_y), TILE_BORDER_COLOR); 
                        }
                    }
                    if (show_borders) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + jj * real_tile_height), ImVec2(startx + real_tile_width * displayed_tiles_x, starty + jj * real_tile_height), TILE_BORDER_COLOR);
                     }
               }
               if (show_borders) {
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx, starty + displayed_tiles_y * real_tile_height), ImVec2(startx + displayed_tiles_x * real_tile_width, starty + displayed_tiles_y * real_tile_height), TILE_BORDER_COLOR);
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(startx + displayed_tiles_x * real_tile_width, starty), ImVec2(startx + displayed_tiles_x * real_tile_width, starty + displayed_tiles_y * real_tile_height), TILE_BORDER_COLOR);
                }
        }
        ImGui::End();
    }
 }
}

static char memoryUsageStr[64];
void humanMemorySize(uint64_t bytes) {
    const char *sizeNames[] = { "B", "KB", "MB", "GB", "TB" };
    uint64_t i = (uint64_t) floor(log(bytes) / log(1024));
    double humanSize = bytes / pow(1024, i);
    snprintf(memoryUsageStr, 64, "%g %s", humanSize, sizeNames[i]);
}


static bool show_imgui_metrics = false;
static bool show_implot_metrics = false;

void TiffViewer::process_imgui() {
    if (the_tiff.isLoaded()) {
        displayTiles();
          for (int d = 0; d < the_tiff.num_directories; ++d) {
            ImGui::PushID(d);
            TiffDirectory* di = the_tiff.getDirectoryInfo(d);

            int displayed_tiles_x = MIN(di->image_columns - current_columns[current_directory], visible_columns[current_directory]);
            int displayed_tiles_y = MIN(di->image_rows - current_rows[current_directory], visible_rows[current_directory]);

           char buff[64];
            snprintf(buff, 64, "Directory %d; %d x %d  %s", d, di->image_width, di->image_height, di->subfileDescription());
            ImGui::Separator();
            if (ImGui::RadioButton(buff, &current_directory, d)) update();
            ImGui::BulletText("%s | %d bps | %d spp", di->compressionDescription(), di->bits_per_sample, di->samples_per_pixel);
            if (di->is_tiled) ImGui::BulletText("tile size %d x %d | %d x %d tiles (%d)", di->tile_width, di->tile_height, di->image_columns, di->image_rows, di->image_columns * di->image_rows);
            else ImGui::BulletText("strip height %d | %d strips", di->tile_height, di->image_rows);   
            
            float r =  di->image_width / (float)di->image_height;
            int heatmax_size = 200;
            int heat_width = r > 1 ? heatmax_size : heatmax_size * r;
            int heat_height = r > 1 ? heatmax_size / r : heatmax_size;
            if (d != current_directory) ImGui::BeginDisabled();
            ImGui::PushItemWidth(heat_width);
            if (di->is_tiled) {  
                //if (ImGui::DragIntRange2("Coloadtilelumns", &current_columns[d], &visible_columns[d], 1.0, 0, di->image_columns - 1, "Start: %d", "Width: %d")) update();                 
                 if(ImGui::SliderInt("Columns", &(visible_columns[d]), 1, di->image_columns)) {
                    update();
                 }
                if(ImGui::SliderInt("Column", &(current_columns[d]), 0, di->image_columns - 1)) {
                    update();  
                }         
            }
            // heatmap
            static ImPlotHeatmapFlags hm_flags = 0;
            static ImPlotColormap map  = ImPlotColormap_Viridis;
            ImPlot::PushColormap(map);
            static ImPlotAxisFlags axes_flag = ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;
            if (ImPlot::BeginPlot("ImPlot", ImVec2(heat_width, heat_height), ImPlotFlags_NoTitle |ImPlotFlags_NoInputs | ImPlotFlags_NoLegend | ImPlotFlags_NoFrame | ImPlotFlags_NoMouseText)) {
                ImPlot::SetupAxes(NULL, NULL, axes_flag, axes_flag);
                if (mini_heatmaps[d] != nullptr) {
                    ImPlot::SetupAxesLimits(0, mini_heatmaps_widths[d], 0, mini_heatmaps_heights[d]);
                    ImPlot::PlotHeatmap("Heatmap", mini_heatmaps[d], mini_heatmaps_heights[d], mini_heatmaps_widths[d], 0, maxheats[d], NULL, 
                        ImPlotPoint(0,0), ImPlotPoint(mini_heatmaps_widths[d],mini_heatmaps_heights[d]), hm_flags);
                } else if (heatmaps[d] != nullptr)  {
                    ImPlot::SetupAxesLimits(0, di->image_columns, 0, di->image_rows);
                    ImPlot::PlotHeatmap("Heatmap", heatmaps[d], di->image_rows, di->image_columns, 0, maxheats[d], NULL, ImPlotPoint(0,0), ImPlotPoint(di->image_columns,di->image_rows), hm_flags);
                }
                if (d == current_directory) {
    //                ImGui::SameLine(); ImPlot::ColormapScale("##Heatscale", 0, maxheat[d], ImVec2(0, 0), "", ImPlotColormapScaleFlags_NoLabel);
                }
              ImPlot::EndPlot();
            }
            ImGui::SameLine();
            if(ImGui::VSliderInt("Row", ImVec2(18,  200), &(current_rows[d]), 0, di->image_rows - 1)) {
                update();
            }
            ImGui::SameLine();
            if(ImGui::VSliderInt("Rows", ImVec2(18,  200), &(visible_rows[d]), 1, di->image_rows)) {
                update();
            }
            ImGui::PopItemWidth();
            if (d == current_directory) {
                ImGui::Checkbox("Show tiles borders", &show_borders);               
                ImGui::SameLine();  ImGui::Checkbox("Show tiles numbers", &show_numbers); 

                ImGui::Checkbox("Full image mode", &full_image_mode); 
                if (!full_image_mode) ImGui::BeginDisabled();
                    ImGui::SameLine();  ImGui::Checkbox("Show image border", &show_image_rect);
               if (!full_image_mode) ImGui::EndDisabled();

                uint64_t requested_tiles = displayed_tiles_x * displayed_tiles_y;
                TiffDirectory* cdi = the_tiff.getDirectoryInfo(current_directory);
                uint64_t one_tile_memory = 4 * cdi->tile_height * cdi->tile_width;
                uint64_t memory_requested = requested_tiles * one_tile_memory / 1000000;
                int percent_loaded = loaded_tiles / (float)requested_tiles * 100;
                  ImGui::BulletText("Limit memory to");
                ImGui::SameLine();
                if(ImGui::SliderInt("##", &memory_limit, 0, 32000, "%d Mb")) {
                    update();  
                }
                if (ImGui::Checkbox("Shuffle",  &shuffle_mode)) update();
               ImGui::BulletText("Tiles requested %llu (%d x %d and %llu Mb)", requested_tiles, displayed_tiles_x, displayed_tiles_y, memory_requested);
               ImGui::BulletText("Tiles loaded: %d (%d%%)", loaded_tiles, percent_loaded);
            }
            if (d != current_directory) ImGui::EndDisabled();
            ImGui::PopID();
        }
        if (show_imgui_metrics) ImGui::ShowMetricsWindow(&show_imgui_metrics);
        if (show_implot_metrics) ImPlot::ShowMetricsWindow(&show_implot_metrics);
    } else {
        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();
    }
}

 // now recompute everything
 // TODO: make a list then load and enforce memory limit
bool TiffViewer::update() {
    if (the_tiff.isLoaded()) {
        cleanDirectory(current_directory);
        TiffDirectory* di = the_tiff.getDirectoryInfo(current_directory);
          // pass 1 compute which tiles mstr be loaded

        std::vector<std::pair<int, int>> tiles_to_load;
         for (int jj = 0; jj < di->image_rows; ++jj) {
            boolean isRowVisible = (jj >= current_rows[current_directory] && jj < (current_rows[current_directory] + visible_rows[current_directory]));
            for (int ii = 0; ii < di->image_columns; ++ii) {
                boolean isColumnVisible = (ii >= current_columns[current_directory] && ii < (current_columns[current_directory] + visible_columns[current_directory]));
                if(isColumnVisible && isRowVisible) {
                    tiles_to_load.push_back(std::make_pair(ii, jj));
                }
            }
        }
        // pass 2 shuffle 
        if (shuffle_mode) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(tiles_to_load.begin(), tiles_to_load.end(), gen);
        }

        // pass 3 load
         loaded_tiles = 0;
        int max_load = memory_limit * 1000000 / (di->tile_width * di->tile_height * 4);
        GLuint* ttiles = tiles[current_directory];
        float* heatmap = heatmaps != nullptr ? heatmaps[current_directory]  : nullptr;

        for (int tt = 0; tt < tiles_to_load.size(); ++tt) {
            std::pair<int,int> tile_to_load = tiles_to_load[tt];
            int column = tile_to_load.first;
            int row = tile_to_load.second;
            int tile_index = row * di->image_columns + column;
            if (loaded_tiles >= max_load) {
                ttiles[tile_index] = 0;
            } else {
                unsigned char* data = the_tiff.loadTileOrStrip(current_directory, column, row);
                ttiles[tile_index] = texture(data, di->tile_width, di->tile_height);
                if (heatmap !=  nullptr)  {
                    heatmap[tile_index] += 1.0;
                    if (heatmap[tile_index] > maxheats[current_directory]) {
                        maxheats[current_directory] = heatmap[tile_index];
                    }
                }
                ++loaded_tiles;
            }
        }
        if (heatmaps != nullptr && mini_heatmaps != nullptr && heatmaps[current_directory] != nullptr && mini_heatmaps[current_directory] != nullptr) {
            stbir_resize_float_linear(heatmaps[current_directory], di->image_columns, di->image_rows, 0, 
                mini_heatmaps[current_directory], mini_heatmaps_widths[current_directory], mini_heatmaps_heights[current_directory], 0, (stbir_pixel_layout)1);
        }
    }
    return true;
}

// reset the textures
void TiffViewer::cleanDirectory(int directory) {
         TiffDirectory* di = the_tiff.getDirectoryInfo(directory);
         int num_tiles = di->image_columns * di->image_rows;
        if (tiles[directory] != nullptr) {
            glDeleteTextures(num_tiles, tiles[directory]);
            memset(tiles[directory], 0, num_tiles * sizeof (int));
        }
    }

void TiffViewer::clean() {
    for (int d = 0; d < the_tiff.num_directories; ++d) {
        cleanDirectory(d);
        if (mini_heatmaps != nullptr)
            delete [] mini_heatmaps[d];
    }
    delete [] heatmaps;   heatmaps = nullptr;
    delete [] mini_heatmaps; mini_heatmaps = nullptr;
   delete [] mini_heatmaps_widths; mini_heatmaps_widths = nullptr;
   delete [] mini_heatmaps_heights; mini_heatmaps_heights = nullptr;
    delete [] maxheats; maxheats  = nullptr;
    delete [] visible_columns; visible_columns = nullptr;
    delete [] visible_rows; visible_rows  = nullptr;    
    delete [] tiles; tiles = nullptr;
    delete [] current_columns; current_columns = nullptr;
    delete [] current_rows; current_rows = nullptr;
    loaded_tiles = 0;
}


int mini_heatmap_limit = 20;
int heat_limit = mini_heatmap_limit * 5;
void TiffViewer::init() {   
        // TODO: one class
        heatmaps = new float*[the_tiff.num_directories];
        mini_heatmaps = new float*[the_tiff.num_directories];
        mini_heatmaps_widths = new int[the_tiff.num_directories];
        mini_heatmaps_heights = new int[the_tiff.num_directories];
        maxheats = new float[the_tiff.num_directories];

    tiles = new GLuint*[the_tiff.num_directories];
    visible_columns = new int[the_tiff.num_directories];
    visible_rows = new int[the_tiff.num_directories];
    current_columns = new int[the_tiff.num_directories];
    current_rows = new int[the_tiff.num_directories];
    for (int d = 0; d < the_tiff.num_directories; ++d) {
        TiffDirectory* di = the_tiff.getDirectoryInfo(d);
        int num_tiles = di->image_columns * di->image_rows;
        if (num_tiles < (heat_limit * heat_limit)) {
           heatmaps[d] = new float[num_tiles];
        }
        tiles[d] = new GLuint[num_tiles];
       for (int idx = 0; idx < num_tiles; ++idx) {
            if (heatmaps[d] != nullptr) {
                heatmaps[d][idx] = 0.0;
            }
            tiles[d][idx] = 0;
       }
        // allocate lower res heatmap
            if ((heatmaps[d] != nullptr) && (di->image_columns * di->image_rows > (mini_heatmap_limit * mini_heatmap_limit))) {
                float ratio = di->image_columns / di->image_rows;
                int mini_heat_width;
                int mini_heat_height;
                if (ratio > 1) {
                    mini_heat_width = mini_heatmap_limit;
                    mini_heat_height = mini_heatmap_limit / ratio;
                } else {
                    mini_heat_width = mini_heatmap_limit * ratio;
                    mini_heat_height = mini_heatmap_limit;
                }
                mini_heatmaps[d] = new float[mini_heat_width * mini_heat_height]; 
                memset(mini_heatmaps[d], 0, mini_heat_width * mini_heat_height * sizeof(float));
                mini_heatmaps_widths[d] = mini_heat_width;    
                mini_heatmaps_heights[d] = mini_heat_height;
            } else {
                mini_heatmaps[d] = nullptr;
                mini_heatmaps_widths[d] = 0;    
                mini_heatmaps_heights[d] = 0;
            }
        current_columns[d] = 0;    
        current_rows[d] = 0;
        visible_columns[d] = 1 ;
        visible_rows[d] = 1;
    }
    // 
    int nomem_size = 128;
    int checker_size  = nomem_size / 2;
    unsigned char gray1[4] = { 0xee, 0xee, 0xee, 0xff };
     unsigned char gray2[4] = { 0xcc, 0xcc, 0xcc, 0xff };
    unsigned char* data = new unsigned char[nomem_size * nomem_size * 4]; 

    for (int y = 0; y < nomem_size; ++y) {
        boolean isYEven = (y % checker_size) > (checker_size / 2);
        for (int x = 0; x < nomem_size; ++x) {
            boolean isXEven = (x % checker_size) > (checker_size / 2);         
            unsigned char* ptr = data + y * 4 * nomem_size +  4 * x;
            memcpy(ptr, ((isXEven && isYEven) || (!isXEven && !isYEven)) ? gray1: gray2, 4); 
        }
    }
    nomem = texture(data, nomem_size, nomem_size);
}

void TiffViewer::resize(int width, int height) {
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

void TiffViewer::keyboard(int key, int code, int action, int mods)  {
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key) {
        case GLFW_KEY_D:        {
           show_imgui_metrics = true;          
            show_implot_metrics = true;          
              break;
        }

        default: {
            Window::keyboard(key, code, action, mods);
            break;
        }
    }
}
