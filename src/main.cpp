// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"

#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include "space_font.h"

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

extern "C" {
#include <tiffio.h>
#include <zlib.h>
#include <jpeglib.h>
}


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct MyTiff {
    TIFF *tifin;
    unsigned int directories;
    int32_t bits_per_sample;
    int32_t samples_per_pixel;
    int32_t compression;
    int32_t compression_level;
    boolean is_tiled;
    unsigned int tile_width;
    unsigned int tile_height;   // also strip_height;
    // image specific
    unsigned int image_width;
    unsigned int image_height;
    unsigned int image_columns;
    unsigned int image_rows;
  
    MyTiff(const char *filename);
    virtual ~MyTiff();
    void close();
    boolean setDirectory(int directory);
    boolean loadTile(unsigned char *data, int directory, int col, int row);
    boolean loadStrip(unsigned char *data, int directory, int strip);
    void blendWithChecker(unsigned char* data, unsigned int width, unsigned int height, unsigned int col, unsigned int row);
    GLuint load(int directory, int col, int row);
    const char* compressionDescription();
};

MyTiff::MyTiff(const char *filename) {
    TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
    TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0); // unlimited
    tifin = TIFFOpenExt(filename, "rb", opts);
    TIFFOpenOptionsFree(opts);
    if (tifin)  {
        TIFFGetFieldDefaulted(tifin, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
        TIFFGetFieldDefaulted(tifin, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
         TIFFGetFieldDefaulted(tifin, TIFFTAG_COMPRESSION, &compression);
         if (compression == COMPRESSION_JPEG)
            TIFFGetFieldDefaulted(tifin, TIFFTAG_JPEGQUALITY, &compression_level);
            else if (compression == COMPRESSION_ADOBE_DEFLATE || compression == COMPRESSION_DEFLATE)
           TIFFGetFieldDefaulted(tifin, TIFFTAG_ZIPQUALITY, &compression_level); 
        TIFFGetFieldDefaulted(tifin, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
        directories = TIFFNumberOfDirectories(tifin);
        is_tiled = TIFFIsTiled(tifin);
        if (is_tiled) {
            TIFFGetFieldDefaulted(tifin, TIFFTAG_TILEWIDTH, &tile_width);
            TIFFGetFieldDefaulted(tifin, TIFFTAG_TILELENGTH, &tile_height);
        } else {
            TIFFGetFieldDefaulted(tifin, TIFFTAG_ROWSPERSTRIP, &tile_height);       
         }
    }
}

MyTiff::~MyTiff() {
    close();
}

void MyTiff::close() {
    TIFFClose(tifin);
}

const char* MyTiff::compressionDescription() {
    static char buffer[128];
    switch(compression) {
        case COMPRESSION_ADOBE_DEFLATE:    
        case COMPRESSION_DEFLATE:
            snprintf(buffer, 32, "Deflate Level %d", compression_level == -1 ? 6 : compression_level);
            break;
        case COMPRESSION_JPEG:
            snprintf(buffer, 128, "Jpeg Level %d",  compression_level);
            break;
       case COMPRESSION_NONE:
            snprintf(buffer, 128, "None");
            break;
        default:
          snprintf(buffer, 128, "%d", compression);  
    }
    return buffer;
}


boolean MyTiff::setDirectory(int directory) {
if (directory >= directories) return false;
    TIFFSetDirectory(tifin, directory);
    TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &image_width);
    TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &image_height); 
    if (is_tiled) {
        image_columns = image_width / tile_width;
        if (image_width %   tile_width)
            ++image_columns;

        image_rows = image_height / tile_height;
        if (image_height %   tile_height)
            ++image_rows;
    } else {
        tile_width = image_width;
        image_columns = 1;
        image_rows = image_height / tile_height;
        if (image_height % tile_height)
        ++ image_rows;
    }
    return true;
}


int min_height = 32;
int max_width = 2048;
int max_height =2048;
int min_width = 32;
int default_tile_width = 256;
int default_tile_height= 256;

static GLuint DataToTexture(unsigned char* data, int width, int height) {
    if (!data) return 0;
    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    int final_width = width > max_width ? max_width : width;
    if (final_width <= min_width) final_width = min_width;
    int final_height = height > max_height ? max_height : height;
    if (final_height <= min_height) final_height = min_height;

// set alpha to 1 as loadRGBAX seems to fill hidden pixels with 0
//     for (unsigned int p = 0; p < (width * height); ++p)
//         data[4 * p + 3] = 255;
    unsigned char* newdata = new unsigned char[final_width * final_height * 4];
    stbir_resize_uint8_linear(data, width, height, 0, newdata, final_width, final_height, 0, (stbir_pixel_layout)4);
    delete [] data;
    data = newdata;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, final_width, final_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    delete[] data;
    return texture_id;
}

