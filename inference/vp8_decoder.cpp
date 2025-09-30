// #include "include/vp8_decoder.hpp"
// #include <iostream>

// VP8Decoder::VP8Decoder() : initialized_(false) {
//     memset(&codec_, 0, sizeof(codec_));
// }

// VP8Decoder::~VP8Decoder() {
//     cleanup();
// }

// bool VP8Decoder::initialize() {
//     if (initialized_) {
//         cleanup();
//     }
    
//     vpx_codec_dec_cfg_t cfg;
//     memset(&cfg, 0, sizeof(cfg));
//     cfg.w = 640;  // Default width, will be updated per frame
//     cfg.h = 480;  // Default height
    
//     vpx_codec_err_t res = vpx_codec_dec_init(&codec_, vpx_codec_vp8_dx(), &cfg, 0);
//     if (res != VPX_CODEC_OK) {
//         std::cerr << "Failed to initialize VP8 decoder: " << vpx_codec_error(&codec_) << std::endl;
//         return false;
//     }
    
//     initialized_ = true;
//     std::cout << "VP8 decoder initialized successfully" << std::endl;
//     return true;
// }

// cv::Mat VP8Decoder::decodeFrame(const uint8_t* vp8_data, size_t data_size, int width, int height) {
//     if (!initialized_ && !initialize()) {
//         return cv::Mat();
//     }
    
//     // Decode the VP8 frame
//     vpx_codec_err_t res = vpx_codec_decode(&codec_, vp8_data, data_size, nullptr, 0);
//     if (res != VPX_CODEC_OK) {
//         std::cerr << "Failed to decode VP8 frame: " << vpx_codec_error(&codec_) << std::endl;
//         return cv::Mat();
//     }
    
//     // Get the decoded frame
//     vpx_codec_iter_t iter = nullptr;
//     vpx_image_t* img = vpx_codec_get_frame(&codec_, &iter);
//     if (!img) {
//         std::cerr << "Failed to get decoded frame" << std::endl;
//         return cv::Mat();
//     }
    
//     // Convert VPX image to OpenCV Mat (grayscale)
//     cv::Mat decoded_frame;
//     if (img->fmt == VPX_IMG_FMT_I420) {
//         // I420 (YUV 4:2:0) to grayscale (we only need the Y plane)
//         cv::Mat y_plane(img->d_h, img->d_w, CV_8UC1, img->planes[0], img->stride[0]);
//         decoded_frame = y_plane.clone();
//     } else {
//         std::cerr << "Unsupported VPX image format: " << img->fmt << std::endl;
//         return cv::Mat();
//     }
    
//     return decoded_frame;
// }

// void VP8Decoder::cleanup() {
//     if (initialized_) {
//         vpx_codec_destroy(&codec_);
//         initialized_ = false;
//     }
// }