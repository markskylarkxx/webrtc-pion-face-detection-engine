







//DECODING IN C++ SERVER.
// #include "face_detection.hpp"

// #include <chrono>
// #include <iostream>
// #include <algorithm>
// #include <vector>
// #include <cstring>

// extern "C" {
// #include <libswscale/swscale.h>
// }

// using namespace std::chrono;

// FaceDetector::FaceDetector() 
//     : decoder_ctx(nullptr), 
//       packet(nullptr), 
//       frame(nullptr),
//       decoder_initialized(false),
//       frame_counter(0),
//       process_every_n_frames(1),
//       has_received_keyframe(false),
//       consecutive_failures(0) {
// }

// FaceDetector::~FaceDetector() {
//     cleanupDecoder();
// }

// bool FaceDetector::initialize(const std::string& cascade_path) {
//     std::cout << "Initializing FaceDetector..." << std::endl;

//     std::string path = cascade_path;
//     if (path.empty()) {
//         const char* cascade_paths[] = {
//             "haarcascade_frontalface_default.xml",
//             "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
//             "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
//             "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
//             nullptr
//         };
//         for (int i = 0; cascade_paths[i] != nullptr; ++i) {
//             if (face_cascade.load(cascade_paths[i])) {
//                 std::cout << "✅ Loaded cascade: " << cascade_paths[i] << std::endl;
//                 return true;
//             }
//         }
//         std::cerr << "❌ ERROR: Could not load Haar cascade" << std::endl;
//         return false;
//     }

//     if (!face_cascade.load(path)) {
//         std::cerr << "❌ ERROR: Could not load cascade from: " << path << std::endl;
//         return false;
//     }

//     std::cout << "✅ Loaded cascade from: " << path << std::endl;
//     return true;
// }

// void FaceDetector::cleanup() {
//     cleanupDecoder();
// }

// bool FaceDetector::isVP8Keyframe(const uint8_t* data, size_t size) {
//     if (size < 10) {
//         return false;
//     }
    
//     // VP8 keyframe detection
//     // Bit 0 of first byte: 0=keyframe, 1=interframe
//     bool isKeyframe = (data[0] & 0x01) == 0;
    
//     if (isKeyframe && size >= 6) {
//         // Verify start code for keyframes: 0x9d 0x01 0x2a
//         bool hasStartCode = (data[3] == 0x9d && data[4] == 0x01 && data[5] == 0x2a);
//         return hasStartCode;
//     }
    
//     return false;
// }

// AVCodecID FaceDetector::pickCodecIdFromString(const std::string& codec) {
//     std::string s = codec;
//     std::transform(s.begin(), s.end(), s.begin(), ::tolower);

//     if (s.find("h264") != std::string::npos || s.find("avc") != std::string::npos) 
//         return AV_CODEC_ID_H264;
//     if (s.find("vp8") != std::string::npos) 
//         return AV_CODEC_ID_VP8;
//     if (s.find("vp9") != std::string::npos) 
//         return AV_CODEC_ID_VP9;

//     return AV_CODEC_ID_VP8;
// }

// bool FaceDetector::initializeDecoder(const std::string& codec) {
//     AVCodecID codec_id = pickCodecIdFromString(codec);
//     const AVCodec* decoder = avcodec_find_decoder(codec_id);
//     if (!decoder) {
//         std::cerr << "❌ Failed to find decoder for: " << codec << std::endl;
//         return false;
//     }

//     decoder_ctx = avcodec_alloc_context3(decoder);
//     if (!decoder_ctx) {
//         std::cerr << "❌ Failed to allocate codec context" << std::endl;
//         return false;
//     }

//     // FIXED: Optimized decoder configuration for WebRTC VP8 streams
//     decoder_ctx->thread_count = 1;
//     decoder_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
//     decoder_ctx->flags2 |= AV_CODEC_FLAG2_CHUNKS;  // Handle incomplete frames
//     decoder_ctx->flags2 |= AV_CODEC_FLAG2_SHOW_ALL; // Show all frames
    