unsigned char grey[4] = { 0xa0, 0xa0, 0xa0, 0xff };
unsigned char white[4] = { 0xff , 0xff, 0xff, 0xff };
unsigned int checker_size = 120;

void blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4]) {
    unsigned int alpha = fg[3] + 1;
    unsigned int inv_alpha = 256 - fg[3];
    result[0] = (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8);
    result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
    result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
    result[3] = 0xff;
}

unsigned char* checker(boolean evenrow, boolean evencol) {
    return (evenrow && evencol) || (!evenrow && !evencol) ? grey : white;
}

void MyTiff::blendWithChecker(unsigned char* data, unsigned int width, unsigned int height, unsigned int col, unsigned int row) {
      for (unsigned r = 0; r < height; ++r) {
            unsigned char *startrow = data + r * 4 * width;
            boolean evenrow = ((r + row * tile_height) % checker_size) < (checker_size / 2);
            for (unsigned int c = 0; c < width; ++c)  {
                boolean evencol = ((c + col * tile_width) % checker_size) < (checker_size / 2);
                blend(startrow + c * 4, startrow + c * 4, checker(evencol, evenrow));
             }
      }
}

static char errorbuffer[1024] = { 0 };

static void RedirectErrors(const char* module, const char* fmt, va_list ap) {
  vsnprintf(errorbuffer, 1024, fmt, ap);
}

boolean debug_load_tiff = true;
GLuint MyTiff::load(int directory, int column, int row) {
   if (!setDirectory(directory))
        return 0;
   unsigned char *data = new unsigned char[tile_width * tile_height * 4]; 
    is_tiled ? loadTile(data, directory, column, row) : loadStrip(data, directory, row);
    if (debug_load_tiff)    {
        char buffer[128];
        snprintf(buffer, 128, "debug/directory_%.2d_column_%.4d_row_%.4d.png", directory, column, row);
        int final_width = tile_width > max_width ? max_width : tile_width;
        if (final_width <= min_width) final_width = min_width;
        int final_height = tile_height > max_height ? max_height : tile_height;
        if (final_height <= min_height) final_height = min_height;

        unsigned char* newdata = new unsigned char[final_width * final_height * 4];
       stbir_resize_uint8_linear(data, tile_width, tile_height, 0, newdata, final_width, final_height, 0, (stbir_pixel_layout)4);
        stbi_write_png(buffer, final_width, final_height, 4, newdata, 0);
        delete [] newdata;
    }
    return ::DataToTexture(data, tile_width, tile_height);
 }

static void SwapVertical(unsigned char* data, int width, int height) { // 4 bytes per pixel and 32 bytes at once
 uint32_t *top = (uint32_t *)data;
    uint32_t *bottom = (uint32_t *)(data + (height - 1) * width * 4);
    uint32_t* temp = (uint32_t *)new unsigned char[width * 4];
    for (unsigned int yy = 0; yy < height / 2; ++yy) {
#if 0
        for (unsigned int xx = 0; xx < width; ++xx) {
            uint32_t temp = top[xx];
            top[xx] = bottom[xx];
            bottom[xx] = temp;
        }
#else
        stbir_simd_memcpy(temp, top, 4 * width);
        stbir_simd_memcpy(top, bottom, 4 * width);
        stbir_simd_memcpy(bottom, temp, 4 * width);         
#endif
        top += width;
        bottom -= width;
    }
}

boolean MyTiff::loadTile(unsigned char *data, int directory, int column, int row) {
  if (column >= image_columns || row >= image_rows)
     return 0;
 boolean ok = TIFFReadRGBATile(tifin, column * tile_width, row * tile_height, (uint32_t *)data) == 1;
         // TIFFReadRGBATile returns upside-down data...
    if (!ok) {
         snprintf(errorbuffer, 1024, "Error reading tile %d %d", column, row);
          return 0;
    }
    ::SwapVertical(data, tile_width, tile_height);
    blendWithChecker(data, tile_width, tile_height, column, row);

    return ok;
}

boolean MyTiff::loadStrip(unsigned char *data, int directory, int row) {
    setDirectory(directory);
    if (directory >= directories || row >= image_rows)
        return 0;
    boolean ok = TIFFReadRGBAStrip(tifin, row * tile_height, (uint32_t *)data) == 1;
   if (!ok) {
         snprintf(errorbuffer, 1024, "Error reading strip %d", row);
          return 0;
    }
    SwapVertical(data, tile_width, tile_height);
    blendWithChecker(data, tile_width, tile_height, 1, row);
    return ok;
}

