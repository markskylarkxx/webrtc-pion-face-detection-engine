#ifndef FACE_DETECTION_HPP
#define FACE_DETECTION_HPP

#include <vector>
#include <cstdint>

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
    
    bool initialize();
    InferenceResult processFrame(const uint8_t* frame_data, int width, int height, int channels);
    void cleanup();

private:
    void* model_data;
};

// Optional C-style interface (remove extern "C" if returning C++ types)
FaceDetector* create_detector();
void destroy_detector(FaceDetector* detector);
InferenceResult process_frame(FaceDetector* detector, const uint8_t* data, int w, int h, int c);

#endif // FACE_DETECTION_HPP