//     // Error resilience for WebRTC streams
//     decoder_ctx->err_recognition = AV_EF_IGNORE_ERR;
//     decoder_ctx->workaround_bugs = FF_BUG_AUTODETECT;
    
//     // VP8-specific settings
//     if (codec_id == AV_CODEC_ID_VP8) {
//         decoder_ctx->skip_frame = AVDISCARD_DEFAULT;
//         decoder_ctx->skip_idct = AVDISCARD_DEFAULT;
//         decoder_ctx->skip_loop_filter = AVDISCARD_DEFAULT;
//     }

//     if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
//         std::cerr << "❌ Failed to open codec" << std::endl;
//         avcodec_free_context(&decoder_ctx);
//         return false;
//     }

//     packet = av_packet_alloc();
//     frame = av_frame_alloc();
    
//     if (!packet || !frame) {
//         std::cerr << "❌ Failed to allocate packet/frame" << std::endl;
//         cleanupDecoder();
//         return false;
//     }
    
//     std::cout << "✅ Decoder initialized for: " << codec << std::endl;
//     return true;
// }

// void FaceDetector::cleanupDecoder() {
//     if (packet) {
//         av_packet_free(&packet);
//         packet = nullptr;
//     }
//     if (frame) {
//         av_frame_free(&frame);
//         frame = nullptr;
//     }
//     if (decoder_ctx) {
//         avcodec_free_context(&decoder_ctx);
//         decoder_ctx = nullptr;
//     }
//     decoder_initialized = false;
//     has_received_keyframe = false;
//     consecutive_failures = 0;
//     current_codec.clear();
// }

// void FaceDetector::resetDecoder() {
//     std::lock_guard<std::mutex> lock(decoder_mutex);
//     if (decoder_ctx) {
//         avcodec_flush_buffers(decoder_ctx);
//         has_received_keyframe = false;
//         consecutive_failures = 0;
//         std::cout << "🔄 Decoder flushed and reset" << std::endl;
//     }
// }

// InferenceResult FaceDetector::processFrame(const uint8_t* encoded_data, size_t encoded_size,
//                                            const std::string& codec,
//                                            int expected_width, int expected_height) {
//     InferenceResult result;
//     result.faces_detected = 0;
//     result.timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

//     // Validation
//     if (!encoded_data || encoded_size == 0) {
//         return result;
//     }

//     if (encoded_size < 10) {
//         std::cout << "❌ Frame too small: " << encoded_size << " bytes" << std::endl;
//         return result;
//     }

//     if (encoded_size > 200000) {
//         std::cout << "❌ Frame too large: " << encoded_size << " bytes" << std::endl;
//         return result;
//     }

//     // Check if keyframe
//     bool is_keyframe = isVP8Keyframe(encoded_data, encoded_size);
    
//     std::cout << "Processing " << (is_keyframe ? "KEYFRAME" : "P-frame") 
//               << ": " << encoded_size << " bytes" << std::endl;

//     std::lock_guard<std::mutex> lock(decoder_mutex);

//     // Initialize decoder on first frame or codec change
//     if (!decoder_initialized || current_codec != codec) {
//         cleanupDecoder();
//         if (!initializeDecoder(codec)) {
//             return result;
//         }
//         decoder_initialized = true;
//         current_codec = codec;
//     }

//     // CRITICAL: Wait for keyframe before processing P-frames
//     if (!has_received_keyframe && !is_keyframe) {
//         std::cout << "Waiting for keyframe, skipping P-frame" << std::endl;
//         return result;
//     }

//     // Prepare packet
//     av_packet_unref(packet);
    
//     if (av_new_packet(packet, encoded_size) < 0) {
//         std::cout << "Failed to allocate packet" << std::endl;
//         consecutive_failures++;
//         return result;
//     }
    
//     memcpy(packet->data, encoded_data, encoded_size);
//     packet->size = encoded_size;
    
//     // Mark keyframes
//     if (is_keyframe) {
//         packet->flags |= AV_PKT_FLAG_KEY;
//     }

