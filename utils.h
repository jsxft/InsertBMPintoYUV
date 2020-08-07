#ifndef INSERTBMPINTOYUV_UTILS_H
#define INSERTBMPINTOYUV_UTILS_H


#include "image.h"

typedef uint8_t     BYTE;
typedef uint16_t    WORD;
typedef uint32_t    DWORD;
typedef int32_t     LONG;

#pragma pack(push, r1, 1)
typedef struct BITMAPFILEHEADER
{
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER
{
    DWORD   biSize;
    LONG    biWidth;
    LONG    biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    LONG    biXPelsPerMeter;
    LONG    biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop, r1)


namespace utils {
    void print_headers(BITMAPFILEHEADER &bfh, BITMAPINFOHEADER &bih);
    ImageRGB * read_bmp(const std::string &path);

    void insert_bmp_into_yuv(const std::string &path_image, const std::string &path_src, const std::string &path_dst,
                             int height, int width, int frames);
    void insert_bmp_into_yuv_multithread(const std::string &path_image, const std::string &path_src,
                                         const std::string &path_dst, int height, int width, int frames);
    void insert_bmp_into_yuv_simd(const std::string &path_image, const std::string &path_src,
                                  const std::string &path_dst, int height, int width, int frames);

    void compare_convert_time_rgb_to_yuv(const std::string &path_image, int repeat_number);
};


#endif //INSERTBMPINTOYUV_UTILS_H
