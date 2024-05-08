#include <Window.h>
#include <imgui.h>

#include <iostream>

#include <Tiff.h>

class TiffViewer : public pmp::Window {
public:
    TiffViewer(int width, int height);

    void display() override;;
    void process_imgui() override;
    void displayImGuiContents() override;
    void resize(int width, int height) override;

    bool load(const char* filename);
    bool update(); // when something change

    uint32_t texture(const unsigned char* data, int width, int height);
    
    Tiff the_tiff;

     int current_directory;
     GLuint* _textures;
     int num_textures; // hack when update
    bool show_borders;
    float** heatmaps;
    float* maxheat;
    int* visible_columns;
    int* visible_rows;
     int* current_columns;
     int* current_rows;
   void init();    
    void clean();

};
