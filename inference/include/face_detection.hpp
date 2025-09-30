// working with test pattern!;

// #ifndef FACE_DETECTION_HPP
// #define FACE_DETECTION_HPP

// #include <vector>
// #include <cstdint>
// #include <opencv2/opencv.hpp>

// struct FaceBox {
//     int x;
//     int y;
//     int width;
//     int height;
//     float confidence;
// };

// struct InferenceResult {
//     int faces_detected;
//     std::vector<FaceBox> bounding_boxes;
//     int64_t timestamp;
// };

// class FaceDetector {
// public:
//     FaceDetector();
//     ~FaceDetector();
    
//     bool initialize();
//     InferenceResult processFrame(const uint8_t* frame_data, int width, int height, int channels);
//     void cleanup();

// private:
//     cv::CascadeClassifier face_cascade;
//     bool loadHaarCascade();
// };

// // C-style interface
// FaceDetector* create_detector();
// void destroy_detector(FaceDetector* detector);
// InferenceResult process_frame(FaceDetector* detector, const uint8_t* data, int w, int h, int c);

// #endif // FACE_DETECTION_HPP































//DECODING IN C++ SERVER
#ifndef FACE_DETECTION_HPP
#define FACE_DETECTION_HPP

#include <string>
#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>

// Include FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>
}

struct FaceBox {
    int x;
    int y;
    int width;
    int height;
    float confidence;
};

struct InferenceResult {
    int faces_detected;
    std::vector<FaceBox> bounding_boxes;
    int64_t timestamp;
};

class FaceDetector {
public:
    FaceDetector();
    ~FaceDetector();
    
    bool initialize(const std::string& cascade_path = "");
    void cleanup();
    InferenceResult processFrame(const uint8_t* encoded_data, size_t encoded_size,
                                const std::string& codec = "vp8",
                                int expected_width = 640, int expected_height = 480);
    void resetDecoder();

private:
    bool initializeDecoder(const std::string& codec);
    void cleanupDecoder();
    int processDecodedFrame(AVFrame* frame, InferenceResult& result);
    bool isVP8Keyframe(const uint8_t* data, size_t size);
    AVCodecID pickCodecIdFromString(const std::string& codec);
    
    // FFmpeg components
    AVCodecContext* decoder_ctx;
    AVPacket* packet;
    AVFrame* frame;
    
    // OpenCV components
    cv::CascadeClassifier face_cascade;
    
    // State management
    std::mutex decoder_mutex;
    bool decoder_initialized;
    std::string current_codec;
    
    // Frame processing
    int frame_counter;
    int process_every_n_frames;
    
    // FIXED: Keyframe tracking and error recovery
    bool has_received_keyframe;
    int consecutive_failures;
};

// C-style interface
extern "C" {
    FaceDetector* create_detector();
    FaceDetector* create_detector_with_cascade(const char* cascade_path);
    void destroy_detector(FaceDetector* detector);
    void reset_detector(FaceDetector* detector);
    void process_frame_c(FaceDetector* detector,
                        const uint8_t* encoded_data,
                        size_t encoded_size,
                        const char* codec,
                        int expected_width,
                        int expected_height,
                        int* faces_detected,
                        int64_t* timestamp);
}

#endif // FACE_DETECTION_HPP