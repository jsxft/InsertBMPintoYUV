#include <cstring>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <x86intrin.h>
#include "image.h"


ImageRGB::ImageRGB(int height, int width) : height(height), width(width) {
    buff = new BYTE [3 * height * width];
}

ImageRGB::~ImageRGB() {
    delete [] buff;
}

BYTE * ImageRGB::get_buff(int row) {
    return buff + row * 3 * width;
}

int ImageRGB::get_height() const {
    return height;
}

int ImageRGB::get_width() const {
    return width;
}

void ImageRGB::rgb_to_yuv(BYTE r, BYTE g, BYTE b, BYTE &y, BYTE &u, BYTE &v) {
    y = (( 66 * r + 129 * g +  25 * b) >> 8) +  16;
    u = ((-38 * r -  74 * g + 112 * b) >> 8) + 128;
    v = ((112 * r -  94 * g -  18 * b) >> 8) + 128;
}

void ImageRGB::to_yuv444(ImageYUV444 *res) {
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width ; ++col) {
            int i = row * width + col;
            rgb_to_yuv(buff[3 * i + 2], buff[3 * i + 1], buff[3 * i],
                       res->buff_y[i], res->buff_u[i], res->buff_v[i]);
        }
}

void ImageRGB::to_yuv444_mt_loc(ImageRGB *src, ImageYUV444 *res, int thread, int threads_number) {
    for (int row = 0; row < src->height; ++row)
        for (int col = thread; col < src->width; col += threads_number) {
            int i = row * src->width + col;
            rgb_to_yuv(src->buff[3 * i + 2], src->buff[3 * i + 1], src->buff[3 * i],
                       res->buff_y[i], res->buff_u[i], res->buff_v[i]);
        }
}

void ImageRGB::to_yuv444_mt(ImageYUV444 *res) {
    int threads_number = (int) std::thread::hardware_concurrency();

    if (threads_number < 2)
        return to_yuv444(res);

    std::vector<std::thread> threads;
    threads.reserve(threads_number);

    for (int thread = 0; thread < threads_number; ++thread)
        threads.emplace_back(to_yuv444_mt_loc, this, res, thread, threads_number);

    for (int thread = 0; thread < threads_number; ++thread)
        threads[thread].join();
}

void ImageRGB::to_yuv444_simd(ImageYUV444 *res) {
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width - 15; col += 16) {
            __m128i mask = _mm_setr_epi8(
                    0x02, 0x05, 0x08, 0x0b,
                    0x01, 0x04, 0x07, 0x0a,
                    0x00, 0x03, 0x06, 0x09,
                    -1, -1, -1, -1);
            __m128i clr[4], tmp[4];

            for (int i = 0; i < 4; ++i) {
                clr[i] = _mm_loadu_si128((__m128i *) (buff + 3 * row * width + 3 * col + i * 12));
                clr[i] = _mm_shuffle_epi8(clr[i], mask);
            }

            tmp[0] = _mm_unpacklo_epi32(clr[0], clr[1]);
            tmp[1] = _mm_unpacklo_epi32(clr[2], clr[3]);
            tmp[2] = _mm_unpackhi_epi32(clr[0], clr[1]);
            tmp[3] = _mm_unpackhi_epi32(clr[2], clr[3]);
            clr[0] = _mm_unpacklo_epi64(tmp[0], tmp[1]);
            clr[1] = _mm_unpackhi_epi64(tmp[0], tmp[1]);
            clr[2] = _mm_unpacklo_epi64(tmp[2], tmp[3]);

            __m128i v_lo[3], v_hi[3];
            for (int i = 0; i < 3; ++i) {
                v_lo[i] = _mm_unpacklo_epi8(clr[i], _mm_setzero_si128());
                v_hi[i] = _mm_unpackhi_epi8(clr[i], _mm_setzero_si128());
            }

            short m[12] = {
                    66, 129, 25, 4096,
                    -38, -74, 112, -32768,
                    112, -94, -18, -32768
            };
            __m128i yuv[3];
            for (int i = 0; i < 3; ++i) {
                __m128i yuv_lo, yuv_hi;
                yuv_lo = _mm_add_epi16(_mm_add_epi16(
                        _mm_mullo_epi16(v_lo[0], _mm_set1_epi16(m[i * 4])),
                        _mm_mullo_epi16(v_lo[1], _mm_set1_epi16(m[i * 4 + 1]))),
                                       _mm_mullo_epi16(v_lo[2], _mm_set1_epi16(m[i * 4 + 2])));
                yuv_lo = _mm_add_epi16(yuv_lo, _mm_set1_epi16(m[i * 4 + 3]));
                yuv_lo = _mm_srli_epi16(yuv_lo, 8);

                yuv_hi = _mm_add_epi16(_mm_add_epi16(
                        _mm_mullo_epi16(v_hi[0], _mm_set1_epi16(m[i * 4])),
                        _mm_mullo_epi16(v_hi[1], _mm_set1_epi16(m[i * 4 + 1]))),
                                       _mm_mullo_epi16(v_hi[2], _mm_set1_epi16(m[i * 4 + 2])));
                yuv_hi = _mm_add_epi16(yuv_hi, _mm_set1_epi16(m[i * 4 + 3]));
                yuv_hi = _mm_srli_epi16(yuv_hi, 8);

                yuv[i] = _mm_packus_epi16(yuv_lo, yuv_hi);
            }

            uint8_t y[16], u[16], v[16];
            _mm_storeu_si128((__m128i *) y, yuv[0]);
            _mm_storeu_si128((__m128i *) u, yuv[1]);
            _mm_storeu_si128((__m128i *) v, yuv[2]);

            for (int i = 0; i < 16; ++i) {
                memcpy(res->buff_y + row * width + col, y, 16);
                memcpy(res->buff_u + row * width + col, u, 16);
                memcpy(res->buff_v + row * width + col, v, 16);
            }
        }

        for (int col = width & (-16); col < width; col++) {
            int i = row * width + col;
            rgb_to_yuv(buff[3 * i + 2], buff[3 * i + 1], buff[3 * i],
                       res->buff_y[i], res->buff_u[i], res->buff_v[i]);
        }
    }
}


