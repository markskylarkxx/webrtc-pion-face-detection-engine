// #ifndef VP8_DECODER_HPP
// #define VP8_DECODER_HPP

// #include <opencv2/opencv.hpp>
// #include <memory>
// #include <iostream>

// // Forward declarations for libvpx
// struct vpx_codec_ctx;
// struct vpx_image;

// class VP8Decoder {
// public:
//     VP8Decoder();
//     ~VP8Decoder();
    
//     bool initialize();
//     cv::Mat decodeFrame(const uint8_t* data, size_t data_size, int expected_width = 0, int expected_height = 0);
//     void cleanup();

// private:
//     std::unique_ptr<vpx_codec_ctx> codec_ctx_;
//     bool initialized_;
//     int frame_count_;
    
//     // Helper methods
//     bool isValidVP8Frame(const uint8_t* data, size_t size);
//     cv::Mat convertVpxImageToMat(const vpx_image* img);
//     void extractVP8Dimensions(const uint8_t* data, size_t size, int& width, int& height);
// };

// // Implementation
// #include <vpx/vpx_decoder.h>
// #include <vpx/vp8dx.h>

// VP8Decoder::VP8Decoder() : initialized_(false), frame_count_(0) {
//     codec_ctx_ = std::make_unique<vpx_codec_ctx>();
// }

// VP8Decoder::~VP8Decoder() {
//     cleanup();
// }

// bool VP8Decoder::initialize() {
//     if (initialized_) {
//         return true;
//     }
    
//     // Initialize VP8 decoder
//     vpx_codec_iface_t* iface = vpx_codec_vp8_dx();
//     if (!iface) {
//         std::cerr << "ERROR: VP8 decoder interface not available" << std::endl;
//         return false;
//     }
    
//     vpx_codec_dec_cfg_t cfg;
//     memset(&cfg, 0, sizeof(cfg));
//     cfg.threads = 1;  // Single threaded for simplicity
//     cfg.w = 0;       // Width will be determined from bitstream
//     cfg.h = 0;       // Height will be determined from bitstream
    
//     vpx_codec_err_t err = vpx_codec_dec_init(codec_ctx_.get(), iface, &cfg, 0);
//     if (err != VPX_CODEC_OK) {
//         std::cerr << "ERROR: Failed to initialize VP8 decoder: " 
//                   << vpx_codec_error(codec_ctx_.get()) << std::endl;
//         return false;
//     }
    
//     initialized_ = true;
//     std::cout << "VP8 decoder initialized successfully" << std::endl;
//     return true;
// }

// void VP8Decoder::cleanup() {
//     if (initialized_ && codec_ctx_) {
//         vpx_codec_destroy(codec_ctx_.get());
//         initialized_ = false;
//     }
// }

// bool VP8Decoder::isValidVP8Frame(const uint8_t* data, size_t size) {
//     if (!data || size < 3) {
//         return false;
//     }
    
//     // Basic VP8 frame validation
//     // Check for reasonable frame size
//     if (size > 1024 * 1024 * 2) {  // Max 2MB frame
//         return false;
//     }
    
//     // Check if first few bytes look like VP8
//     // This is a heuristic check
//     uint8_t first_byte = data[0];
    
//     // VP8 frames should have specific patterns in first byte
//     bool is_keyframe = (first_byte & 0x01) == 0;
//     uint8_t version = (first_byte >> 1) & 0x07;
//     bool show_frame = (first_byte >> 4) & 0x01;
    
//     // Version should be 0-3 for valid VP8
//     if (version > 3) {
//         return false;
//     }
    
//     // For keyframes, check additional markers
//     if (is_keyframe && size >= 10) {
//         // Check for VP8 sync code in keyframes
//         if (size >= 3) {
//             uint32_t sync_code = (data[0]) | (data[1] << 8) | (data[2] << 16);
//             // Look for patterns that indicate valid VP8 keyframe
//             if ((sync_code & 0xFFFF) == 0x019D) {  // Common VP8 pattern
//                 return true;
//             }
//         }
//     }
    
//     // Additional validation for any VP8 frame
//     return true;  // Accept if basic checks pass
// }

// void VP8Decoder::extractVP8Dimensions(const uint8_t* data, size_t size, int& width, int& height) {
//     width = 0;
//     height = 0;
    
//     if (!data || size < 10) {
//         return;
//     }
    
//     // Check if this is a keyframe
//     if ((data[0] & 0x01) == 0) {  // Keyframe
//         // Skip the frame tag (3 bytes) and look for dimensions
//         if (size >= 10) {
//             // VP8 keyframe format: width and height are at specific offsets
//             width = data[6] | (data[7] << 8);
//             height = data[8] | (data[9] << 8);
            