//     // Send packet to decoder
//     int send_ret = avcodec_send_packet(decoder_ctx, packet);
//     if (send_ret < 0) {
//         char errbuf[128];
//         av_strerror(send_ret, errbuf, sizeof(errbuf));
//         std::cout << "Failed to send packet: " << errbuf << std::endl;
        
//         consecutive_failures++;
        
//         // Reset decoder after multiple failures
//         if (consecutive_failures > 5) {
//             std::cout << "Too many failures, resetting decoder" << std::endl;
//             avcodec_flush_buffers(decoder_ctx);
//             has_received_keyframe = false;
//             consecutive_failures = 0;
//         }
        
//         return result;
//     }

//     // Receive decoded frame
//     av_frame_unref(frame);
//     int recv_ret = avcodec_receive_frame(decoder_ctx, frame);
    
//     if (recv_ret == AVERROR(EAGAIN)) {
//         // Need more data
//         return result;
//     } else if (recv_ret == AVERROR_EOF) {
//         return result;
//     } else if (recv_ret < 0) {
//         char errbuf[128];
//         av_strerror(recv_ret, errbuf, sizeof(errbuf));
//         std::cout << "Failed to receive frame: " << errbuf << std::endl;
//         consecutive_failures++;
//         return result;
//     }

//     // Successfully decoded
//     consecutive_failures = 0;
//     has_received_keyframe = true;
    
//     std::cout << "Decoded frame: " << frame->width << "x" << frame->height << std::endl;
    
//     processDecodedFrame(frame, result);
    
//     if (result.faces_detected > 0) {
//         std::cout << "SUCCESS: Detected " << result.faces_detected << " face(s)" << std::endl;
//     }

//     return result;
// }

// int FaceDetector::processDecodedFrame(AVFrame* frame, InferenceResult& result) {
//     if (!frame || frame->width <= 0 || frame->height <= 0) {
//         return -1;
//     }

//     std::cout << "Processing decoded frame: " << frame->width << "x" << frame->height << std::endl;

//     // Scale down for performance
//     int process_width = frame->width;
//     int process_height = frame->height;
    
//     if (frame->width > 640) {
//         process_width = 640;
//         process_height = (640 * frame->height) / frame->width;
//         std::cout << "Scaling to: " << process_width << "x" << process_height << std::endl;
//     }

//     // Convert to BGR for OpenCV
//     SwsContext* swsCtx = sws_getContext(
//         frame->width, frame->height, (AVPixelFormat)frame->format,
//         process_width, process_height, AV_PIX_FMT_BGR24,
//         SWS_BILINEAR, nullptr, nullptr, nullptr);
        
//     if (!swsCtx) {
//         std::cout << "Failed to create SwsContext" << std::endl;
//         return -1;
//     }

//     int stride = 3 * process_width;
//     std::vector<uint8_t> bgr_buf(stride * process_height);
//     uint8_t* dst[4] = { bgr_buf.data(), nullptr, nullptr, nullptr };
//     int dst_stride[4] = { stride, 0, 0, 0 };

//     int scale_ret = sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dst, dst_stride);
//     sws_freeContext(swsCtx);
    
//     if (scale_ret < 0) {
//         std::cout << "Failed to scale image" << std::endl;
//         return -1;
//     }

//     // Create OpenCV Mat
//     cv::Mat img(process_height, process_width, CV_8UC3, bgr_buf.data(), stride);
//     if (img.empty()) {
//         std::cout << "Empty image after conversion" << std::endl;
//         return -1;
//     }

//     cv::Mat gray;
//     try {
//         cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//         cv::equalizeHist(gray, gray);
//     } catch (const cv::Exception& ex) {
//         std::cout << "OpenCV conversion error: " << ex.what() << std::endl;
//         return -1;
//     }

//     // Face detection
//     std::vector<cv::Rect> faces;
//     try {
//         face_cascade.detectMultiScale(
//             gray, faces, 
//             1.1,        // scaleFactor
//             3,          // minNeighbors
//             0 | cv::CASCADE_SCALE_IMAGE, 
//             cv::Size(20, 20),
//             cv::Size(400, 400)
//         );
        
//         std::cout << "Detected " << faces.size() << " face(s)" << std::endl;
        
