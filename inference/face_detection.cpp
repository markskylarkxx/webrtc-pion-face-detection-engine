//working with test pattern
// #include "face_detection.hpp"
// #include <chrono>
// #include <iostream>
// #include <opencv2/opencv.hpp>

// FaceDetector::FaceDetector() {}

// FaceDetector::~FaceDetector() { 
//     cleanup(); 
// }

// bool FaceDetector::initialize() {
//     std::cout << "Initializing FaceDetector..." << std::endl;
    
//     // Try to load Haar cascade from different possible paths
//     const char* cascade_paths[] = {
//         "haarcascade_frontalface_default.xml",
//         "haarcascade_frontalface_alt.xml",
//         "haarcascade_frontalface_alt2.xml",
//         "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
//         "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
//         nullptr
//     };
    
//     for (int i = 0; cascade_paths[i] != nullptr; i++) {
//         std::cout << "Trying to load cascade: " << cascade_paths[i] << std::endl;
//         if (face_cascade.load(cascade_paths[i])) {
//             std::cout << "✅ Successfully loaded Haar cascade: " << cascade_paths[i] << std::endl;
//             return true;
//         }
//     }
    
//     std::cerr << "❌ ERROR: Could not load any Haar cascade file!" << std::endl;
//     std::cerr << "Please download haarcascade_frontalface_default.xml and place it in the current directory" << std::endl;
//     return false;
// }

// InferenceResult FaceDetector::processFrame(const uint8_t* frame_data, int width, int height, int channels) {
//     InferenceResult result;
//     result.faces_detected = 0;
//     result.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
    
//     // Validate input
//     if (!frame_data || width <= 0 || height <= 0) {
//         std::cerr << "❌ Invalid frame data received" << std::endl;
//         return result;
//     }
    
//     std::cout << "🔍 Processing frame: " << width << "x" << height << " channels: " << channels << std::endl;
    
//     try {
//         // Create OpenCV Mat from the frame data
//         cv::Mat frame;
//         if (channels == 1) {
//             // Grayscale image
//             frame = cv::Mat(height, width, CV_8UC1, (void*)frame_data);
//         } else if (channels == 3) {
//             // RGB image - convert to grayscale for face detection
//             cv::Mat color_frame(height, width, CV_8UC3, (void*)frame_data);
//             cv::cvtColor(color_frame, frame, cv::COLOR_RGB2GRAY);
//         } else {
//             std::cerr << "❌ Unsupported number of channels: " << channels << std::endl;
//             return result;
//         }
        
//         if (frame.empty()) {
//             std::cerr << "❌ Created empty OpenCV Mat!" << std::endl;
//             return result;
//         }
        
//         std::cout << "✅ OpenCV Mat created: " << frame.cols << "x" << frame.rows 
//                   << " type: " << frame.type() << std::endl;
        
//         // Equalize histogram for better detection
//         cv::Mat equalized_frame;
//         cv::equalizeHist(frame, equalized_frame);
        
//         // Detect faces
//         std::vector<cv::Rect> faces;
//         face_cascade.detectMultiScale(
//             equalized_frame, 
//             faces, 
//             1.1,    // scale factor
//             3,      // min neighbors
//             0,      // flags (0 for default)
//             cv::Size(30, 30)  // min size
//         );
        
//         std::cout << "🎯 Detected " << faces.size() << " potential faces" << std::endl;
        
//         // Convert OpenCV rectangles to our format
//         for (const auto& face_rect : faces) {
//             FaceBox box;
//             box.x = face_rect.x;
//             box.y = face_rect.y;
//             box.width = face_rect.width;
//             box.height = face_rect.height;
//             box.confidence = 0.95f; // Haar cascade doesn't provide confidence
            
//             result.bounding_boxes.push_back(box);
//             result.faces_detected++;
//         }
        
//         std::cout << "✅ Face detection completed. Found " << result.faces_detected << " faces" << std::endl;
        
//     } catch (const cv::Exception& e) {
//         std::cerr << "❌ OpenCV Exception in processFrame: " << e.what() << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "❌ Exception in processFrame: " << e.what() << std::endl;
//     }
    
//     return result;
// }

// void FaceDetector::cleanup() {
//     // Cascade classifier automatically cleans up
//     std::cout << "FaceDetector cleanup completed" << std::endl;
// }

// // C-style interface implementations
// FaceDetector* create_detector() {
//     FaceDetector* detector = new FaceDetector();
//     if (detector->initialize()) {
//         return detector;
//     }
//     delete detector;
//     return nullptr;
// }

