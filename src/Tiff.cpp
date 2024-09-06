#include <Tiff.h>

#include <iostream>
#include <limits>



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Tiff::Tiff() : _tifin(nullptr), 
 previous_strip(-1), 
 full_strip_data(nullptr),
  num_directories(0), 
  directory_infos(nullptr) {
    TIFFSetErrorHandler(NULL);
    TIFFSetErrorHandlerExt(NULL);
    TIFFSetWarningHandler(NULL);
    TIFFSetWarningHandlerExt(NULL);

}
Tiff::~Tiff() { 
    delete [] full_strip_data;
    close(); 
 }

boolean Tiff::loadFromFile(const char* name) {
        TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
        TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0); // unlimited
        _tifin = TIFFOpenExt(name, "rb", opts);
        TIFFOpenOptionsFree(opts);
         if (_tifin) {
            num_directories = TIFFNumberOfDirectories(_tifin);
            current_directory = 0;
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
        TIFFGetFieldDefaulted(tifin, TIFFTAG_SAMPLEFORMAT, &sample_format);
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
        tile_width = 512;
        image_columns = image_width / tile_width;
        if (image_width % tile_width)
        ++ image_columns;
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
        directory_infos = nullptr;
        TIFFClose(_tifin); 
        _tifin = nullptr;      
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
    case FILETYPE_REDUCEDIMAGE: return "Reduced image"; break;
   case FILETYPE_PAGE: return "Page"; break;
   default: return "Normal image"; break;
    }
    }

    const char *TiffDirectory::sampleFormatDescription()
    {
        switch (sample_format) {
        case SAMPLEFORMAT_UINT:
            return "Unsigned integer"; break;
        case SAMPLEFORMAT_INT:
            return "Signed integer"; break;
        case SAMPLEFORMAT_IEEEFP:
            return "IEEE floating point"; break;
        case SAMPLEFORMAT_COMPLEXINT:
            return "Complex signed integer"; break;
        case SAMPLEFORMAT_COMPLEXIEEEFP:
            return "Complex IEEE floating point"; break;
        case SAMPLEFORMAT_VOID:
        default:
            return "Untyped data"; break;
        }
    }

    unsigned char* Tiff::loadTileOrStrip(int directory, int column, int row) {
         TiffDirectory* di = getDirectoryInfo(directory);
        if (di == nullptr || !di->isValid()) return nullptr;
        if (!setDirectory(directory) || column < 0 || row < 0 || column >= di->image_columns || row >= di->image_rows)
            return nullptr;
        unsigned char* data = new unsigned char[di->tile_width * di->tile_height * 4];
        for(int idx = 0; idx < di->tile_width * di->tile_height * 4;  ++idx) memset(&data[idx], 255, 1);
        if (!data) {
            snprintf(error_buffer, 1024, "Error allocating data for tile %dx%d", column, row);
            return nullptr;
        } 
        boolean ok = false;
        if (di->is_tiled) {
                if (di->sample_format == SAMPLEFORMAT_IEEEFP && di->bits_per_sample == 32) {
                    // TODO use same buffer
                    float* fdata = (float*)data;
                    auto tilenum  = TIFFComputeTile(_tifin, column * di->tile_width, row * di->tile_height, 0, 0);
                    ok = TIFFReadEncodedTile(_tifin, tilenum, fdata, di->tile_width * di->tile_height * 4);
                    if (ok) {
                        float fmin = std::numeric_limits<float>::max();
                        float fmax = std::numeric_limits<float>::min();
                        for (int idx = 0; idx < di->tile_width * di->tile_height; ++idx) {
                            int value = fdata[idx];
                            if (value < fmin) fmin = value;
                            if (value > fmax) fmax = value; 
                        }
                        for (int idx = 0; idx < di->tile_width * di->tile_height; ++idx) {
                            unsigned char value = (unsigned char)(int)(255 * (fdata[idx] - fmin / (fmax - fmin)));
                            data[idx * 4 + 0] = value;
                            data[idx * 4 + 1] = value;
                            data[idx * 4 + 2] = value;
                            data[idx * 4 + 3] = 255; 
                        }
                     }
                } else if (di->sample_format == SAMPLEFORMAT_UINT && di->bits_per_sample == 2) {
                    // TODO use same buffer
                    uint16_t* idata = new uint16_t[di->tile_width * di->tile_height];
                    auto tilenum  = TIFFComputeTile(_tifin, column * di->tile_width, row * di->tile_height, 0, 0);
                    ok = TIFFReadEncodedTile(_tifin, tilenum, idata, di->tile_width * di->tile_height * 4);
                    if (ok) {
                        uint16_t fmin = std::numeric_limits<uint16_t>::max();
                        uint16_t fmax = std::numeric_limits<uint16_t>::min();
                        for (int idx = 0; idx < di->tile_width * di->tile_height; ++idx) {
                            int value = idata[idx];
                            if (value < fmin) fmin = value;
                            if (value > fmax) fmax = value; 
                        }
                        for (int idx = 0; idx < di->tile_width * di->tile_height; ++idx) {
                            unsigned char value = (unsigned char)(int)(255 * (idata[idx] - fmin / (fmax - fmin)));
                            data[idx * 4 + 0] = value;
                            data[idx * 4 + 1] = value;
                            data[idx * 4 + 2] = value;
                            data[idx * 4 + 3] = 255; 
                        }
                        delete [] idata;
                     }
                } else {
                    ok = TIFFReadRGBATile(_tifin, column * di->tile_width, row * di->tile_height, (uint32_t *)data) == 1;       
                    if (ok) stbi__vertical_flip(data, di->tile_width, di->tile_height, 4);
                }
        } else {
            // cache
           if (previous_strip != row) {
                delete [] full_strip_data; 
                full_strip_data = new unsigned char[di->image_width * di->tile_height * 4];
                ok = TIFFReadRGBAStrip(_tifin, row * di->tile_height, (uint32_t *)full_strip_data) == 1;
               if (ok) {
                  stbi__vertical_flip(full_strip_data, di->image_width, di->tile_height, 4);
                }
                previous_strip = row;
            } // copy to data
            unsigned char* start_pos = full_strip_data + column * di->tile_width * 4;
            for (int y = 0; y < di->tile_height; ++y) {
                memcpy(data + di->tile_width * 4 * y, start_pos, di->tile_width * 4);
                start_pos += di->image_width * 4;
            }
        }      
        return data;
    }
 