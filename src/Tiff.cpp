#include <Tiff.h>

#include <iostream>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Tiff::Tiff() : _tifin(nullptr), num_directories(0), directory_infos(nullptr), filename(0) {
      TIFFSetErrorHandler(NULL);
        TIFFSetErrorHandlerExt(NULL);
 TIFFSetWarningHandler(NULL);
 TIFFSetWarningHandlerExt(NULL);

}
Tiff::~Tiff() { close(); }

boolean Tiff::loadFromFile(const char* name) {
        TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
        TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0); // unlimited
        _tifin = TIFFOpenExt(name, "rb", opts);
        TIFFOpenOptionsFree(opts);
         if (_tifin) {
            num_directories = TIFFNumberOfDirectories(_tifin);
            current_directory = 0;
            filename = name;
        }
       return _tifin != nullptr;
     }

    boolean Tiff::isLoaded() {
        return _tifin != nullptr;
    }

    TiffDirectory* Tiff::getDirectoryInfo(int directory) {
        if (directory_infos == nullptr)
            buildDirectoryInfo();
        return (directory >= num_directories) ? nullptr : &directory_infos[directory];
    }

    void Tiff::buildDirectoryInfo() {
        delete [] directory_infos;
        directory_infos = new TiffDirectory[num_directories];
        for(int d = 0; d < num_directories; ++d) {
            TIFFSetDirectory(_tifin, d);
              directory_infos[d].getInfo(this);
            directory_infos[d].level = d;
        }
        TIFFSetDirectory(_tifin, current_directory);
    }

    void TiffDirectory::getInfo(Tiff* tiff) {
        TIFF* tifin = tiff->_tifin;
        TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &image_width);
        TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &image_height); 
        TIFFGetFieldDefaulted(tifin, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
        TIFFGetFieldDefaulted(tifin, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
         TIFFGetFieldDefaulted(tifin, TIFFTAG_COMPRESSION, &compression);
         isCompressionConfigured =  TIFFIsCODECConfigured(compression);
         if (compression == COMPRESSION_JPEG)
            TIFFGetFieldDefaulted(tifin, TIFFTAG_JPEGQUALITY, &compression_level);
            else if (compression == COMPRESSION_ADOBE_DEFLATE || compression == COMPRESSION_DEFLATE)
           TIFFGetFieldDefaulted(tifin, TIFFTAG_ZIPQUALITY, &compression_level);         
        is_tiled = TIFFIsTiled(tifin);
        if (is_tiled) {
            TIFFGetFieldDefaulted(tifin, TIFFTAG_TILEWIDTH, &tile_width);
            TIFFGetFieldDefaulted(tifin, TIFFTAG_TILELENGTH, &tile_height);
        } else {
            TIFFGetFieldDefaulted(tifin, TIFFTAG_ROWSPERSTRIP, &tile_height);       
         }
        TIFFGetFieldDefaulted(tifin, TIFFTAG_SUBFILETYPE, &subfiletype);
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
    }

static char error_buffer[1024];

    boolean Tiff::setDirectory(int directory) {
        if (directory >= num_directories || directory < 0) return false;
        current_directory = directory;
        TIFFSetDirectory(_tifin, current_directory);
        return true;
    }

    void Tiff::close() {

        delete [] directory_infos;
        TIFFClose(_tifin);       
    }

    const char* TiffDirectory::compressionDescription() {
    static char buffer[64];   
    static char description[128];
    switch(compression) {
        case COMPRESSION_ADOBE_DEFLATE:    
        case COMPRESSION_DEFLATE:
            snprintf(buffer, sizeof(buffer), "Deflate Level %d", compression_level == -1 ? 6 : compression_level);
            break;
        case COMPRESSION_JPEG:
            snprintf(buffer, sizeof(buffer), "Jpeg Level %d",  compression_level);
            break;
       case COMPRESSION_NONE:
            snprintf(buffer, sizeof(buffer), "No compression");
            break;
            case 5:
             snprintf(buffer, sizeof(buffer), "LZW");
            break;
        case 33003:
        snprintf(buffer, sizeof(buffer), "Aperio jpeg2000 Ycbcr");
            break;
       case 33005:
        snprintf(buffer, sizeof(buffer), "Aperio jpeg2000 RGB");
            break;
        default:
          snprintf(buffer, sizeof(buffer), "%d", compression);  
    }
    snprintf(description, sizeof(description), "%s %s", buffer, isCompressionConfigured ? "" : "not supported");
    return description;
  }

    const char* TiffDirectory::subfileDescription() {
switch(subfiletype) {
    case FILETYPE_REDUCEDIMAGE: return "Reduced"; break;
   case FILETYPE_PAGE: return "Page"; break;
   default: return "Normal"; break;
    }
    }

    unsigned char* Tiff::loadTileOrStrip(int directory, int column, int row) {
         TiffDirectory* di = getDirectoryInfo(directory);
        if (di == nullptr || !di->isValid()) return nullptr;
        if (!setDirectory(directory) || column < 0 || row < 0 || column >= di->image_columns || row >= di->image_rows)
            return nullptr;
        unsigned char* data = new unsigned char[di->tile_width * di->tile_height * 4];
        if (!data) {
            snprintf(error_buffer, 1024, "Error allocating data for tile %dx%d", column, row);
            return nullptr;
        } 
        boolean ok = false;
        if (di->is_tiled) {
            ok = TIFFReadRGBATile(_tifin, column * di->tile_width, row * di->tile_height, (uint32_t *)data) == 1;
        } else {
            ok = TIFFReadRGBAStrip(_tifin, row * di->tile_height, (uint32_t *)data) == 1;
        }      
        if (ok) stbi__vertical_flip(data, di->tile_width, di->tile_height,4);
        else {
            snprintf(error_buffer, 1024, "Error reading %s %dx%d", di->is_tiled ? "tile": "strip", column, row);
            delete [] data;
            data = 0;
        }
        return data;
    }
 