










//DECODING IN C++ SERVER
// #include <iostream>
// #include <memory>
// #include <string>
// #include <chrono>
// #include <grpcpp/grpcpp.h>
// #include <opencv2/opencv.hpp>

// #include "inference.grpc.pb.h"
// #include "face_detection.hpp"

// using grpc::Server;
// using grpc::ServerBuilder;
// using grpc::ServerContext;
// using grpc::Status;
// using inference::FaceDetection;
// using inference::FrameRequest;
// using inference::DetectionResponse;
// using inference::BoundingBox;

// class FaceDetectionServiceImpl final : public FaceDetection::Service {
// public:
//     FaceDetectionServiceImpl(const std::string& cascade_path = "") {
//         std::cout << "🔄 Creating FaceDetectionServiceImpl..." << std::endl;
//         detector = std::make_unique<FaceDetector>();

//         if (!cascade_path.empty()) {
//             if (!detector->initialize(cascade_path)) {
//                 std::cerr << "❌ ERROR: Failed to initialize face detector with cascade: "
//                           << cascade_path << std::endl;
//             }
//         } else {
//             if (!detector->initialize()) {
//                 std::cerr << "❌ ERROR: Failed to initialize face detector" << std::endl;
//             }
//         }

//         std::cout << "✅ Face detector initialization attempt finished" << std::endl;
//     }

//     Status DetectFaces(ServerContext* context,
//                        const FrameRequest* request,
//                        DetectionResponse* response) override {
//         // Extract encoded frame + metadata
//         const std::string& encoded_frame = request->encoded_frame();
//         const std::string codec = request->codec();
//         int width = request->width();
//         int height = request->height();
//         int64_t timestamp = request->timestamp();
//         std::string frame_id = request->frame_id();
//         size_t frame_data_size = encoded_frame.size();

//         std::cout << "\n=== NEW ENCODED FRAME RECEIVED ===" << std::endl;
//         std::cout << "Frame ID: " << frame_id << std::endl;
//         std::cout << "Codec: " << codec << std::endl;
//         std::cout << "Encoded size: " << frame_data_size << " bytes" << std::endl;
//         std::cout << "Expected dim (may be 0): " << width << "x" << height << std::endl;

//         if (frame_data_size == 0) {
//             std::cerr << "❌ Empty encoded frame received" << std::endl;
//             response->set_timestamp(timestamp);
//             response->set_frame_id(frame_id);
//             response->set_processing_time_ms(0);
//             return Status::OK;
//         }

//         auto start_time = std::chrono::high_resolution_clock::now();

//         // Call detector which will decode using FFmpeg and run detection
//         InferenceResult res = detector->processFrame(
//             reinterpret_cast<const uint8_t*>(encoded_frame.data()),
//             encoded_frame.size(),
//             codec,
//             width,
//             height
//         );

//         auto end_time = std::chrono::high_resolution_clock::now();
//         auto processing_time =
//             std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

//         // Populate response
//         response->set_timestamp(timestamp);
//         response->set_frame_id(frame_id);
//         response->set_processing_time_ms(processing_time.count());

//         for (const auto& face : res.bounding_boxes) {
//             BoundingBox* bbox = response->add_faces();
//             bbox->set_x(face.x);
//             bbox->set_y(face.y);
//             bbox->set_width(face.width);
//             bbox->set_height(face.height);
//             bbox->set_confidence(face.confidence);
//         }

//         if (res.faces_detected > 0) {
//             std::cout << "✅ Detected " << res.faces_detected << " faces in frame "
//                       << frame_id << " (" << processing_time.count() << " ms)" << std::endl;
//         } else {
//             std::cout << "ℹ️ No faces detected in frame " << frame_id
//                       << " (" << processing_time.count() << " ms)" << std::endl;
//         }

//         std::cout << "=== FRAME PROCESSING COMPLETED ===\n" << std::endl;
//         return Status::OK;
//     }

// private:
//     std::unique_ptr<FaceDetector> detector;
// };

// void RunServer(const std::string& cascade_path = "") {
//     std::string server_address("0.0.0.0:50051");
//     FaceDetectionServiceImpl service(cascade_path);

//     ServerBuilder builder;
//     // Listen on the given address without authentication
//     builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//     // Register service
//     builder.RegisterService(&service);

//     // Build & start
//     std::unique_ptr<Server> server(builder.BuildAndStart());
//     std::cout << "🎯 Face Detection Server listening on " << server_address << std::endl;
//     std::cout << "🚀 Server is ready to process requests..." << std::endl;