// void destroy_detector(FaceDetector* detector) {
//     if (detector) {
//         detector->cleanup();
//         delete detector;
//     }
// }

// InferenceResult process_frame(FaceDetector* detector, const uint8_t* data, int w, int h, int c) {
//     if (detector) {
//         return detector->processFrame(data, w, h, c);
//     }
    
//     InferenceResult empty{};
//     empty.faces_detected = 0;
//     empty.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
//     return empty;
// }










//DECODING IN C++ SERVER.
#include "face_detection.hpp"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>

extern "C" {
#include <libswscale/swscale.h>
}

using namespace std::chrono;

FaceDetector::FaceDetector() 
    : decoder_ctx(nullptr), 
      packet(nullptr), 
      frame(nullptr),
      decoder_initialized(false),
      frame_counter(0),
      process_every_n_frames(1),
      has_received_keyframe(false),
      consecutive_failures(0) {
}

FaceDetector::~FaceDetector() {
    cleanupDecoder();
}

bool FaceDetector::initialize(const std::string& cascade_path) {
    std::cout << "Initializing FaceDetector..." << std::endl;

    std::string path = cascade_path;
    if (path.empty()) {
        const char* cascade_paths[] = {
            "haarcascade_frontalface_default.xml",
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            nullptr
        };
        for (int i = 0; cascade_paths[i] != nullptr; ++i) {
            if (face_cascade.load(cascade_paths[i])) {
                std::cout << "✅ Loaded cascade: " << cascade_paths[i] << std::endl;
                return true;
            }
        }
        std::cerr << "❌ ERROR: Could not load Haar cascade" << std::endl;
        return false;
    }

    if (!face_cascade.load(path)) {
        std::cerr << "❌ ERROR: Could not load cascade from: " << path << std::endl;
        return false;
    }

    std::cout << "✅ Loaded cascade from: " << path << std::endl;
    return true;
}

void FaceDetector::cleanup() {
    cleanupDecoder();
}

bool FaceDetector::isVP8Keyframe(const uint8_t* data, size_t size) {
    if (size < 10) {
        return false;
    }
    
    // VP8 keyframe detection
    // Bit 0 of first byte: 0=keyframe, 1=interframe
    bool isKeyframe = (data[0] & 0x01) == 0;
    
    if (isKeyframe && size >= 6) {
        // Verify start code for keyframes: 0x9d 0x01 0x2a
        bool hasStartCode = (data[3] == 0x9d && data[4] == 0x01 && data[5] == 0x2a);
        return hasStartCode;
    }
    
    return false;
}

AVCodecID FaceDetector::pickCodecIdFromString(const std::string& codec) {
    std::string s = codec;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s.find("h264") != std::string::npos || s.find("avc") != std::string::npos) 
        return AV_CODEC_ID_H264;
    if (s.find("vp8") != std::string::npos) 
        return AV_CODEC_ID_VP8;
    if (s.find("vp9") != std::string::npos) 
        return AV_CODEC_ID_VP9;

    return AV_CODEC_ID_VP8;
}

bool FaceDetector::initializeDecoder(const std::string& codec) {
    AVCodecID codec_id = pickCodecIdFromString(codec);
    const AVCodec* decoder = avcodec_find_decoder(codec_id);
    if (!decoder) {
        std::cerr << "❌ Failed to find decoder for: " << codec << std::endl;
        return false;
    }

    decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx) {
        std::cerr << "❌ Failed to allocate codec context" << std::endl;
        return false;
    }

    // FIXED: Optimized decoder configuration for WebRTC VP8 streams
    decoder_ctx->thread_count = 1;
    decoder_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    decoder_ctx->flags2 |= AV_CODEC_FLAG2_CHUNKS;  // Handle incomplete frames
    decoder_ctx->flags2 |= AV_CODEC_FLAG2_SHOW_ALL; // Show all frames
    
    // Error resilience for WebRTC streams
    decoder_ctx->err_recognition = AV_EF_IGNORE_ERR;
    decoder_ctx->workaround_bugs = FF_BUG_AUTODETECT;
    
    // VP8-specific settings
    if (codec_id == AV_CODEC_ID_VP8) {
        decoder_ctx->skip_frame = AVDISCARD_DEFAULT;
        decoder_ctx->skip_idct = AVDISCARD_DEFAULT;
        decoder_ctx->skip_loop_filter = AVDISCARD_DEFAULT;
    }

    if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
        std::cerr << "❌ Failed to open codec" << std::endl;
        avcodec_free_context(&decoder_ctx);
        return false;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    
    if (!packet || !frame) {
        std::cerr << "❌ Failed to allocate packet/frame" << std::endl;
        cleanupDecoder();
        return false;
    }
    
    std::cout << "✅ Decoder initialized for: " << codec << std::endl;
    return true;
}

