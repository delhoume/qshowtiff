#include <Window.h>
#include <imgui.h>

#include <iostream>

#include <Tiff.h>

class TiffViewer : public pmp::Window {
public:
    TiffViewer(int width, int height);
    ~TiffViewer() override;

    void display() override;
    void process_imgui() override;
    void displayImGuiContents() override;
    void resize(int width, int height) override;

    bool load(const char* filename);
    bool update(); // when something change

    uint32_t texture(const unsigned char* data, int width, int height);
    
    Tiff the_tiff;

     int current_directory;
     int current_column;
     int current_row;
     int visible_columns;
     int visible_rows;
     int max_visible_columns;
     int max_visible_rows;
     GLuint* _textures;
     int num_textures; // hack when update
 bool show_borders;

};
