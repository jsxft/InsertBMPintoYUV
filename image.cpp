#include <thread>
#include <vector>
#include "image.h"



BYTE RGB::get_y() const {
    return (66 * this->red + 129 * this->green + 25 * this->blue >> 8) +  16;
}

BYTE RGB::get_u() const {
    return (-38 * this->red - 74 * this->green + 112 * this->blue >> 8) + 128;
}

BYTE RGB::get_v() const {
    return (112 * this->red - 94 * this->green - 18 * this->blue >> 8) + 128;
}

ImageRGB::ImageRGB(int height, int width) : height(height), width(width) {
    this->buff = new RGB [height * width];
}

ImageRGB::~ImageRGB() {
    delete [] this->buff;
}

RGB* ImageRGB::operator [] (int row) {
    return this->buff + row * width;
}

ImageYUV ImageRGB::to_yuv() {
    ImageYUV image(this->height, this->width);

    for (int row = 0; row < this->height; ++row)
        for (int col = 0; col < this->width; ++col)
            image.set_y(row, col, (*this)[row][col].get_y());

    for (int row = 0; row < this->height; row += 2)
        for (int col = 0; col < this->width; col += 2) {
            BYTE u = (0 + (*this)[row][col].get_u() + (*this)[row + 1][col].get_u()
                    + (*this)[row + 1][col].get_u() + (*this)[row + 1][col + 1].get_u()) / 4;
            BYTE v = (0 + (*this)[row][col].get_v() + (*this)[row + 1][col].get_v()
                    + (*this)[row + 1][col].get_v() + (*this)[row + 1][col + 1].get_v()) / 4;
            image.set_u(row / 2, col / 2, u);
            image.set_v(row / 2, col / 2, v);
        }

    return image;
}

ImageYUV ImageRGB::to_yuv_multithread() {
    int threads_number = std::thread::hardware_concurrency();

    if (threads_number < 2)
        return this->to_yuv();

    ImageYUV image(this->height, this->width);
    std::vector<std::thread> threads;

    static auto rgb_to_yuv = [](ImageRGB &image_rgb, ImageYUV &image_yuv, int thread, int threads_number) {
        for (int row = 0; row < image_rgb.height; ++row)
            for (int col = thread % threads_number; col < image_rgb.width; col += threads_number)
                image_yuv.set_y(row, col, image_rgb[row][col].get_y());

        for (int row = 0; row < image_rgb.height; row += 2)
            for (int col = thread % threads_number; col < image_rgb.width; col += 2 * threads_number) {
                BYTE u = (0 + image_rgb[row][col].get_u() + image_rgb[row + 1][col].get_u()
                          + image_rgb[row + 1][col].get_u() + image_rgb[row + 1][col + 1].get_u()) / 4;
                BYTE v = (0 + image_rgb[row][col].get_v() + image_rgb[row + 1][col].get_v()
                          + image_rgb[row + 1][col].get_v() + image_rgb[row + 1][col + 1].get_v()) / 4;
                image_yuv.set_u(row / 2, col / 2, u);
                image_yuv.set_v(row / 2, col / 2, v);
            }
    };

    threads.reserve(threads_number);
    for (int thread = 0; thread < threads_number; ++thread)
        threads.emplace_back(rgb_to_yuv, std::ref(*this), std::ref(image), thread, threads_number);

    for (int thread = 0; thread < threads_number; ++thread)
        threads[thread].join();

    return image;
}


ImageYUV::ImageYUV(int height, int width) : height(height), width(width) {
    this->buff_y = new BYTE [height * width + (height / 2) * (width / 2) * 2];
    this->buff_u = this->buff_y + height * width;
    this->buff_v = this->buff_u + (height / 2) * (width / 2);
}

ImageYUV::~ImageYUV() {
    delete [] this->buff_y;
}

BYTE* ImageYUV::get_buff() {
    return this->buff_y;
}

int ImageYUV::get_buff_size() const {
    return this->height * this->width + (this->height / 2) * (this->width / 2) * 2;
}

int ImageYUV::get_height() const {
    return this->height;
}

int ImageYUV::get_width() const {
    return this->width;
}

BYTE ImageYUV::get_y(int row, int col) const {
    return this->buff_y[row * this->width + col];
}

BYTE ImageYUV::get_u(int row, int col) const {
    return this->buff_u[row * (this->width / 2) + col];
}

BYTE ImageYUV::get_v(int row, int col) const {
    return this->buff_v[row * (this->width / 2) + col];
}

void ImageYUV::set_y(int row, int col, BYTE val) {
    this->buff_y[row * this->width + col] = val;
}

void ImageYUV::set_u(int row, int col, BYTE val) {
    this->buff_u[row * (this->width / 2) + col] = val;
}

void ImageYUV::set_v(int row, int col, BYTE val) {
    this->buff_v[row * (this->width / 2) + col] = val;
}