//             // Mask out reserved bits
//             width &= 0x3FFF;
//             height &= 0x3FFF;
//         }
//     }
// }

// cv::Mat VP8Decoder::decodeFrame(const uint8_t* data, size_t data_size, int expected_width, int expected_height) {
//     if (!initialized_) {
//         std::cerr << "VP8 decoder not initialized" << std::endl;
//         return cv::Mat();
//     }
    
//     if (!data || data_size == 0) {
//         std::cerr << "Invalid input data for VP8 decoding" << std::endl;
//         return cv::Mat();
//     }
    
//     frame_count_++;
    
//     // Validate the frame data
//     if (!isValidVP8Frame(data, data_size)) {
//         if (frame_count_ % 10 == 1) {  // Log occasionally
//             std::cerr << "Invalid VP8 frame data (frame " << frame_count_ << ")" << std::endl;
//         }
//         return cv::Mat();
//     }
    
//     // Extract dimensions if it's a keyframe
//     int frame_width = 0, frame_height = 0;
//     extractVP8Dimensions(data, data_size, frame_width, frame_height);
    
//     // Use expected dimensions if extraction failed
//     if (frame_width == 0 || frame_height == 0) {
//         frame_width = expected_width;
//         frame_height = expected_height;
//     }
    
//     // Decode the frame
//     vpx_codec_err_t err = vpx_codec_decode(codec_ctx_.get(), data, data_size, nullptr, 0);
//     if (err != VPX_CODEC_OK) {
//         if (frame_count_ % 10 == 1) {  // Log occasionally to reduce spam
//             std::cerr << "Failed to decode VP8 frame: " << vpx_codec_error(codec_ctx_.get()) 
//                       << " (frame " << frame_count_ << ", size: " << data_size << ")" << std::endl;
//         }
//         return cv::Mat();
//     }
    
//     // Get the decoded frame
//     vpx_codec_iter_t iter = nullptr;
//     const vpx_image_t* img = vpx_codec_get_frame(codec_ctx_.get(), &iter);
    
//     if (!img) {
//         if (frame_count_ % 10 == 1) {
//             std::cerr << "No decoded frame available from VP8 decoder (frame " << frame_count_ << ")" << std::endl;
//         }
//         return cv::Mat();
//     }
    
//     // Convert VPX image to OpenCV Mat
//     cv::Mat result = convertVpxImageToMat(img);
    
//     if (frame_count_ % 30 == 1) {  // Log success occasionally
//         std::cout << "Successfully decoded VP8 frame " << frame_count_ 
//                   << ": " << result.cols << "x" << result.rows << std::endl;
//     }
    
//     return result;
// }

// cv::Mat VP8Decoder::convertVpxImageToMat(const vpx_image* img) {
//     if (!img) {
//         return cv::Mat();
//     }
    
//     cv::Mat result;
    
//     // Handle different pixel formats
//     switch (img->fmt) {
//         case VPX_IMG_FMT_I420: {
//             // YUV420 format - most common
//             cv::Mat yuv(img->d_h + img->d_h/2, img->d_w, CV_8UC1);
            
//             // Copy Y plane
//             for (unsigned int y = 0; y < img->d_h; y++) {
//                 memcpy(yuv.ptr(y), img->planes[VPX_PLANE_Y] + y * img->stride[VPX_PLANE_Y], img->d_w);
//             }
            
//             // Copy U and V planes (interleaved)
//             for (unsigned int y = 0; y < img->d_h/2; y++) {
//                 uint8_t* dst = yuv.ptr(img->d_h + y);
//                 uint8_t* src_u = img->planes[VPX_PLANE_U] + y * img->stride[VPX_PLANE_U];
//                 uint8_t* src_v = img->planes[VPX_PLANE_V] + y * img->stride[VPX_PLANE_V];
                
//                 for (unsigned int x = 0; x < img->d_w/2; x++) {
//                     dst[x*2] = src_u[x];
//                     dst[x*2+1] = src_v[x];
//                 }
//             }
            
//             // Convert YUV to BGR
//             cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR_I420);
//             break;
//         }
        
//         case VPX_IMG_FMT_YV12: {
//             // YUV420 with swapped U/V planes
//             cv::Mat yuv(img->d_h + img->d_h/2, img->d_w, CV_8UC1);
            
//             // Copy Y plane
//             for (unsigned int y = 0; y < img->d_h; y++) {
//                 memcpy(yuv.ptr(y), img->planes[VPX_PLANE_Y] + y * img->stride[VPX_PLANE_Y], img->d_w);
//             }
            
