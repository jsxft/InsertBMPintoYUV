#ifndef INSERTBMPINTOYUV_IMAGE_H
#define INSERTBMPINTOYUV_IMAGE_H


#include <cstdint>

typedef uint8_t     BYTE;
class ImageYUV444;
class ImageYUV420;


class ImageRGB {
private:
    BYTE    *buff;
    int     height, width;

    static void rgb_to_yuv(BYTE r, BYTE g, BYTE b, BYTE &y, BYTE &u, BYTE &v);
    static void to_yuv444_mt_loc(ImageRGB *src, ImageYUV444 *res, int thread, int threads_number);

public:
    ImageRGB(int height, int width);
    ~ImageRGB();

    BYTE * get_buff(int row);
    int get_height() const;
    int get_width() const;

    void to_yuv444(ImageYUV444 *res);
    void to_yuv444_mt(ImageYUV444 *res);
    void to_yuv444_simd(ImageYUV444 *res);
};


class ImageYUV444 {
    friend ImageRGB;

private:
    BYTE    *buff_y, *buff_u, *buff_v;
    int     height, width;

    static void to_yuv420_mt_loc(ImageYUV444 *src, ImageYUV420 *res, int thread, int threads_number);

public:
    ImageYUV444(int height, int width);
    ~ImageYUV444();

    void to_yuv420(ImageYUV420 *res);
    void to_yuv420_mt(ImageYUV420 *res);
};


class ImageYUV420 {
    friend ImageYUV444;

private:
    BYTE    *buff_y, *buff_u, *buff_v;
    int     height, width;

public:
    ImageYUV420(int height, int width);
    ~ImageYUV420();

    BYTE * get_buff();
    int get_buff_size() const;

    void insert(ImageYUV420 *image, int x = 0, int y = 0);
};


#endif //INSERTBMPINTOYUV_IMAGE_H
