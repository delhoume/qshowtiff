#include <Window.h>
#include <imgui.h>

#include <iostream>

#include <Tiff.h>

class TiffViewer : public pmp::Window {
public:
    TiffViewer(int width, int height);

    void display() override;;
    void process_imgui() override;
    void displayTiles();
    void resize(int width, int height) override;
    void keyboard(int key, int code, int action, int mods) override;

    bool load(const char* filename);
    bool update(); // when something change

    uint32_t texture(const unsigned char* data, int width, int height);
    
    Tiff the_tiff;

     int current_directory;
     GLuint** tiles;
    bool show_borders;  
    bool show_numbers;    
    bool full_image_mode;
    bool show_over_limit_tiles;
    bool show_image_rect;
    float** heatmaps;   
     float** mini_heatmaps;
     int* mini_heatmaps_widths;     
     int* mini_heatmaps_heights;
    float* maxheats;
    int* visible_columns;
    int* visible_rows;
     int* current_columns;
     int* current_rows;
     GLuint nomem;
    int memory_limit; // in megabytes
    int loaded_tiles;
    bool shuffle_mode;
    void init();    
    void clean();
private: 
    void cleanDirectory(int directory);

};
