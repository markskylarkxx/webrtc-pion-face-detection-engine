
//working with test pattern
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
//     FaceDetectionServiceImpl() {
//         std::cout << "🔄 Creating FaceDetectionServiceImpl..." << std::endl;
//         detector = std::make_unique<FaceDetector>();
//         if (!detector->initialize()) {
//             std::cerr << "❌ ERROR: Failed to initialize face detector" << std::endl;
//         } else {
//             std::cout << "✅ Face detector initialized successfully" << std::endl;
//         }
//     }

//     Status DetectFaces(ServerContext* context, const FrameRequest* request,
//                       DetectionResponse* response) override {
        
//         // Extract frame parameters from request
//         int width = request->width();
//         int height = request->height();  
//         int channels = request->channels();
//         int64_t timestamp = request->timestamp();
//         std::string frame_id = request->frame_id();
//         std::string codec = request->codec();
//         const std::string& encoded_frame = request->encoded_frame();
//         size_t frame_data_size = encoded_frame.size();

//         // ✅ ADDED: Detailed debug output
//         std::cout << "\n=== NEW FRAME RECEIVED ===" << std::endl;
//         std::cout << "Frame ID: " << frame_id << std::endl;
//         std::cout << "Dimensions: " << width << "x" << height << "x" << channels << std::endl;
//         std::cout << "Data size: " << frame_data_size << " bytes" << std::endl;
//         std::cout << "Expected size: " << (width * height * channels) << " bytes" << std::endl;
        
//         // Check first few bytes to see if data looks reasonable
//         std::cout << "First 10 bytes: ";
//         for (int i = 0; i < std::min(10, (int)frame_data_size); i++) {
//             printf("%02x ", (unsigned char)encoded_frame[i]);
//         }
//         std::cout << std::endl;
        
//         // Check if data is all zeros
//         bool all_zeros = true;
//         for (int i = 0; i < std::min(100, (int)frame_data_size); i++) {
//             if (encoded_frame[i] != 0) {
//                 all_zeros = false;
//                 break;
//             }
//         }
//         std::cout << "Data all zeros: " << (all_zeros ? "YES" : "NO") << std::endl;

//         // Validate frame dimensions and data size
//         int expected_size = width * height * channels;
        
//         if (frame_data_size != expected_size) {
//             std::cerr << "❌ ERROR: Frame data size mismatch. Expected: " << expected_size 
//                       << ", Got: " << frame_data_size << std::endl;
            
//             // Return empty response but don't fail the request entirely
//             response->set_timestamp(timestamp);
//             response->set_frame_id(frame_id);
//             response->set_processing_time_ms(0);
//             std::cout << "=== FRAME PROCESSING ABORTED ===" << std::endl;
//             return Status::OK;
//         }

//         std::cout << "✅ Frame validation passed. Starting face detection..." << std::endl;

//         // Process the frame for face detection
//         auto start_time = std::chrono::high_resolution_clock::now();
        
//         InferenceResult result = detector->processFrame(
//             reinterpret_cast<const uint8_t*>(encoded_frame.data()), 
//             width, height, channels
//         );

//         auto end_time = std::chrono::high_resolution_clock::now();
//         auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

//         // Populate the response
//         response->set_timestamp(timestamp);
//         response->set_frame_id(frame_id);
//         response->set_processing_time_ms(processing_time.count());

//         // Add bounding boxes to response
//         for (const auto& face : result.bounding_boxes) {
//             BoundingBox* bbox = response->add_faces();
//             bbox->set_x(face.x);
//             bbox->set_y(face.y);
//             bbox->set_width(face.width);
//             bbox->set_height(face.height);
//             bbox->set_confidence(face.confidence);
//         }

//         if (result.faces_detected > 0) {
//             std::cout << "✅ Detected " << result.faces_detected << " faces in frame " 
//                       << frame_id << " (" << processing_time.count() << " ms)" << std::endl;
//         } else {
//             std::cout << "❌ No faces detected in frame " << frame_id 
//                       << " (" << processing_time.count() << " ms)" << std::endl;
//         }
        
//         std::cout << "=== FRAME PROCESSING COMPLETED ===\n" << std::endl;

//         return Status::OK;
//     }

// private:
//     std::unique_ptr<FaceDetector> detector;
// };

// void RunServer() {
//     std::string server_address("0.0.0.0:50051");
//     FaceDetectionServiceImpl service;

//     ServerBuilder builder;
    
//     // Listen on the given address without any authentication mechanism
//     builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    
//     // Register "service" as the instance through which we'll communicate with clients
//     builder.RegisterService(&service);
    
