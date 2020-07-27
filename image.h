#ifndef INSERTBMPINTOYUV_IMAGE_H
#define INSERTBMPINTOYUV_IMAGE_H


#include <cstdint>
#include <fstream>

typedef uint8_t     BYTE;

#pragma pack(push, r1, 1)
typedef struct RGB {
    BYTE    blue;
    BYTE    green;
    BYTE    red;

    BYTE    get_y() const;
    BYTE    get_u() const;
    BYTE    get_v() const;
} RGB;
#pragma pack(pop, r1)

class ImageYUV;


class ImageRGB {
private:
    int     height, width;
    RGB     *buff;

public:
    ImageRGB(int height, int width);
    ~ImageRGB();

    RGB* operator [] (int row);

    ImageYUV to_yuv();
    ImageYUV to_yuv_multithread();
    ImageYUV to_yuv_simd();
};


class ImageYUV {
private:
    int     height, width;
    BYTE    *buff_y, *buff_u, *buff_v;

public:
    ImageYUV(int height, int width);
    ~ImageYUV();

    BYTE* get_buff();
    int get_buff_size() const;

    int get_height() const;
    int get_width() const;

    BYTE get_y(int row, int col) const;
    BYTE get_u(int row, int col) const;
    BYTE get_v(int row, int col) const;

    void set_y(int row, int col, BYTE val);
    void set_u(int row, int col, BYTE val);
    void set_v(int row, int col, BYTE val);

    void add_u(int row, int col, BYTE val);
    void add_v(int row, int col, BYTE val);
};


#endif //INSERTBMPINTOYUV_IMAGE_H
