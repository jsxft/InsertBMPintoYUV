#include <thread>
#include <vector>
#include <x86intrin.h>
#include "image.h"


BYTE RGB::get_y() const {
    return ( 66 * red + 129 * green +  25 * blue >> 8) +  16;
}

BYTE RGB::get_u() const {
    return (-38 * red -  74 * green + 112 * blue >> 8) + 128;
}

BYTE RGB::get_v() const {
    return (112 * red -  94 * green -  18 * blue >> 8) + 128;
}

ImageRGB::ImageRGB(int height, int width) : height(height), width(width) {
    buff = new RGB [height * width];
}

ImageRGB::~ImageRGB() {
    delete [] buff;
}

RGB* ImageRGB::operator [] (int row) {
    return buff + row * width;
}

ImageYUV * ImageRGB::to_yuv() {
    auto image = new ImageYUV(height, width);

    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) {
            image->set_y(row, col, (*this)[row][col].get_y());
            image->add_u(row / 2, col / 2, (*this)[row][col].get_u() / 4);
            image->add_v(row / 2, col / 2, (*this)[row][col].get_v() / 4);
        }

    return image;
}

ImageYUV * ImageRGB::to_yuv_multithread() {
    int threads_number = std::thread::hardware_concurrency();

    if (threads_number < 2)
        return to_yuv();

    auto image = new ImageYUV(height, width);
    std::vector<std::thread> threads;

    static auto rgb_to_yuv = [this, &image, threads_number](int thread) {
        for (int row = 0; row < height; ++row)
            for (int col = thread % threads_number; col < width; col += threads_number) {
                image->set_y(row, col, (*this)[row][col].get_y());
                image->add_u(row / 2, col / 2, (*this)[row][col].get_u() / 4);
                image->add_v(row / 2, col / 2, (*this)[row][col].get_v() / 4);
            }
    };

    threads.reserve(threads_number);
    for (int thread = 0; thread < threads_number; ++thread)
        threads.emplace_back(rgb_to_yuv, thread);

    for (int thread = 0; thread < threads_number; ++thread)
        threads[thread].join();

    return image;
}

ImageYUV * ImageRGB::to_yuv_simd() {
    auto image = new ImageYUV(height, width);

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width - 15; col += 16) {
            __m128i mask = _mm_setr_epi8(
                    0x02, 0x05, 0x08, 0x0b,
                    0x01, 0x04, 0x07, 0x0a,
                    0x00, 0x03, 0x06, 0x09,
                    -1, -1, -1, -1);
            __m128i clr[4], tmp[4];

            for (int i = 0; i < 4; ++i) {
                clr[i] = _mm_loadu_si128((__m128i *) &((*this)[row][col + i * 4]));
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
                image->set_y(row, col + i, y[i]);
                image->add_u(row / 2, (col + i) / 2, u[i] / 4);
                image->add_v(row / 2, (col + i) / 2, v[i] / 4);
            }
        }

        for (int col = width & (-16); col < width; col++) {
            image->set_y(row, col, (*this)[row][col].get_y());
            image->add_u(row / 2, col / 2, (*this)[row][col].get_u() / 4);
            image->add_v(row / 2, col / 2, (*this)[row][col].get_v() / 4);
        }
    }

    return image;
}


ImageYUV::ImageYUV(int height, int width) : height(height), width(width) {
    buff_y = new BYTE [3 * height * width / 2];
    buff_u = buff_y + height * width;
    buff_v = buff_u + height * width / 4;
}

ImageYUV::~ImageYUV() {
    delete [] buff_y;
}

BYTE* ImageYUV::get_buff() {
    return buff_y;
}

int ImageYUV::get_buff_size() const {
    return 3 * height * width / 2;
}

int ImageYUV::get_height() const {
    return height;
}

int ImageYUV::get_width() const {
    return width;
}

BYTE ImageYUV::get_y(int row, int col) const {
    return buff_y[row * width + col];
}

BYTE ImageYUV::get_u(int row, int col) const {
    return buff_u[row * (width / 2) + col];
}

BYTE ImageYUV::get_v(int row, int col) const {
    return buff_v[row * (width / 2) + col];
}

void ImageYUV::set_y(int row, int col, BYTE val) {
    buff_y[row * width + col] = val;
}

void ImageYUV::set_u(int row, int col, BYTE val) {
    buff_u[row * (width / 2) + col] = val;
}

void ImageYUV::set_v(int row, int col, BYTE val) {
    buff_v[row * (width / 2) + col] = val;
}

void ImageYUV::add_u(int row, int col, BYTE val) {
    buff_u[row * (width / 2) + col] += val;
}

void ImageYUV::add_v(int row, int col, BYTE val) {
    buff_v[row * (width / 2) + col] += val;
}
