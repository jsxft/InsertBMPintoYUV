#include <cassert>
#include <iostream>
#include <thread>
#include <fstream>
#include "utils.h"


static std::string to_string(WORD x) {
    std::string res;
    res += (char)x;
    res += (char)(x >> 8);
    return res;
}

void utils::print_headers(BITMAPFILEHEADER &bfh, BITMAPINFOHEADER &bih) {
    std::cout << "### BITMAPFILEHEADER ###" << std::endl;
    std::cout << "bfType: "      << to_string(bfh.bfType) << std::endl;
    std::cout << "bfSize: "      << bfh.bfSize            << std::endl;
    std::cout << "bfReserved1: " << bfh.bfReserved1       << std::endl;
    std::cout << "bfReserved2: " << bfh.bfReserved2       << std::endl;
    std::cout << "bfOffBits: "   << bfh.bfOffBits         << std::endl;
    std::cout << std::endl;

    std::cout << "### BITMAPINFOHEADER ###" << std::endl;
    std::cout << "biSize: "          << bih.biSize          << std::endl;
    std::cout << "biWidth: "         << bih.biWidth         << std::endl;
    std::cout << "biHeight: "        << bih.biHeight        << std::endl;
    std::cout << "biPlanes: "        << bih.biPlanes        << std::endl;
    std::cout << "biBitCount: "      << bih.biBitCount      << std::endl;
    std::cout << "biCompression: "   << bih.biCompression   << std::endl;
    std::cout << "biSizeImage: "     << bih.biSizeImage     << std::endl;
    std::cout << "biXPelsPerMeter: " << bih.biXPelsPerMeter << std::endl;
    std::cout << "biYPelsPerMeter: " << bih.biYPelsPerMeter << std::endl;
    std::cout << "biClrUsed: "       << bih.biClrUsed       << std::endl;
    std::cout << "biClrImportant: "  << bih.biClrImportant  << std::endl;
}

ImageRGB * utils::read_bmp(const std::string &path) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    std::fstream fh(path, std::fstream::in | std::fstream::binary);
    fh.read((char*)&bfh, sizeof(bfh));
    fh.read((char*)&bih, sizeof(bih));

    // print_headers(bfh, bih);

    assert(bfh.bfType == 0x4D42);
    assert(bfh.bfReserved1 == 0);
    assert(bfh.bfReserved2 == 0);

    assert(bih.biPlanes == 1);
    assert(bih.biBitCount == 24);
    assert(bih.biCompression == 0);

    fh.seekg(bfh.bfOffBits, fh.beg);
    auto image = new ImageRGB(bih.biHeight, bih.biWidth);

    for (int row = bih.biHeight - 1; row >= 0; --row) {
        fh.read((char *) image->get_buff(row), 3 * bih.biWidth);
        fh.seekg(bih.biWidth % 4, fh.cur);
    }

    fh.close();

    return image;
}

void utils::insert_bmp_into_yuv(const std::string &path_image, const std::string &path_src, const std::string &path_dst,
                                int height, int width, int frames, int x, int y) {
    auto image_rgb      = read_bmp(path_image);
    auto image_yuv444   = new ImageYUV444(image_rgb->get_height(), image_rgb->get_width());
    auto image_yuv420   = new ImageYUV420(image_rgb->get_height(), image_rgb->get_width());
    auto frame          = new ImageYUV420(height, width);

    image_rgb   ->to_yuv444(image_yuv444);
    image_yuv444->to_yuv420(image_yuv420);

    delete image_rgb;
    delete image_yuv444;

    std::fstream file_src(path_src, std::fstream::in | std::fstream::binary);
    std::fstream file_dst(path_dst, std::fstream::out | std::fstream::binary);

    for (int frame_num = 0; frame_num < frames; ++frame_num) {
        file_src.read((char *)frame->get_buff(), frame->get_buff_size());
        frame->insert(image_yuv420, x, y);
        file_dst.write((char *)frame->get_buff(), frame->get_buff_size());
    }

    file_src.close();
    file_dst.close();

    delete image_yuv420;
    delete frame;
}