int main(int argc, char **argv) {
   TIFFSetWarningHandler(RedirectErrors);
    TIFFSetErrorHandler(RedirectErrors);
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1024, 1024, "qshowtiff  / F. Delhoume / 2024", nullptr, nullptr);
    if (window == nullptr)
        return 1;

      const GLFWvidmode* vmode =  glfwGetVideoMode(glfwGetPrimaryMonitor());
        max_width = vmode->width;
        max_height = vmode->height;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    const int font_size = 24;
    io.Fonts->AddFontFromMemoryCompressedTTF(space_font_compressed_data, space_font_compressed_size, font_size);

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
     ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // load first tiles
    int current_column = 0;
    int current_row = 0;
    int current_dir = 0;
    const char *filename = argc > 1 ? argv[1] : 0;
     MyTiff *mytiff = filename ? new MyTiff(filename) : nullptr;
    GLuint texture = mytiff ? mytiff->load(current_dir, current_column, current_row) : 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
          // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

     ImGuiViewport *viewport = ImGui::GetMainViewport();
        float fheight = ImGui::GetFrameHeight();
        ImGui::NewFrame();

    
        bool show_demo_window = texture == 0;
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (true) {
          int wwidth, wheight;
            glfwGetWindowSize(window, &wwidth, &wheight);
                ImGui::SetNextWindowSize(ImVec2(wwidth, wheight - fheight)); // ensures ImGui fits the GLFW window
                ImGui::SetNextWindowPos(ImVec2(0, 0));               
     if (texture) { 
         // main window
         bool opened = false;
              ImGui::Begin(filename ? filename : "...", &opened, ImGuiWindowFlags_NoResize);
            boolean update = false;
            if (ImGui::SliderInt("Directory", &current_dir, 0, mytiff->directories - 1)) {
                mytiff->setDirectory(current_dir);
                current_column = 0;
                current_row = 0;
                update = true;
            } 
            if (mytiff->is_tiled) {
                if (ImGui::SliderInt("Column", &current_column, 0, mytiff->image_columns - 1)) {
                update = true;
            }
            }
            if (ImGui::SliderInt("Row", &current_row, 0, mytiff->image_rows - 1))  update = true;
            if (update && texture) {
                    glDeleteTextures(1, &texture);
                    errorbuffer[0] = 0;
                    texture = mytiff->load(current_dir, current_column, current_row);  
            }
            // image info
            ImGui::Text("Bits per sample: %d, Samples per pixel: %d, Compression: %s", mytiff->bits_per_sample, mytiff->samples_per_pixel,  mytiff->compressionDescription());
            // tile info
            if (mytiff->is_tiled) {
                ImGui::Text("Directories: %d | Tiles size: %d x %d", mytiff->directories, mytiff->tile_width, mytiff->tile_height);
                ImGui::Text("Directory: %d | Image: %d x %d | Tiles: %d x %d", current_dir, mytiff->image_width, mytiff->image_height, mytiff->image_columns, mytiff->image_rows);
                int left = current_column * mytiff->tile_width;
                int top = current_row * mytiff->tile_height;
                ImGui::Text("Tile: %d x %d (%d) | Corners: %d %d -=> %d %d", current_column, current_row, current_column + mytiff->image_columns * current_row, left, top, left + mytiff->tile_width, top + mytiff->tile_height);;
            } else {
                ImGui::Text("Directories: %d | Strip height: %d", mytiff->directories, mytiff->tile_height);
                int top = current_row * mytiff->tile_height;
               ImGui::Text("Directory: %d | Dimensions: %d x %d | Strips: %d", current_dir, mytiff->image_width, mytiff->image_height, mytiff->image_rows);
                ImGui::Text("Strip: %d | Corners: 0, %d -=> %d, %d", current_row, top, mytiff->tile_width, top +  mytiff->tile_height);   
            }
            if (texture) {
                float ratio = mytiff->tile_width / (float)mytiff->tile_height;
                int final_width = mytiff->tile_width > max_width ? max_width : mytiff->tile_width;
                if (final_width <= min_width) final_width = min_width;
                int wanted_height = final_width / ratio;
                int final_height = wanted_height > max_height ? max_height : wanted_height;
                if (final_height <= min_height) final_height = min_height;

                ImVec2 avail_size = ImGui::GetContentRegionAvail();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                pos.x += ImCeil(ImMax(0.0f, (avail_size.x - final_width) * 0.5f));
                pos.y += ImCeil(ImMax(0.0f, (avail_size.y - final_height) * 0.5f));
                ImGui::SetCursorScreenPos(pos);
               ImGui::Image((void *)(intptr_t)texture, ImVec2(final_width, final_height));
            }
             ImGui::End();   
        }
        if (ImGui::BeginViewportSideBar("StatusBar", viewport, ImGuiDir_Down, fheight, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
           if (ImGui::BeginMenuBar())  {
                ImGui::Text("%s", errorbuffer);
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    }
   glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w); 
   glClear(GL_COLOR_BUFFER_BIT);
 
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
 
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    }
    if (mytiff)  
        delete mytiff;
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