//     } catch (const cv::Exception& ex) {
//         std::cout << "Face detection error: " << ex.what() << std::endl;
//         return -1;
//     }

//     // Scale coordinates back
//     float scale_x = frame->width / (float)process_width;
//     float scale_y = frame->height / (float)process_height;
    
//     for (const auto& rect : faces) {
//         FaceBox box;
//         box.x = rect.x * scale_x;
//         box.y = rect.y * scale_y;
//         box.width = rect.width * scale_x;
//         box.height = rect.height * scale_y;
//         box.confidence = 1.0f;
//         result.bounding_boxes.push_back(box);
//     }
    
//     result.faces_detected = static_cast<int>(faces.size());

//     return 0;
// }

// // C-style wrappers
// extern "C" {

// FaceDetector* create_detector() {
//     FaceDetector* d = new FaceDetector();
//     if (!d->initialize()) {
//         delete d;
//         return nullptr;
//     }
//     return d;
// }

// FaceDetector* create_detector_with_cascade(const char* cascade_path) {
//     FaceDetector* d = new FaceDetector();
//     std::string path = cascade_path ? std::string(cascade_path) : std::string();
//     if (!d->initialize(path)) {
//         delete d;
//         return nullptr;
//     }
//     return d;
// }

// void destroy_detector(FaceDetector* detector) {
//     if (detector) {
//         detector->cleanup();
//         delete detector;
//     }
// }

// void reset_detector(FaceDetector* detector) {
//     if (detector) {
//         detector->resetDecoder();
//     }
// }

// void process_frame_c(FaceDetector* detector,
//                      const uint8_t* encoded_data,
//                      size_t encoded_size,
//                      const char* codec,
//                      int expected_width,
//                      int expected_height,
//                      int* faces_detected,
//                      int64_t* timestamp) {
//     if (!detector || !faces_detected || !timestamp) {
//         if (faces_detected) *faces_detected = 0;
//         if (timestamp) *timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//         return;
//     }
    
//     std::string codec_str = codec ? std::string(codec) : std::string("vp8");
//     InferenceResult result = detector->processFrame(encoded_data, encoded_size, codec_str, expected_width, expected_height);
    
//     *faces_detected = result.faces_detected;
//     *timestamp = result.timestamp;
// }

// }















































#include "face_detection.hpp"
#include "TfLiteEngine.h"
#include "Log.h"
#include "Preprocess.h"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cmath>
#include <memory>

extern "C" {
#include <libswscale/swscale.h>
}

using namespace std::chrono;
using neptune::Log;
using neptune::img::Preprocess;

// Sigmoid helper for confidence scores
static inline float sigmoidf(float x) { 
    return 1.0f / (1.0f + std::exp(-x)); 
}

FaceDetector::FaceDetector() 
    : decoder_ctx(nullptr), 
      packet(nullptr), 
      frame(nullptr),
      decoder_initialized(false),
      has_received_keyframe(false),
      consecutive_failures(0),
      min_confidence_(0.3f) {  // LOWERED: More sensitive detection
}

FaceDetector::~FaceDetector() {
    cleanupDecoder();
}

bool FaceDetector::initialize(const std::string& model_path) {
    Log::info("FaceDetector", "Initializing with TFLite model...");
    
    tflite_engine_ = std::make_unique<neptune::TfLiteEngine>();
    
    std::string path = model_path;
    if (path.empty()) {
        const char* model_paths[] = {
            "../models/face_detection_short_range.tflite",
            "./models/face_detection_short_range.tflite",
            "models/face_detection_short_range.tflite",
            nullptr
        };
        
        for (int i = 0; model_paths[i] != nullptr; ++i) {
            if (tflite_engine_->loadModel(model_paths[i])) {
                Log::info("FaceDetector", std::string("Loaded TFLite model: ") + model_paths[i]);
                
                // Log model input dimensions for debugging
                Log::info("FaceDetector", 
                    std::string("Model input dimensions: ") + 
                    std::to_string(tflite_engine_->inputWidth()) + "x" +
                    std::to_string(tflite_engine_->inputHeight()) + "x" +
                    std::to_string(tflite_engine_->inputChannels()));
                return true;
            }
        }
        Log::error("FaceDetector", "Could not load TFLite model from default paths");
        return false;
    }

    if (!tflite_engine_->loadModel(path)) {
        Log::error("FaceDetector", "Could not load model from: " + path);
        return false;
    }

    Log::info("FaceDetector", "Loaded TFLite model from: " + path);
    return true;
}

