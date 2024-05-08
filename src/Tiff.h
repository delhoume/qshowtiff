extern "C" {
#include <tiffio.h>
#include <zlib.h>
#include <jpeglib.h>
}

class TiffDirectory;

class Tiff {
    public:
    Tiff();
    virtual ~Tiff();

    boolean loadFromFile(const char* filename);
    boolean setDirectory(int directory);
    void close();
    boolean isLoaded();
     TiffDirectory* getDirectoryInfo(int directory);
    unsigned char* loadTileOrStrip(int directory, int col, int row);

    friend class TiffDirectory; 
     private:
    TIFF* _tifin;
   void buildDirectoryInfo();

    public:
   int num_directories;
   int current_directory;
    TiffDirectory* directory_infos;
  };

class TiffDirectory {
    public:
    TiffDirectory() :  level(-1) {}
void getInfo(Tiff* tiff);
boolean isValid() const { return level != -1; }
    const char* compressionDescription();
    const char* subfileDescription();
    int level;
    int32_t bits_per_sample;
    int32_t samples_per_pixel;
    int32_t compression;
    int32_t compression_level;
    boolean is_tiled;
    int subfiletype;
    unsigned int tile_width;
    unsigned int tile_height;   // also strip_height;
    unsigned int image_width;
    unsigned int image_height;
    unsigned int image_columns;
    unsigned int image_rows;
};