//             // Copy V and U planes (swapped for YV12)
//             for (unsigned int y = 0; y < img->d_h/2; y++) {
//                 uint8_t* dst = yuv.ptr(img->d_h + y);
//                 uint8_t* src_v = img->planes[VPX_PLANE_V] + y * img->stride[VPX_PLANE_V];
//                 uint8_t* src_u = img->planes[VPX_PLANE_U] + y * img->stride[VPX_PLANE_U];
                
//                 for (unsigned int x = 0; x < img->d_w/2; x++) {
//                     dst[x*2] = src_u[x];     // Note: U and V swapped
//                     dst[x*2+1] = src_v[x];
//                 }
//             }
            
//             cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR_I420);
//             break;
//         }
        
//         default:
//             std::cerr << "Unsupported VP8 pixel format: " << img->fmt << std::endl;
//             return cv::Mat();
//     }
    
//     return result;
// }

// #endif // VP8_DECODER_HPP

























#ifndef VP8_DECODER_HPP
#define VP8_DECODER_HPP

#include <opencv2/opencv.hpp>
#include <memory>
#include <iostream>
#include <cstring> // for memset

// Include libvpx headers for implementation
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

// Forward declarations for libvpx types
struct vpx_codec_ctx;
struct vpx_image;

class VP8Decoder {
public:
    VP8Decoder();
    ~VP8Decoder();
    
    bool initialize();
    cv::Mat decodeFrame(const uint8_t* data, size_t data_size, int expected_width = 0, int expected_height = 0);
    void cleanup();

private:
    std::unique_ptr<vpx_codec_ctx> codec_ctx_;
    bool initialized_;
    int frame_count_;
    
    // Helper methods
    bool isValidVP8Frame(const uint8_t* data, size_t size);
    cv::Mat convertVpxImageToMat(const vpx_image* img);
    void extractVP8Dimensions(const uint8_t* data, size_t size, int& width, int& height);
};

// =========================================================================================
// Implementation (Included in Header for simplicity, as per original file structure)
// =========================================================================================

VP8Decoder::VP8Decoder() : initialized_(false), frame_count_(0) {
    codec_ctx_ = std::make_unique<vpx_codec_ctx>();
}

VP8Decoder::~VP8Decoder() {
    cleanup();
}

bool VP8Decoder::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize VP8 decoder
    vpx_codec_iface_t* iface = vpx_codec_vp8_dx();
    if (!iface) {
        std::cerr << "ERROR: VP8 decoder interface not available" << std::endl;
        return false;
    }
    
    vpx_codec_dec_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.threads = 1;  
    cfg.w = 0;       
    cfg.h = 0;       
    
    vpx_codec_err_t err = vpx_codec_dec_init(codec_ctx_.get(), iface, &cfg, 0);
    if (err != VPX_CODEC_OK) {
        std::cerr << "ERROR: Failed to initialize VP8 decoder: " 
                  << vpx_codec_error(codec_ctx_.get()) << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "VP8 decoder initialized successfully" << std::endl;
    return true;
}

void VP8Decoder::cleanup() {
    if (initialized_ && codec_ctx_) {
        vpx_codec_destroy(codec_ctx_.get());
        initialized_ = false;
    }
}

bool VP8Decoder::isValidVP8Frame(const uint8_t* data, size_t size) {
    if (!data || size < 3 || size > 1024 * 1024 * 2) {
        return false;
    }
    
    uint8_t first_byte = data[0];
    bool is_keyframe = (first_byte & 0x01) == 0;
    uint8_t version = (first_byte >> 1) & 0x07;
    
    if (version > 3) {
        return false;
    }
    
    if (is_keyframe && size >= 10) {
        if (size >= 3) {
            uint32_t sync_code = (data[0]) | (data[1] << 8) | (data[2] << 16);
            if ((sync_code & 0xFFFF) == 0x019D) { 
                return true;
            }
        }
    }
    
    return true;
}

void VP8Decoder::extractVP8Dimensions(const uint8_t* data, size_t size, int& width, int& height) {
    width = 0;
    height = 0;
    
    if (!data || size < 10) {
        return;
    }
    
    if ((data[0] & 0x01) == 0) {  // Keyframe
        if (size >= 10) {
            width = data[6] | (data[7] << 8);
            height = data[8] | (data[9] << 8);
            
            width &= 0x3FFF;
            height &= 0x3FFF;
        }
    }
}

// =========================================================================================
// THE FIX IS HERE: Rewritten decodeFrame for robustness against initial P-frames
// =========================================================================================