//     // Finally assemble the server
//     std::unique_ptr<Server> server(builder.BuildAndStart());
//     std::cout << "🎯 Face Detection Server listening on " << server_address << std::endl;
//     std::cout << "🚀 Server is ready to process requests..." << std::endl;
//     std::cout << "💡 Make sure haarcascade_frontalface_default.xml is in the current directory" << std::endl;

//     // Wait for the server to shutdown
//     server->Wait();
// }

// int main(int argc, char** argv) {
//     std::cout << "Starting Face Detection Server..." << std::endl;
    
//     // Test OpenCV installation
//     std::cout << "Testing OpenCV installation..." << std::endl;
//     cv::Mat test_mat(10, 10, CV_8UC1);
//     if (test_mat.empty()) {
//         std::cerr << "❌ ERROR: OpenCV test failed - cannot create Mat" << std::endl;
//         return -1;
//     }
//     std::cout << "✅ OpenCV test passed" << std::endl;
    
//     std::cout << "Initializing Face Detection Service..." << std::endl;
    
//     RunServer();
//     return 0;
// }
















//DECODING IN C++ SERVER
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
    FaceDetectionServiceImpl(const std::string& cascade_path = "") {
        std::cout << "🔄 Creating FaceDetectionServiceImpl..." << std::endl;
        detector = std::make_unique<FaceDetector>();

        if (!cascade_path.empty()) {
            if (!detector->initialize(cascade_path)) {
                std::cerr << "❌ ERROR: Failed to initialize face detector with cascade: "
                          << cascade_path << std::endl;
            }
        } else {
            if (!detector->initialize()) {
                std::cerr << "❌ ERROR: Failed to initialize face detector" << std::endl;
            }
        }

        std::cout << "✅ Face detector initialization attempt finished" << std::endl;
    }

    Status DetectFaces(ServerContext* context,
                       const FrameRequest* request,
                       DetectionResponse* response) override {
        // Extract encoded frame + metadata
        const std::string& encoded_frame = request->encoded_frame();
        const std::string codec = request->codec();
        int width = request->width();
        int height = request->height();
        int64_t timestamp = request->timestamp();
        std::string frame_id = request->frame_id();
        size_t frame_data_size = encoded_frame.size();

        std::cout << "\n=== NEW ENCODED FRAME RECEIVED ===" << std::endl;
        std::cout << "Frame ID: " << frame_id << std::endl;
        std::cout << "Codec: " << codec << std::endl;
        std::cout << "Encoded size: " << frame_data_size << " bytes" << std::endl;
        std::cout << "Expected dim (may be 0): " << width << "x" << height << std::endl;

        if (frame_data_size == 0) {
            std::cerr << "❌ Empty encoded frame received" << std::endl;
            response->set_timestamp(timestamp);
            response->set_frame_id(frame_id);
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Call detector which will decode using FFmpeg and run detection
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

        for (const auto& face : res.bounding_boxes) {
            BoundingBox* bbox = response->add_faces();
            bbox->set_x(face.x);
            bbox->set_y(face.y);
            bbox->set_width(face.width);
            bbox->set_height(face.height);
            bbox->set_confidence(face.confidence);
        }

        if (res.faces_detected > 0) {
            std::cout << "✅ Detected " << res.faces_detected << " faces in frame "
                      << frame_id << " (" << processing_time.count() << " ms)" << std::endl;
        } else {
            std::cout << "ℹ️ No faces detected in frame " << frame_id
                      << " (" << processing_time.count() << " ms)" << std::endl;
        }

        std::cout << "=== FRAME PROCESSING COMPLETED ===\n" << std::endl;
        return Status::OK;
    }

private:
    std::unique_ptr<FaceDetector> detector;
};

void RunServer(const std::string& cascade_path = "") {
    std::string server_address("0.0.0.0:50051");
    FaceDetectionServiceImpl service(cascade_path);

    ServerBuilder builder;
    // Listen on the given address without authentication
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register service
    builder.RegisterService(&service);

    // Build & start
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "🎯 Face Detection Server listening on " << server_address << std::endl;
    std::cout << "🚀 Server is ready to process requests..." << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    std::string cascade = "";
    if (argc >= 2) cascade = argv[1];

    std::cout << "Starting Face Detection Server..." << std::endl;

    // Test OpenCV installation
    cv::Mat test_mat(10, 10, CV_8UC1);
    if (test_mat.empty()) {
        std::cerr << "❌ ERROR: OpenCV test failed - cannot create Mat" << std::endl;
        return -1;
    }
    std::cout << "✅ OpenCV test passed" << std::endl;

    RunServer(cascade);
    return 0;
}