void utils::insert_bmp_into_yuv_multithread(const std::string &path_image, const std::string &path_src, const std::string &path_dst,
                                int height, int width, int frames, int x, int y) {
    auto image_rgb      = read_bmp(path_image);
    auto image_yuv444   = new ImageYUV444(image_rgb->get_height(), image_rgb->get_width());
    auto image_yuv420   = new ImageYUV420(image_rgb->get_height(), image_rgb->get_width());
    auto frame          = new ImageYUV420(height, width);

    image_rgb   ->to_yuv444_mt(image_yuv444);
    image_yuv444->to_yuv420_mt(image_yuv420);

    delete image_rgb;
    delete image_yuv444;

    std::fstream file_src(path_src, std::fstream::in | std::fstream::binary);
    std::fstream file_dst(path_dst, std::fstream::out | std::fstream::binary);

    for (int frame_num = 0; frame_num < frames; ++frame_num) {
        file_src.read((char *)frame->get_buff(), frame->get_buff_size());
        frame->insert(image_yuv420, x, y);
        file_dst.write((char *)frame->get_buff(), frame->get_buff_size());
    }

    file_src.close();
    file_dst.close();

    delete image_yuv420;
    delete frame;
}
void utils::insert_bmp_into_yuv_simd(const std::string &path_image, const std::string &path_src, const std::string &path_dst,
                                int height, int width, int frames, int x, int y) {
    auto image_rgb      = read_bmp(path_image);
    auto image_yuv444   = new ImageYUV444(image_rgb->get_height(), image_rgb->get_width());
    auto image_yuv420   = new ImageYUV420(image_rgb->get_height(), image_rgb->get_width());
    auto frame          = new ImageYUV420(height, width);

    image_rgb   ->to_yuv444_simd(image_yuv444);
    image_yuv444->to_yuv420(image_yuv420);

    delete image_rgb;
    delete image_yuv444;

    std::fstream file_src(path_src, std::fstream::in | std::fstream::binary);
    std::fstream file_dst(path_dst, std::fstream::out | std::fstream::binary);

    for (int frame_num = 0; frame_num < frames; ++frame_num) {
        file_src.read((char *)frame->get_buff(), frame->get_buff_size());
        frame->insert(image_yuv420, x, y);
        file_dst.write((char *)frame->get_buff(), frame->get_buff_size());
    }

    file_src.close();
    file_dst.close();

    delete image_yuv420;
    delete frame;
}


void utils::compare_convert_time_rgb_to_yuv(const std::string &path_image, int repeat_number) {
    auto image_rgb = read_bmp(path_image);
    auto image_yuv444   = new ImageYUV444(image_rgb->get_height(), image_rgb->get_width());
    auto image_yuv420   = new ImageYUV420(image_rgb->get_height(), image_rgb->get_width());

    uint64_t total_time = 0, max_time = 0;

    for (int i = 0; i < repeat_number; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        image_rgb   ->to_yuv444(image_yuv444);
        image_yuv444->to_yuv420(image_yuv420);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds> (stop - start);

        total_time += duration.count();
        if (max_time < duration.count())
            max_time = duration.count();
    }

    std::cout << "Simple:      ";
    std::cout << "average time = " << total_time / 1000.0 / repeat_number << " ms, ";
    std::cout << "max time = " << max_time / 1000.0 << " ms" << std::endl;

    total_time = 0;
    max_time = 0;

    for (int i = 0; i < repeat_number; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        image_rgb   ->to_yuv444_mt(image_yuv444);
        image_yuv444->to_yuv420_mt(image_yuv420);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds> (stop - start);

        total_time += duration.count();
        if (max_time < duration.count())
            max_time = duration.count();
    }

    std::cout << "Multithread: ";
    std::cout << "average time = " << total_time / 1000.0 / repeat_number << " ms, ";
    std::cout << "max time = " << max_time / 1000.0 << " ms" << std::endl;

    total_time = 0;
    max_time = 0;

    for (int i = 0; i < repeat_number; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        image_rgb   ->to_yuv444_simd(image_yuv444);
        image_yuv444->to_yuv420(image_yuv420);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds> (stop - start);

        total_time += duration.count();
        if (max_time < duration.count())
            max_time = duration.count();
    }

    std::cout << "SIMD:        ";
    std::cout << "average time = " << total_time / 1000.0 / repeat_number << " ms, ";
    std::cout << "max time = " << max_time / 1000.0 << " ms" << std::endl;

    delete image_rgb;
    delete image_yuv420;
    delete image_yuv444;
}