ImageYUV444::ImageYUV444(int height, int width) : height(height), width(width) {
    buff_y = new BYTE [3 * height * width];
    buff_u = buff_y + height * width;
    buff_v = buff_u + height * width;
}

ImageYUV444::~ImageYUV444() {
    delete [] buff_y;
}

void ImageYUV444::to_yuv420(ImageYUV420 *res) {
    memcpy(res->buff_y, buff_y, height * width);

    for (int row = 0; row < height; row += 2)
        for (int col = 0; col < width; col += 2) {
            int i = row * width / 4 + col / 2;      // = row/2 * width/2 + col/2
            int j1 = row * width + col;
            int j2 = (row + 1) * width + col;

            res->buff_u[i] = buff_u[j1]/4 + buff_u[j1+1]/4 + buff_u[j2]/4 + buff_u[j2+1]/4;
            res->buff_v[i] = buff_v[j1]/4 + buff_v[j1+1]/4 + buff_v[j2]/4 + buff_v[j2+1]/4;
        }
}

void ImageYUV444::to_yuv420_mt_loc(ImageYUV444 *src, ImageYUV420 *res, int thread, int threads_number) {
    if (thread == 0) {
        memcpy(res->buff_y, src->buff_y, src->height * src->width);
        return;
    }

    thread--;
    threads_number--;

    for (int row = 0; row < src->height; row += 2)
        for (int col = thread * 2; col < src->width; col += 2 * threads_number) {
            int i = row * src->width / 4 + col / 2;      // = row/2 * width/2 + col/2
            int j1 = row * src->width + col;
            int j2 = (row + 1) * src->width + col;

            res->buff_u[i] = src->buff_u[j1]/4 + src->buff_u[j1+1]/4 + src->buff_u[j2]/4 + src->buff_u[j2+1]/4;
            res->buff_v[i] = src->buff_v[j1]/4 + src->buff_v[j1+1]/4 + src->buff_v[j2]/4 + src->buff_v[j2+1]/4;
        }
}

void ImageYUV444::to_yuv420_mt(ImageYUV420 *res) {
    int threads_number = (int) std::thread::hardware_concurrency();

    if (threads_number < 2)
        return to_yuv420(res);

    std::vector<std::thread> threads;
    threads.reserve(threads_number);

    for (int thread = 0; thread < threads_number; ++thread)
        threads.emplace_back(to_yuv420_mt_loc, this, res, thread, threads_number);

    for (int thread = 0; thread < threads_number; ++thread)
        threads[thread].join();
}


ImageYUV420::ImageYUV420(int height, int width) : height(height), width(width) {
    buff_y = new BYTE [3 * height * width / 2];
    buff_u = buff_y + height * width;
    buff_v = buff_u + height * width / 4;
}

ImageYUV420::~ImageYUV420() {
    delete [] buff_y;
}

BYTE * ImageYUV420::get_buff() {
    return buff_y;
}

int ImageYUV420::get_buff_size() const {
    return 3 * height * width / 2;
}

void ImageYUV420::insert(ImageYUV420 *image, int x, int y) {
    assert(x % 2 == 0);
    assert(y % 2 == 0);

    assert(width >= image->width + x);
    assert(height >= image->height + y);

    for (int row = 0; row < image->height; ++row) {
        memcpy(buff_y + (row + y) * width + x, image->buff_y + row * image->width, image->width);
    }
    
    for (int row = 0; row < image->height / 2; ++row) {
        memcpy(buff_u + (row + y / 2) * width / 2 + x / 2, image->buff_u + row * image->width / 2, image->width / 2);
        memcpy(buff_v + (row + y / 2) * width / 2 + x / 2, image->buff_v + row * image->width / 2, image->width / 2);
    }
}