cv::Mat VP8Decoder::decodeFrame(const uint8_t* data, size_t data_size, int expected_width, int expected_height) {
    if (!initialized_ || !data || data_size == 0) {
        std::cerr << "VP8 decoder not initialized or invalid input data" << std::endl;
        return cv::Mat();
    }
    
    frame_count_++;
    
    // --- Step 1: Pre-validation ---
    if (!isValidVP8Frame(data, data_size)) {
        if (frame_count_ % 50 == 1) { 
            std::cerr << "Invalid VP8 frame data (frame " << frame_count_ << ")" << std::endl;
        }
        return cv::Mat();
    }
    
    // --- Step 2: Decode the frame ---
    vpx_codec_err_t err = vpx_codec_decode(codec_ctx_.get(), data, data_size, nullptr, 0);
    
    // CRITICAL FIX: DO NOT return on decode error. Non-fatal errors (like "Bitstream not supported") 
    // are expected when receiving a P-frame before the keyframe. We MUST proceed to check if a frame was output.
    if (err != VPX_CODEC_OK) {
        if (frame_count_ % 50 == 1) { 
            // Log as a warning/debug message, not a fatal error
            std::cerr << "⚠️ VP8 Decode failed (Likely P-frame without keyframe context): " 
                      << vpx_codec_error(codec_ctx_.get()) 
                      << " (frame " << frame_count_ << ", size: " << data_size << ")" << std::endl;
        }
    }
    
    // --- Step 3: Get the decoded frame (The true success indicator) ---
    vpx_codec_iter_t iter = nullptr;
    const vpx_image_t* img = vpx_codec_get_frame(codec_ctx_.get(), &iter);
    
    if (!img) {
        // If img is null, no *new* frame was produced. The frame is discarded.
        return cv::Mat();
    }
    
    // --- Step 4: Success! Convert and Return ---
    cv::Mat result = convertVpxImageToMat(img);
    
    if (frame_count_ % 30 == 1) { 
        std::cout << "✅ Successfully decoded VP8 frame " << frame_count_ 
                  << ": " << result.cols << "x" << result.rows << std::endl;
    }
    
    return result;
}

// =========================================================================================
// YUV to Mat Conversion (Unchanged)
// =========================================================================================

cv::Mat VP8Decoder::convertVpxImageToMat(const vpx_image* img) {
    if (!img) {
        return cv::Mat();
    }
    
    cv::Mat result;
    
    switch (img->fmt) {
        case VPX_IMG_FMT_I420: {
            cv::Mat yuv(img->d_h + img->d_h/2, img->d_w, CV_8UC1);
            
            // Copy Y plane
            for (unsigned int y = 0; y < img->d_h; y++) {
                memcpy(yuv.ptr(y), img->planes[VPX_PLANE_Y] + y * img->stride[VPX_PLANE_Y], img->d_w);
            }
            
            // Copy U and V planes (interleaved)
            for (unsigned int y = 0; y < img->d_h/2; y++) {
                uint8_t* dst = yuv.ptr(img->d_h + y);
                uint8_t* src_u = img->planes[VPX_PLANE_U] + y * img->stride[VPX_PLANE_U];
                uint8_t* src_v = img->planes[VPX_PLANE_V] + y * img->stride[VPX_PLANE_V];
                
                for (unsigned int x = 0; x < img->d_w/2; x++) {
                    dst[x*2] = src_u[x];
                    dst[x*2+1] = src_v[x];
                }
            }
            
            cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR_I420);
            break;
        }
        
        case VPX_IMG_FMT_YV12: {
            cv::Mat yuv(img->d_h + img->d_h/2, img->d_w, CV_8UC1);
            
            // Copy Y plane
            for (unsigned int y = 0; y < img->d_h; y++) {
                memcpy(yuv.ptr(y), img->planes[VPX_PLANE_Y] + y * img->stride[VPX_PLANE_Y], img->d_w);
            }
            
            // Copy V and U planes (swapped for YV12)
            for (unsigned int y = 0; y < img->d_h/2; y++) {
                uint8_t* dst = yuv.ptr(img->d_h + y);
                uint8_t* src_v = img->planes[VPX_PLANE_V] + y * img->stride[VPX_PLANE_V];
                uint8_t* src_u = img->planes[VPX_PLANE_U] + y * img->stride[VPX_PLANE_U];
                
                for (unsigned int x = 0; x < img->d_w/2; x++) {
                    dst[x*2] = src_u[x];     
                    dst[x*2+1] = src_v[x];
                }
            }
            
            cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR_I420);
            break;
        }
        
        default:
            std::cerr << "Unsupported VP8 pixel format: " << img->fmt << std::endl;
            return cv::Mat();
    }
    
    return result;
}

#endif // VP8_DECODER_HPP