void FaceDetector::cleanupDecoder() {
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (decoder_ctx) {
        avcodec_free_context(&decoder_ctx);
        decoder_ctx = nullptr;
    }
    decoder_initialized = false;
    has_received_keyframe = false;
    consecutive_failures = 0;
    current_codec.clear();
}

void FaceDetector::resetDecoder() {
    std::lock_guard<std::mutex> lock(decoder_mutex);
    if (decoder_ctx) {
        avcodec_flush_buffers(decoder_ctx);
        has_received_keyframe = false;
        consecutive_failures = 0;
        std::cout << "🔄 Decoder flushed and reset" << std::endl;
    }
}

InferenceResult FaceDetector::processFrame(const uint8_t* encoded_data, size_t encoded_size,
                                           const std::string& codec,
                                           int expected_width, int expected_height) {
    InferenceResult result;
    result.faces_detected = 0;
    result.timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    // Validation
    if (!encoded_data || encoded_size == 0) {
        return result;
    }

    if (encoded_size < 10) {
        std::cout << "❌ Frame too small: " << encoded_size << " bytes" << std::endl;
        return result;
    }

    if (encoded_size > 200000) {
        std::cout << "❌ Frame too large: " << encoded_size << " bytes" << std::endl;
        return result;
    }

    // Check if keyframe
    bool is_keyframe = isVP8Keyframe(encoded_data, encoded_size);
    
    std::cout << "Processing " << (is_keyframe ? "KEYFRAME" : "P-frame") 
              << ": " << encoded_size << " bytes" << std::endl;

    std::lock_guard<std::mutex> lock(decoder_mutex);

    // Initialize decoder on first frame or codec change
    if (!decoder_initialized || current_codec != codec) {
        cleanupDecoder();
        if (!initializeDecoder(codec)) {
            return result;
        }
        decoder_initialized = true;
        current_codec = codec;
    }

    // CRITICAL: Wait for keyframe before processing P-frames
    if (!has_received_keyframe && !is_keyframe) {
        std::cout << "Waiting for keyframe, skipping P-frame" << std::endl;
        return result;
    }

    // Prepare packet
    av_packet_unref(packet);
    
    if (av_new_packet(packet, encoded_size) < 0) {
        std::cout << "Failed to allocate packet" << std::endl;
        consecutive_failures++;
        return result;
    }
    
    memcpy(packet->data, encoded_data, encoded_size);
    packet->size = encoded_size;
    
    // Mark keyframes
    if (is_keyframe) {
        packet->flags |= AV_PKT_FLAG_KEY;
    }

    // Send packet to decoder
    int send_ret = avcodec_send_packet(decoder_ctx, packet);
    if (send_ret < 0) {
        char errbuf[128];
        av_strerror(send_ret, errbuf, sizeof(errbuf));
        std::cout << "Failed to send packet: " << errbuf << std::endl;
        
        consecutive_failures++;
        
        // Reset decoder after multiple failures
        if (consecutive_failures > 5) {
            std::cout << "Too many failures, resetting decoder" << std::endl;
            avcodec_flush_buffers(decoder_ctx);
            has_received_keyframe = false;
            consecutive_failures = 0;
        }
        
        return result;
    }

    // Receive decoded frame
    av_frame_unref(frame);
    int recv_ret = avcodec_receive_frame(decoder_ctx, frame);
    
    if (recv_ret == AVERROR(EAGAIN)) {
        // Need more data
        return result;
    } else if (recv_ret == AVERROR_EOF) {
        return result;
    } else if (recv_ret < 0) {
        char errbuf[128];
        av_strerror(recv_ret, errbuf, sizeof(errbuf));
        std::cout << "Failed to receive frame: " << errbuf << std::endl;
        consecutive_failures++;
        return result;
    }

    // Successfully decoded
    consecutive_failures = 0;
    has_received_keyframe = true;
    
    std::cout << "Decoded frame: " << frame->width << "x" << frame->height << std::endl;
    
    processDecodedFrame(frame, result);
    
    if (result.faces_detected > 0) {
        std::cout << "SUCCESS: Detected " << result.faces_detected << " face(s)" << std::endl;
    }

    return result;
}

