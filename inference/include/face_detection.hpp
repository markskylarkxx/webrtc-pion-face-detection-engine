








// //DECODING IN C++ SERVER
// #ifndef FACE_DETECTION_HPP
// #define FACE_DETECTION_HPP

// #include <string>
// #include <vector>
// #include <mutex>
// #include <opencv2/opencv.hpp>

// // Include FFmpeg headers
// extern "C" {
// #include <libavcodec/avcodec.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/pixfmt.h>
// #include <libavutil/opt.h>
// }

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
    
//     bool initialize(const std::string& cascade_path = "");
//     void cleanup();
//     InferenceResult processFrame(const uint8_t* encoded_data, size_t encoded_size,
//                                 const std::string& codec = "vp8",
//                                 int expected_width = 640, int expected_height = 480);
//     void resetDecoder();

// private:
//     bool initializeDecoder(const std::string& codec);
//     void cleanupDecoder();
//     int processDecodedFrame(AVFrame* frame, InferenceResult& result);
//     bool isVP8Keyframe(const uint8_t* data, size_t size);
//     AVCodecID pickCodecIdFromString(const std::string& codec);
    
//     // FFmpeg components
//     AVCodecContext* decoder_ctx;
//     AVPacket* packet;
//     AVFrame* frame;
    
//     // OpenCV components
//     cv::CascadeClassifier face_cascade;
    
//     // State management
//     std::mutex decoder_mutex;
//     bool decoder_initialized;
//     std::string current_codec;
    
//     // Frame processing
//     int frame_counter;
//     int process_every_n_frames;
    
//     // FIXED: Keyframe tracking and error recovery
//     bool has_received_keyframe;
//     int consecutive_failures;
// };

// // C-style interface
// extern "C" {
//     FaceDetector* create_detector();
//     FaceDetector* create_detector_with_cascade(const char* cascade_path);
//     void destroy_detector(FaceDetector* detector);
//     void reset_detector(FaceDetector* detector);
//     void process_frame_c(FaceDetector* detector,
//                         const uint8_t* encoded_data,
//                         size_t encoded_size,
//                         const char* codec,
//                         int expected_width,
//                         int expected_height,
//                         int* faces_detected,
//                         int64_t* timestamp);
// }

// #endif // FACE_DETECTION_HPP



























//DECODING IN C++ SERVER
#ifndef FACE_DETECTION_HPP
#define FACE_DETECTION_HPP

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <opencv2/opencv.hpp>

// Include FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

// Forward declaration to avoid including TfLiteEngine.h here
namespace neptune {
    class TfLiteEngine;
}

struct Point {
    float x;
    float y;
};

struct FaceBox {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    std::vector<Point> landmarks;  // 6 facial landmarks
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
    
    bool initialize(const std::string& model_path = "");
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
    
    // MediaPipe TFLite face detection
    std::vector<FaceBox> detectFacesTFLite(const cv::Mat& image);
    std::vector<FaceBox> parseMediaPipeOutput(const cv::Mat& image, 
                                             const std::vector<float>& scores,
                                             const std::vector<float>& boxes_and_keypoints);
    std::vector<FaceBox> nonMaxSuppression(std::vector<FaceBox>& boxes, float iou_threshold = 0.3f);
    float calculateIOU(const FaceBox& a, const FaceBox& b);
    
    // FFmpeg components
    AVCodecContext* decoder_ctx;
    AVPacket* packet;
    AVFrame* frame;
    
    // Neptune TFLite engine
    std::unique_ptr<neptune::TfLiteEngine> tflite_engine_;
    float min_confidence_ = 0.7f;
    
    // State management
    std::mutex decoder_mutex;
    bool decoder_initialized;
    std::string current_codec;
    
    // Keyframe tracking and error recovery
    bool has_received_keyframe;
    int consecutive_failures;
};

// C-style interface
extern "C" {
    FaceDetector* create_detector();
    FaceDetector* create_detector_with_model(const char* model_path);
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