//     server->Wait();
// }

// int main(int argc, char** argv) {
//     std::string cascade = "";
//     if (argc >= 2) cascade = argv[1];

//     std::cout << "Starting Face Detection Server..." << std::endl;

//     // Test OpenCV installation
//     cv::Mat test_mat(10, 10, CV_8UC1);
//     if (test_mat.empty()) {
//         std::cerr << "❌ ERROR: OpenCV test failed - cannot create Mat" << std::endl;
//         return -1;
//     }
//     std::cout << "✅ OpenCV test passed" << std::endl;

//     RunServer(cascade);
//     return 0;
// }










#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>

#include "inference.grpc.pb.h"
#include "face_detection.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using inference::FaceDetection;
using inference::FrameRequest;
using inference::DetectionResponse;
using inference::BoundingBox;

class FaceDetectionServiceImpl final : public FaceDetection::Service {
public:
    FaceDetectionServiceImpl(const std::string& model_path = "") {
        std::cout << "🔄 Creating FaceDetectionServiceImpl..." << std::endl;
        detector = std::make_unique<FaceDetector>();

        if (!model_path.empty()) {
            if (!detector->initialize(model_path)) {
                std::cerr << "❌ ERROR: Failed to initialize face detector with model: "
                          << model_path << std::endl;
                throw std::runtime_error("Detector initialization failed");
            }
        } else {
            if (!detector->initialize()) {
                std::cerr << "❌ ERROR: Failed to initialize face detector with default model" << std::endl;
                throw std::runtime_error("Detector initialization failed");
            }
        }

        std::cout << "✅ Face detector initialized successfully with TFLite model" << std::endl;
        frame_count = 0;
    }

    Status DetectFaces(ServerContext* context,
                       const FrameRequest* request,
                       DetectionResponse* response) override {
        auto start_time = std::chrono::high_resolution_clock::now();

        const std::string& encoded_frame = request->encoded_frame();
        const std::string codec = request->codec();
        int width = request->width();
        int height = request->height();
        int64_t timestamp = request->timestamp();
        std::string frame_id = request->frame_id();

        if (encoded_frame.empty()) {
            response->set_timestamp(timestamp);
            response->set_frame_id(frame_id);
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        // Process frame
        InferenceResult res = detector->processFrame(
            reinterpret_cast<const uint8_t*>(encoded_frame.data()),
            encoded_frame.size(),
            codec,
            width,
            height
        );

        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Populate response
        response->set_timestamp(timestamp);
        response->set_frame_id(frame_id);
        response->set_processing_time_ms(processing_time.count());

        // Add bounding boxes
        for (const auto& face : res.bounding_boxes) {
            BoundingBox* bbox = response->add_faces();
            bbox->set_x(face.x);
            bbox->set_y(face.y);
            bbox->set_width(face.width);
            bbox->set_height(face.height);
            bbox->set_confidence(face.confidence);
        }

        // Compact logging
        frame_count++;
        if (res.faces_detected > 0) {
            std::cout << "✅ Frame " << frame_count << ": " << res.faces_detected 
                      << " face(s) detected (" << processing_time.count() << "ms)" << std::endl;
        } else {
            // Only log every 10th frame when no faces detected
            if (frame_count % 10 == 0) {
                std::cout << "⚪ Frame " << frame_count << ": No faces (" 
                          << processing_time.count() << "ms)" << std::endl;
            }
        }

        return Status::OK;
    }

private:
    std::unique_ptr<FaceDetector> detector;
    int frame_count;
};

void RunServer(const std::string& model_path = "") {
    std::string server_address("0.0.0.0:50051");
    
    try {
        FaceDetectionServiceImpl service(model_path);

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "🎯 Face Detection Server listening on " << server_address << std::endl;
        std::cout << "🚀 Server is ready to process requests..." << std::endl;
        std::cout << "💡 Using TFLite/MediaPipe face detection model" << std::endl;

        server->Wait();
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to start server: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    std::string model_path = "";
    if (argc >= 2) model_path = argv[1];

    std::cout << "Starting Face Detection Server with TFLite..." << std::endl;

    cv::Mat test_mat(10, 10, CV_8UC1);
    if (test_mat.empty()) {
        std::cerr << "❌ ERROR: OpenCV test failed" << std::endl;
        return -1;
    }
    std::cout << "✅ OpenCV test passed" << std::endl;

    RunServer(model_path);
    return 0;
}