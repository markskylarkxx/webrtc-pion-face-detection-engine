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
    FaceDetectionServiceImpl() {
        detector = std::make_unique<FaceDetector>();
        if (!detector->initialize()) {
            std::cerr << "ERROR: Failed to initialize face detector" << std::endl;
        } else {
            std::cout << "Face detector initialized successfully" << std::endl;
        }
    }

    Status DetectFaces(ServerContext* context, const FrameRequest* request,
                      DetectionResponse* response) override {
        
        // Extract frame parameters from request - use the correct field names
        int width = request->width();        // Field 1 in proto
        int height = request->height();      // Field 2 in proto  
        int channels = request->channels();  // Field 3 in proto
        int64_t timestamp = request->timestamp();  // Field 4 in proto
        std::string frame_id = request->frame_id(); // Field 5 in proto
        std::string codec = request->codec();      // Field 6 in proto
        const std::string& encoded_frame = request->encoded_frame(); // Field 7 in proto
        size_t frame_data_size = encoded_frame.size();

        // Debug log to see what we're receiving
        std::cout << "Received frame: " << frame_id 
                  << " | Size: " << width << "x" << height 
                  << " | Channels: " << channels 
                  << " | Data size: " << frame_data_size 
                  << " | Codec: " << codec << std::endl;

        // Validate frame dimensions and data size
        int expected_size = width * height * channels;
        
        if (frame_data_size != expected_size) {
            std::cerr << "ERROR: Frame data size mismatch. Expected: " << expected_size 
                      << ", Got: " << frame_data_size 
                      << " (Width: " << width << ", Height: " << height 
                      << ", Channels: " << channels << ")" << std::endl;
            
            // Return empty response but don't fail the request entirely
            response->set_timestamp(timestamp);
            response->set_frame_id(frame_id);
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        // Process the frame for face detection
        auto start_time = std::chrono::high_resolution_clock::now();
        
        InferenceResult result = detector->processFrame(
            reinterpret_cast<const uint8_t*>(encoded_frame.data()), 
            width, height, channels
        );

        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Populate the response
        response->set_timestamp(timestamp);
        response->set_frame_id(frame_id);
        response->set_processing_time_ms(processing_time.count());

        // Add bounding boxes to response
        for (const auto& face : result.bounding_boxes) {
            BoundingBox* bbox = response->add_faces();
            bbox->set_x(face.x);
            bbox->set_y(face.y);
            bbox->set_width(face.width);
            bbox->set_height(face.height);
            bbox->set_confidence(face.confidence);
        }

        if (result.faces_detected > 0) {
            std::cout << "Detected " << result.faces_detected << " faces in frame " 
                      << frame_id << " (" << processing_time.count() << " ms)" << std::endl;
        } else {
            std::cout << "No faces detected in frame " << frame_id 
                      << " (" << processing_time.count() << " ms)" << std::endl;
        }

        return Status::OK;
    }

private:
    std::unique_ptr<FaceDetector> detector;
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FaceDetectionServiceImpl service;

    ServerBuilder builder;
    
    // Listen on the given address without any authentication mechanism
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    
    // Register "service" as the instance through which we'll communicate with clients
    builder.RegisterService(&service);
    
    // Finally assemble the server
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Face Detection Server listening on " << server_address << std::endl;
    std::cout << "Server is ready to process requests..." << std::endl;

    // Wait for the server to shutdown
    server->Wait();
}

int main(int argc, char** argv) {
    std::cout << "Starting Face Detection Server..." << std::endl;
    
    // Test OpenCV installation
    std::cout << "Testing OpenCV installation..." << std::endl;
    cv::Mat test_mat(10, 10, CV_8UC1);
    if (test_mat.empty()) {
        std::cerr << "ERROR: OpenCV test failed - cannot create Mat" << std::endl;
        return -1;
    }
    std::cout << "OpenCV test passed" << std::endl;
    
    std::cout << "Initializing Face Detection Service..." << std::endl;
    
    RunServer();
    return 0;
}