void FaceDetector::cleanup() {
    cleanupDecoder();
}

bool FaceDetector::isVP8Keyframe(const uint8_t* data, size_t size) {
    if (size < 10) return false;
    
    bool isKeyframe = (data[0] & 0x01) == 0;
    
    if (isKeyframe && size >= 6) {
        bool hasStartCode = (data[3] == 0x9d && data[4] == 0x01 && data[5] == 0x2a);
        return hasStartCode;
    }
    
    return isKeyframe;
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
        Log::error("FaceDetector", "Failed to find decoder for: " + codec);
        return false;
    }

    decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx) {
        Log::error("FaceDetector", "Failed to allocate codec context");
        return false;
    }

    // Optimized decoder configuration
    decoder_ctx->thread_count = 1;
    decoder_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    decoder_ctx->err_recognition = AV_EF_IGNORE_ERR;
    
    if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
        Log::error("FaceDetector", "Failed to open codec");
        avcodec_free_context(&decoder_ctx);
        return false;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    
    if (!packet || !frame) {
        Log::error("FaceDetector", "Failed to allocate packet/frame");
        cleanupDecoder();
        return false;
    }
    
    Log::info("FaceDetector", "Decoder initialized for: " + codec);
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
        Log::info("FaceDetector", "Decoder flushed and reset");
    }
}

// MediaPipe TFLite Face Detection - FIXED VERSION
std::vector<FaceBox> FaceDetector::detectFacesTFLite(const cv::Mat& image) {
    std::vector<FaceBox> results;
    
    if (!tflite_engine_ || image.empty()) {
        Log::debug("FaceDetector", "Empty image or engine not ready");
        return results;
    }

    try {
        // Convert BGR to RGB (MediaPipe models expect RGB)
        cv::Mat rgb_image;
        cv::cvtColor(image, rgb_image, cv::COLOR_BGR2RGB);
        
        // Resize to model input dimensions
        cv::Mat processed;
        cv::resize(rgb_image, processed, 
                  cv::Size(tflite_engine_->inputWidth(), tflite_engine_->inputHeight()));
        
        // Normalize pixel values to [0, 1] range
        processed.convertTo(processed, CV_32FC3, 1.0/255.0);
        
        // Create input tensor - flatten the image data
        std::vector<float> input_tensor;
        input_tensor.assign((float*)processed.datastart, (float*)processed.dataend);
        
        // Run inference
        if (!tflite_engine_->setInputTensor(input_tensor)) {
            Log::error("FaceDetector", "Failed to set input tensor");
            return results;
        }
        
        if (!tflite_engine_->invoke()) {
            Log::error("FaceDetector", "TFLite inference failed");
            return results;
        }
        
        // Get outputs - MediaPipe face detection format
        auto scores = tflite_engine_->getOutputTensor(1);  // Confidence scores
        auto boxes = tflite_engine_->getOutputTensor(0);   // Bounding boxes + landmarks
        
        if (scores.empty() || boxes.empty()) {
            Log::debug("FaceDetector", "No detection outputs");
            return results;
        }
        
        results = parseMediaPipeOutput(image, scores, boxes);
        
        // Debug logging
        if (!results.empty()) {
            Log::info("FaceDetector", "Detected " + std::to_string(results.size()) + " faces");
        }
        
    } catch (const std::exception& e) {
        Log::error("FaceDetector", std::string("Exception in detectFacesTFLite: ") + e.what());
    }
    
    return results;
}

