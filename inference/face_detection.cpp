#include "face_detection.hpp"
#include <chrono>
#include <iostream>

FaceDetector::FaceDetector() : model_data(nullptr) {}

FaceDetector::~FaceDetector() { cleanup(); }

bool FaceDetector::initialize() {
    // Load model / initialize
    return true;
}

InferenceResult FaceDetector::processFrame(const uint8_t* frame_data, int width, int height, int channels) {
    InferenceResult result;
    result.faces_detected = 0;
    result.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return result;
}

void FaceDetector::cleanup() {
    if (model_data) {
        model_data = nullptr;
    }
}

FaceDetector* create_detector() {
    FaceDetector* detector = new FaceDetector();
    if (detector->initialize()) return detector;
    delete detector;
    return nullptr;
}

void destroy_detector(FaceDetector* detector) {
    delete detector;
}

InferenceResult process_frame(FaceDetector* detector, const uint8_t* data, int w, int h, int c) {
    if (detector) return detector->processFrame(data, w, h, c);
    InferenceResult empty{};
    empty.faces_detected = 0;
    empty.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return empty;
}
