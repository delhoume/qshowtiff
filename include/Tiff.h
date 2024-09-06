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
    int previous_strip;
    unsigned char* full_strip_data;

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
    const char* sampleFormatDescription();
    int level;
    int32_t bits_per_sample;
    int32_t samples_per_pixel;
    int32_t sample_format;
    int32_t compression;
    int32_t compression_level;
    boolean isCompressionConfigured;
    boolean is_tiled;
    int subfiletype;
    int tile_width;
    int tile_height;   // also strip_height;
    int image_width;
    int image_height;
    int image_columns;
    int image_rows;
};