std::vector<FaceBox> FaceDetector::parseMediaPipeOutput(const cv::Mat& image, 
                                                       const std::vector<float>& scores,
                                                       const std::vector<float>& boxes) {
    std::vector<FaceBox> results;
    if (scores.empty() || boxes.empty()) {
        return results;
    }
    
    int num_detections = static_cast<int>(scores.size());
    std::vector<FaceBox> detections;
    
    for (int i = 0; i < num_detections; ++i) {
        float score = sigmoidf(scores[i]);
        
        // Use lower confidence threshold for more sensitive detection
        if (score < min_confidence_) continue;
        
        // MediaPipe format: 16 values per detection [y_center, x_center, h, w, 6 landmarks]
        int base_idx = i * 16;
        
        if (base_idx + 15 >= static_cast<int>(boxes.size())) {
            break;
        }
        
        // Extract normalized coordinates
        float y_center = boxes[base_idx];
        float x_center = boxes[base_idx + 1];
        float h = boxes[base_idx + 2];
        float w = boxes[base_idx + 3];
        
        // Convert to pixel coordinates
        int x1 = static_cast<int>((x_center - w/2.0f) * image.cols);
        int y1 = static_cast<int>((y_center - h/2.0f) * image.rows);
        int x2 = static_cast<int>((x_center + w/2.0f) * image.cols);
        int y2 = static_cast<int>((y_center + h/2.0f) * image.rows);
        
        // Clamp to image boundaries
        x1 = std::max(0, std::min(x1, image.cols - 1));
        y1 = std::max(0, std::min(y1, image.rows - 1));
        x2 = std::max(0, std::min(x2, image.cols - 1));
        y2 = std::max(0, std::min(y2, image.rows - 1));
        
        int width = x2 - x1;
        int height = y2 - y1;
        
        // Validate box dimensions
        if (width <= 10 || height <= 10) {
            continue;
        }
        
        FaceBox box;
        box.x = x1;
        box.y = y1;
        box.width = width;
        box.height = height;
        box.confidence = score;
        
        // Extract 6 facial landmarks
        for (int k = 0; k < 6; ++k) {
            Point landmark;
            landmark.x = boxes[base_idx + 4 + k*2] * image.cols;
            landmark.y = boxes[base_idx + 5 + k*2] * image.rows;
            box.landmarks.push_back(landmark);
        }
        
        detections.push_back(box);
        
        // Debug: Log detection details
        Log::debug("FaceDetector", 
            "Face detected: x=" + std::to_string(box.x) + 
            " y=" + std::to_string(box.y) + 
            " w=" + std::to_string(box.width) + 
            " h=" + std::to_string(box.height) + 
            " conf=" + std::to_string(box.confidence));
    }
    
    // Apply Non-Maximum Suppression with relaxed IOU threshold
    if (!detections.empty()) {
        results = nonMaxSuppression(detections, 0.4f); // Lower IOU threshold
        Log::debug("FaceDetector", "After NMS: " + std::to_string(results.size()) + " faces");
    }
    
    return results;
}

float FaceDetector::calculateIOU(const FaceBox& a, const FaceBox& b) {
    int inter_x1 = std::max(a.x, b.x);
    int inter_y1 = std::max(a.y, b.y);
    int inter_x2 = std::min(a.x + a.width, b.x + b.width);
    int inter_y2 = std::min(a.y + a.height, b.y + b.height);
    
    int inter_width = std::max(0, inter_x2 - inter_x1);
    int inter_height = std::max(0, inter_y2 - inter_y1);
    float inter_area = static_cast<float>(inter_width * inter_height);
    
    float area_a = static_cast<float>(a.width * a.height);
    float area_b = static_cast<float>(b.width * b.height);
    
    return inter_area / (area_a + area_b - inter_area + 1e-6f);
}

std::vector<FaceBox> FaceDetector::nonMaxSuppression(std::vector<FaceBox>& boxes, float iou_threshold) {
    if (boxes.empty()) return boxes;
    
    // Sort by confidence descending
    std::sort(boxes.begin(), boxes.end(), 
              [](const FaceBox& a, const FaceBox& b) { 
                  return a.confidence > b.confidence; 
              });
    
    std::vector<FaceBox> kept;
    std::vector<bool> suppressed(boxes.size(), false);
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (suppressed[i]) continue;
        kept.push_back(boxes[i]);
        
        for (size_t j = i + 1; j < boxes.size(); ++j) {
            if (suppressed[j]) continue;
            float iou = calculateIOU(boxes[i], boxes[j]);
            if (iou > iou_threshold) {
                suppressed[j] = true;
            }
        }
    }
    
    return kept;
}