int FaceDetector::processDecodedFrame(AVFrame* frame, InferenceResult& result) {
    if (!frame || frame->width <= 0 || frame->height <= 0) {
        return -1;
    }

    std::cout << "Processing decoded frame: " << frame->width << "x" << frame->height << std::endl;

    // Scale down for performance
    int process_width = frame->width;
    int process_height = frame->height;
    
    if (frame->width > 640) {
        process_width = 640;
        process_height = (640 * frame->height) / frame->width;
        std::cout << "Scaling to: " << process_width << "x" << process_height << std::endl;
    }

    // Convert to BGR for OpenCV
    SwsContext* swsCtx = sws_getContext(
        frame->width, frame->height, (AVPixelFormat)frame->format,
        process_width, process_height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
        
    if (!swsCtx) {
        std::cout << "Failed to create SwsContext" << std::endl;
        return -1;
    }

    int stride = 3 * process_width;
    std::vector<uint8_t> bgr_buf(stride * process_height);
    uint8_t* dst[4] = { bgr_buf.data(), nullptr, nullptr, nullptr };
    int dst_stride[4] = { stride, 0, 0, 0 };

    int scale_ret = sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dst, dst_stride);
    sws_freeContext(swsCtx);
    
    if (scale_ret < 0) {
        std::cout << "Failed to scale image" << std::endl;
        return -1;
    }

    // Create OpenCV Mat
    cv::Mat img(process_height, process_width, CV_8UC3, bgr_buf.data(), stride);
    if (img.empty()) {
        std::cout << "Empty image after conversion" << std::endl;
        return -1;
    }

    cv::Mat gray;
    try {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
    } catch (const cv::Exception& ex) {
        std::cout << "OpenCV conversion error: " << ex.what() << std::endl;
        return -1;
    }

    // Face detection
    std::vector<cv::Rect> faces;
    try {
        face_cascade.detectMultiScale(
            gray, faces, 
            1.1,        // scaleFactor
            3,          // minNeighbors
            0 | cv::CASCADE_SCALE_IMAGE, 
            cv::Size(20, 20),
            cv::Size(400, 400)
        );
        
        std::cout << "Detected " << faces.size() << " face(s)" << std::endl;
        
    } catch (const cv::Exception& ex) {
        std::cout << "Face detection error: " << ex.what() << std::endl;
        return -1;
    }

    // Scale coordinates back
    float scale_x = frame->width / (float)process_width;
    float scale_y = frame->height / (float)process_height;
    
    for (const auto& rect : faces) {
        FaceBox box;
        box.x = rect.x * scale_x;
        box.y = rect.y * scale_y;
        box.width = rect.width * scale_x;
        box.height = rect.height * scale_y;
        box.confidence = 1.0f;
        result.bounding_boxes.push_back(box);
    }
    
    result.faces_detected = static_cast<int>(faces.size());

    return 0;
}

// C-style wrappers
extern "C" {

FaceDetector* create_detector() {
    FaceDetector* d = new FaceDetector();
    if (!d->initialize()) {
        delete d;
        return nullptr;
    }
    return d;
}

FaceDetector* create_detector_with_cascade(const char* cascade_path) {
    FaceDetector* d = new FaceDetector();
    std::string path = cascade_path ? std::string(cascade_path) : std::string();
    if (!d->initialize(path)) {
        delete d;
        return nullptr;
    }
    return d;
}

void destroy_detector(FaceDetector* detector) {
    if (detector) {
        detector->cleanup();
        delete detector;
    }
}

void reset_detector(FaceDetector* detector) {
    if (detector) {
        detector->resetDecoder();
    }
}

void process_frame_c(FaceDetector* detector,
                     const uint8_t* encoded_data,
                     size_t encoded_size,
                     const char* codec,
                     int expected_width,
                     int expected_height,
                     int* faces_detected,
                     int64_t* timestamp) {
    if (!detector || !faces_detected || !timestamp) {
        if (faces_detected) *faces_detected = 0;
        if (timestamp) *timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        return;
    }
    
    std::string codec_str = codec ? std::string(codec) : std::string("vp8");
    InferenceResult result = detector->processFrame(encoded_data, encoded_size, codec_str, expected_width, expected_height);
    
    *faces_detected = result.faces_detected;
    *timestamp = result.timestamp;
}

}