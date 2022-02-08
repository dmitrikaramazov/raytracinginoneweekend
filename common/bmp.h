#pragma pack(push,1)

#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>

#include "rtweekend.h"
#include "vec3.h"


struct BMPFileHeader {
    uint16_t file_type{0x4D42};
    uint32_t file_size{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offset_data{0};
};

struct BMPInfoHeader {
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bit_count{0};
    uint32_t compression{0};
    uint32_t size_image{0};
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
};

struct BMPColorHeader {
    uint32_t red_mask{0x00ff0000};
    uint32_t green_mask{0x0000ff00};
    uint32_t blue_mask{0x000000ff};
    uint32_t alpha_mask{0xff000000};
    uint32_t color_space_type{0x73524742};
    uint32_t unused[16]{0};
};

struct BMP{
    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
    BMPColorHeader bmp_color_header;
    std::vector<uint8_t> data;

    BMP(const char *fname) {
        read(fname);
    }

    void read(const char *fname) {
        std::ifstream inp{ fname, std::ios_base::binary };
        if(inp) {
            inp.read((char*)&file_header, sizeof(file_header));
            if(file_header.file_type != 0x4D42){
                throw std::runtime_error("Error! File Format.");
            }
            inp.read((char*)&bmp_info_header,sizeof(bmp_info_header));
            
            if(bmp_info_header.bit_count == 32){
                if(bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))){
                    inp.read((char*)&bmp_color_header, sizeof(bmp_color_header));
                    check_color_header(bmp_color_header);
                } else {
                    std::cerr << "Warning the file \"" << fname << "\" does not seem to contain bitmask information\n";
                    throw std::runtime_error("Error! File Format.");
                }
            }
            inp.seekg(file_header.offset_data,inp.beg);
            if(bmp_info_header.bit_count == 32){
                bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            } else {
                bmp_info_header.size = sizeof(BMPInfoHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            file_header.file_size = file_header.offset_data;
            if(bmp_info_header.height < 0 ){
                throw std::runtime_error("BMP Origin incorrect");
            }
            data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count/8);
            if(bmp_info_header.width & 4 == 0){
                inp.read((char*)data.data(),data.size());
                file_header.file_size += data.size();
            } else {
                row_stride = bmp_info_header.width * bmp_info_header.bit_count/8;
                uint32_t new_stride = make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - row_stride);
                for(int y = 0; y < bmp_info_header.height; ++y){
                    inp.read((char*)(data.data()+row_stride*y),row_stride);
                    inp.read((char*)padding_row.data(),padding_row.size());
                }
                file_header.file_size += data.size() + bmp_info_header.height * padding_row.size();
            }

        } else {
            throw std::runtime_error("Unable to open input image");
        }
    }

    BMP(int32_t width, int32_t height, bool has_alpha = true){
        if(width <=0 || height <= 0 ){
            throw std::runtime_error("negative dimensions");
        }
        bmp_info_header.width = width;
        bmp_info_header.height = height;
        if(has_alpha){
            bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            bmp_info_header.bit_count = 32;
            bmp_info_header.compression = 3;
            row_stride = width * 4;
            data.resize(row_stride*height);
            file_header.file_size = file_header.offset_data + data.size();
        } else {
            bmp_info_header.size = sizeof(BMPInfoHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            bmp_info_header.bit_count = 24;
            bmp_info_header.compression = 0;
            row_stride = width * 3;
            data.resize(row_stride*height);
            uint32_t new_stride = make_stride_aligned(4);
            file_header.file_size = file_header.offset_data + data.size() + bmp_info_header.height * (new_stride - row_stride);
            
        }
    }

    void write(const char *fname){
        std::ofstream of{ fname, std::ios_base::binary };
        if(of){
            if(bmp_info_header.bit_count == 32){
                write_headers_and_data(of);
            } else if (bmp_info_header.bit_count == 24){
                if(bmp_info_header.width % 4 == 0){
                    write_headers_and_data(of);
                } else {
                    uint32_t new_stride = make_stride_aligned(4);
                    std::vector<uint8_t> padding_row(new_stride = row_stride);
                    write_headers(of);
                    for(int y = 0; y < bmp_info_header.height; ++y){
                        of.write((const char*)(data.data() + row_stride*y),row_stride);
                        of.write((const char*)padding_row.data(), padding_row.size());
                    }
                }
            } else {
                throw std::runtime_error("not 24 or 32 bits per pix");
            }
        } else {
            throw std::runtime_error("unable to open image");
        }
    }

/* 
    void fill_region(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t R, uint8_t G, uint8_t B, uint8_t A){
        if(x0 + w > (uint32_t)bmp_info_header.width || y0 + h > (uint32_t)bmp_info_header.height){
            throw std::runtime_error("The region does not fit in the image");
        }
        uint32_t channels = bmp_info_header.bit_count / 8;
        for(uint32_t y = y0; y < y0 + h; ++y){
            for(uint32_t x = x0; x < x0 + w; ++x){
                    
                data[channels * (y * bmp_info_header.width + x) + 0] = B;
                
                data[channels * (y * bmp_info_header.width + x) + 1] = G;
                
                data[channels * (y * bmp_info_header.width + x) + 2] = R;
                
                if(channels == 4){
                    data[channels * (y * bmp_info_header.width + x) + 3] = A;
                }
            }
        }
    }
 */

    void fill_region(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t A, color pixel_color){
        if(x0 + w > (uint32_t)bmp_info_header.width || y0 + h > (uint32_t)bmp_info_header.height){
            throw std::runtime_error("The region does not fit in the image");
        }
        uint32_t channels = bmp_info_header.bit_count / 8;
        for(uint32_t y = y0; y < y0 + h; ++y){
            for(uint32_t x = x0; x < x0 + w; ++x){
                    
                data[channels * (y * bmp_info_header.width + x) + 2] = static_cast<int>(255.999 * pixel_color.x()); // R
                data[channels * (y * bmp_info_header.width + x) + 1] = static_cast<int>(255.999 * pixel_color.y()); // G
                data[channels * (y * bmp_info_header.width + x) + 0] = static_cast<int>(255.999 * pixel_color.z()); // B
                if(channels == 4){
                    data[channels * (y * bmp_info_header.width + x) + 3] = A; // A
                }
            }
        }
    }

    void write_color(uint32_t x, uint32_t y, color pixel_color, int samples_per_pixel){
        if(x > bmp_info_header.width || y > bmp_info_header.height){
            throw std::runtime_error("Outside image dimensions");
        }
        uint32_t channels = bmp_info_header.bit_count / 8;
        auto r = pixel_color.x();
        auto g = pixel_color.y();
        auto b = pixel_color.z();
        auto c = clamp(1.0,1.0,1.0);
        auto scale = 1.0 / samples_per_pixel;
        // Divide color by number of samples and gamma correct for gamma=2.0
        r = sqrt(scale*r);
        g = sqrt(scale*g);
        b = sqrt(scale*b);
        data[channels * (y * bmp_info_header.width + x) + 2] = static_cast<int>(256 * clamp(r,0.0,0.999)); // R
        data[channels * (y * bmp_info_header.width + x) + 1] = static_cast<int>(256 * clamp(g,0.0,0.999)); // G
        data[channels * (y * bmp_info_header.width + x) + 0] = static_cast<int>(256 * clamp(b,0.0,0.999)); // B
        if(channels == 4){
            data[channels * (y * bmp_info_header.width + x) + 3] = 255; // A
        }
    }

    /* 
    void fill_region(){
        uint32_t x0 = 0;
        uint32_t y0 = 0;
        uint32_t w = 800;
        uint32_t h = 600;
        if(x0 + w > (uint32_t)bmp_info_header.width || y0 + h > (uint32_t)bmp_info_header.height){
            throw std::runtime_error("The region does not fit in the image");
        }
        uint32_t channels = bmp_info_header.bit_count / 8;
        for(uint32_t y = y0; y < y0 + h; ++y){
            for(uint32_t x = x0; x < x0 + w; ++x){

                color pixel_color(double(y)/(bmp_info_header.height - 1), double(x)/(bmp_info_header.width - 1), 0.25);
                uint8_t A = 255;
                uint8_t R = static_cast<uint8_t>(254.999 * pixel_color.x());
                uint8_t G = static_cast<uint8_t>(254.999 * pixel_color.y());
                uint8_t B = static_cast<uint8_t>(254.999 * pixel_color.z());
                data[channels * (y * bmp_info_header.width + x) + 0] = R;
                
                data[channels * (y * bmp_info_header.width + x) + 1] = G;
                
                data[channels * (y * bmp_info_header.width + x) + 2] = B;
                
                if(channels == 4){
                    data[channels * (y * bmp_info_header.width + x) + 3] = A;
                }
            }
        }
    }
     */

    private:

    uint32_t row_stride{0};
    void write_headers(std::ofstream &of){
        of.write((const char*)&file_header, sizeof(file_header));
        of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
        if(bmp_info_header.bit_count == 32){
            of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
        }
    }
    void write_headers_and_data(std::ofstream &of){
        write_headers(of);
        of.write((const char*)data.data(),data.size());
    }
    uint32_t make_stride_aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while(new_stride & align_stride != 0){
            new_stride++;
        }
        return new_stride;
    }

    void check_color_header(BMPColorHeader &bmp_color_header){
        BMPColorHeader expected_color_header;
        if(expected_color_header.red_mask != bmp_color_header.red_mask ||
            expected_color_header.blue_mask != bmp_color_header.blue_mask ||
            expected_color_header.green_mask != bmp_color_header.green_mask ||
            expected_color_header.alpha_mask != bmp_color_header.alpha_mask)
        {
            throw std::runtime_error("unexpected color mask format");
        }
        if(expected_color_header.color_space_type != bmp_color_header.color_space_type){
            throw std::runtime_error("Unexpected color space type");
        }
    }
};

#pragma pack(pop)