InferenceResult FaceDetector::processFrame(const uint8_t* encoded_data, size_t encoded_size,
                                          const std::string& codec,
                                          int expected_width, int expected_height) {
    InferenceResult result;
    result.faces_detected = 0;
    result.timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    if (!encoded_data || encoded_size == 0 || encoded_size > 200000) {
        return result;
    }

    bool is_keyframe = isVP8Keyframe(encoded_data, encoded_size);
    
    std::lock_guard<std::mutex> lock(decoder_mutex);

    // Initialize decoder if needed
    if (!decoder_initialized || current_codec != codec) {
        cleanupDecoder();
        if (!initializeDecoder(codec)) {
            return result;
        }
        decoder_initialized = true;
        current_codec = codec;
    }

    // Wait for keyframe before processing P-frames
    if (!has_received_keyframe && !is_keyframe) {
        Log::debug("FaceDetector", "Waiting for keyframe, skipping P-frame");
        return result;
    }

    // Prepare and send packet to decoder
    av_packet_unref(packet);
    
    if (av_new_packet(packet, encoded_size) < 0) {
        consecutive_failures++;
        return result;
    }
    
    memcpy(packet->data, encoded_data, encoded_size);
    packet->size = encoded_size;
    
    if (is_keyframe) {
        packet->flags |= AV_PKT_FLAG_KEY;
        Log::debug("FaceDetector", "Processing keyframe");
    }

    // Send to decoder
    int send_ret = avcodec_send_packet(decoder_ctx, packet);
    if (send_ret < 0) {
        consecutive_failures++;
        if (consecutive_failures > 5) {
            resetDecoder();
        }
        return result;
    }

    // Receive decoded frame
    av_frame_unref(frame);
    int recv_ret = avcodec_receive_frame(decoder_ctx, frame);
    
    if (recv_ret == AVERROR(EAGAIN)) {
        return result;
    } else if (recv_ret < 0) {
        return result;
    }

    // Successfully decoded
    if (is_keyframe) {
        has_received_keyframe = true;
    }
    
    consecutive_failures = 0;
    
    processDecodedFrame(frame, result);
    
    return result;
}

int FaceDetector::processDecodedFrame(AVFrame* frame, InferenceResult& result) {
    if (!frame || frame->width <= 0 || frame->height <= 0) {
        return -1;
    }

    // Convert to BGR for OpenCV
    SwsContext* swsCtx = sws_getContext(
        frame->width, frame->height, (AVPixelFormat)frame->format,
        frame->width, frame->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
        
    if (!swsCtx) {
        return -1;
    }

    int stride = 3 * frame->width;
    std::vector<uint8_t> bgr_buf(stride * frame->height);
    uint8_t* dst[4] = { bgr_buf.data(), nullptr, nullptr, nullptr };
    int dst_stride[4] = { stride, 0, 0, 0 };

    sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dst, dst_stride);
    sws_freeContext(swsCtx);

    // Create OpenCV Mat and run detection
    cv::Mat img(frame->height, frame->width, CV_8UC3, bgr_buf.data(), stride);
    if (img.empty()) {
        return -1;
    }

    auto faces = detectFacesTFLite(img);
    result.bounding_boxes = faces;
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

    FaceDetector* create_detector_with_model(const char* model_path) {
        FaceDetector* d = new FaceDetector();
        std::string path = model_path ? std::string(model_path) : std::string();
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
        
        std::string codec_str = codec ? std::string(codec) : "vp8";
        InferenceResult result = detector->processFrame(encoded_data, encoded_size, codec_str, 
                                                       expected_width, expected_height);
        
        *faces_detected = result.faces_detected;
        *timestamp = result.timestamp;
    }
}