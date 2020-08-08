#include <iostream>
#include "utils.h"


int main(int argc, char* argv[]) {
    if (argc < 7) {
        std::cout << "Using: " << argv[0] << " path_img path_src path_out width height frames_num [x y]" << std::endl;
        std::cout << "\tpath_img   - path to image (.bmp)" << std::endl;
        std::cout << "\tpath_src   - path to video (.yuv)" << std::endl;
        std::cout << "\tpath_out   - path to output file (.yuv)" << std::endl;
        std::cout << "\twidth      - width of video" << std::endl;
        std::cout << "\theight     - height of video" << std::endl;
        std::cout << "\tframes_num - number of frames in video" << std::endl;
        std::cout << "\tx, y       - position to insert (default x=0, y=0)" << std::endl;
        return 0;
    }
    std::string path_img(argv[1]);
    std::string path_src(argv[2]);
    std::string path_out(argv[3]);
    int width      = atoi(argv[4]);
    int height     = atoi(argv[5]);
    int frames_num = atoi(argv[6]);
    int x = 0, y = 0;
    if (argc >= 9) {
        x = atoi(argv[7]);
        y = atoi(argv[8]);
    }

//    utils::insert_bmp_into_yuv(path_img, path_src, path_out, height, width, frames_num, x, y);
//    utils::insert_bmp_into_yuv_multithread(path_img, path_src, path_out, height, width, frames_num, x, y);
//    utils::insert_bmp_into_yuv_simd(path_img, path_src, path_out, height, width, frames_num, x, y);

    utils::compare_convert_time_rgb_to_yuv(path_img, 10000);

    return 0;
}
