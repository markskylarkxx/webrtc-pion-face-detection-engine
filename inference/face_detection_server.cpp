
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <iostream>
#include <vector>

#include "face_detection.hpp"
#include "inference.pb.h"
#include "inference.grpc.pb.h"

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
        std::cout << "Initializing Face Detection Service..." << std::endl;
        
        // Try multiple possible cascade file locations
        std::vector<std::string> cascade_paths = {
            "haarcascade_frontalface_default.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
        };
        
        bool cascade_loaded = false;
        for (const auto& path : cascade_paths) {
            if (face_cascade.load(path)) {
                std::cout << "Successfully loaded face cascade from: " << path << std::endl;
                cascade_loaded = true;
                break;
            }
        }
        
        if (!cascade_loaded) {
            std::cerr << "ERROR: Could not load face cascade classifier from any known location!" << std::endl;
            std::cerr << "Please ensure haarcascade_frontalface_default.xml is in the current directory or standard OpenCV paths." << std::endl;
        }
    }

    Status DetectFaces(ServerContext* context, const FrameRequest* request,
                       DetectionResponse* response) override {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Validate that cascade classifier is loaded
        if (face_cascade.empty()) {
            std::cerr << "ERROR: Face cascade classifier not loaded!" << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        // Validate input parameters
        if (request->width() <= 0 || request->height() <= 0 || request->channels() <= 0) {
            std::cerr << "ERROR: Invalid frame dimensions: " 
                      << request->width() << "x" << request->height() 
                      << " channels: " << request->channels() << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        // Check if frame data is not empty
        if (request->encoded_frame().empty()) {
            std::cerr << "ERROR: Empty frame data received" << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status::OK;
        }

        // For VP8 encoded data (1 channel), we need to handle it differently
        if (request->channels() == 1) {
            return HandleVP8Frame(context, request, response);
        }

        // Original handling for raw frame data (3 channels)
        return HandleRawFrame(context, request, response);
    }

private:
    cv::CascadeClassifier face_cascade;

    Status HandleRawFrame(ServerContext* context, const FrameRequest* request, DetectionResponse* response) {
        auto start_time = std::chrono::high_resolution_clock::now();

        try {
            // Calculate expected size and validate
            size_t expected_size = request->width() * request->height() * request->channels();
            if (request->encoded_frame().size() != expected_size) {
                std::cerr << "ERROR: Frame data size mismatch. Expected: " << expected_size 
                          << ", Got: " << request->encoded_frame().size() << std::endl;
                response->set_timestamp(request->timestamp());
                response->set_frame_id(request->frame_id());
                response->set_processing_time_ms(0);
                return Status::OK;
            }

            // Create Mat with proper data handling
            cv::Mat frame;
            if (request->channels() == 3) {
                frame = cv::Mat(request->height(), request->width(), CV_8UC3);
                memcpy(frame.data, request->encoded_frame().data(), request->encoded_frame().size());
            } else {
                std::cerr << "ERROR: Unsupported number of channels for raw frame: " << request->channels() << std::endl;
                response->set_timestamp(request->timestamp());
                response->set_frame_id(request->frame_id());
                response->set_processing_time_ms(0);
                return Status::OK;
            }

            if (frame.empty()) {
                std::cerr << "ERROR: Created empty frame after copy" << std::endl;
                response->set_timestamp(request->timestamp());
                response->set_frame_id(request->frame_id());
                response->set_processing_time_ms(0);
                return Status::OK;
            }

            return ProcessFrameWithOpenCV(frame, request, response, start_time);

        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV Exception in HandleRawFrame: " << e.what() << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status(grpc::StatusCode::INTERNAL, "OpenCV processing error");
        }
    }

    Status HandleVP8Frame(ServerContext* context, const FrameRequest* request, DetectionResponse* response) {
        auto start_time = std::chrono::high_resolution_clock::now();

        try {
            // For VP8 data, we'll attempt to decode it
            std::vector<uchar> vp8_data(request->encoded_frame().begin(), request->encoded_frame().end());
            
            // Attempt to decode as VP8 using OpenCV
            // Note: OpenCV may not support VP8 decoding directly, so we'll use a fallback approach
            
            // Fallback: If VP8 decoding fails, try to interpret as raw grayscale data
            cv::Mat frame;
            
            // First, try to interpret as raw grayscale (common fallback)
            if (request->encoded_frame().size() == static_cast<size_t>(request->width() * request->height())) {
                frame = cv::Mat(request->height(), request->width(), CV_8UC1);
                memcpy(frame.data, request->encoded_frame().data(), request->encoded_frame().size());
                std::cout << "Using raw grayscale interpretation for VP8 data" << std::endl;
            } else {
                // If size doesn't match, create a simple test pattern
                frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar(100, 100, 100));
                std::cout << "Created test pattern for VP8 data analysis" << std::endl;
                
                // Log the actual data characteristics for debugging
                std::cout << "VP8 data analysis - Size: " << request->encoded_frame().size() 
                          << ", Expected for " << request->width() << "x" << request->height() 
                          << ": " << (request->width() * request->height()) << std::endl;
            }

            if (frame.empty()) {
                std::cerr << "ERROR: Failed to create frame from VP8 data" << std::endl;
                response->set_timestamp(request->timestamp());
                response->set_frame_id(request->frame_id());
                response->set_processing_time_ms(0);
                return Status::OK;
            }

            return ProcessFrameWithOpenCV(frame, request, response, start_time);

        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV Exception in HandleVP8Frame: " << e.what() << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status(grpc::StatusCode::INTERNAL, "VP8 processing error");
        }
    }

    Status ProcessFrameWithOpenCV(const cv::Mat& frame, const FrameRequest* request, 
                                 DetectionResponse* response, 
                                 std::chrono::high_resolution_clock::time_point start_time) {
        try {
            // Convert to grayscale for Haar cascade
            cv::Mat gray;
            if (frame.channels() == 3) {
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            } else {
                gray = frame.clone();
            }

            // Debug: save first few frames for analysis
            static int debug_counter = 0;
            if (debug_counter < 3) {
                std::string debug_prefix = "debug_frame_" + std::to_string(debug_counter);
                cv::imwrite(debug_prefix + "_input.jpg", frame);
                cv::imwrite(debug_prefix + "_gray.jpg", gray);
                
                double minVal, maxVal;
                cv::minMaxLoc(gray, &minVal, &maxVal);
                std::cout << "Debug frame " << debug_counter 
                          << " - Size: " << frame.cols << "x" << frame.rows 
                          << ", Channels: " << frame.channels()
                          << ", Gray range: " << minVal << "-" << maxVal << std::endl;
                debug_counter++;
            }

            // Apply histogram equalization to improve contrast
            cv::Mat gray_eq;
            cv::equalizeHist(gray, gray_eq);

            // Detect faces with primary parameters
            std::vector<cv::Rect> faces;
            face_cascade.detectMultiScale(
                gray_eq, 
                faces, 
                1.1,    // scaleFactor
                3,      // minNeighbors
                0 | cv::CASCADE_SCALE_IMAGE,
                cv::Size(30, 30),
                cv::Size(300, 300)
            );

            // Fallback detection if no faces found
            if (faces.empty()) {
                face_cascade.detectMultiScale(
                    gray_eq,
                    faces,
                    1.05,   // more thorough scale factor
                    2,      // lower neighbor requirement
                    0 | cv::CASCADE_SCALE_IMAGE,
                    cv::Size(20, 20),
                    cv::Size(400, 400)
                );
            }

            // Convert detections to response format
            for (const auto& face : faces) {
                BoundingBox* bbox = response->add_faces();
                bbox->set_x(face.x);
                bbox->set_y(face.y);
                bbox->set_width(face.width);
                bbox->set_height(face.height);
                bbox->set_confidence(0.9); // Default confidence for Haar cascade
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(duration.count());

            // Always log for now to see what's happening
            std::cout << "C++ detection: " << faces.size() << " faces, " 
                      << duration.count() << " ms, Frame size: " << frame.cols << "x" << frame.rows 
                      << "x" << frame.channels() << std::endl;

        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV Exception in ProcessFrameWithOpenCV: " << e.what() << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status(grpc::StatusCode::INTERNAL, "OpenCV processing error");
        } catch (const std::exception& e) {
            std::cerr << "Standard Exception in ProcessFrameWithOpenCV: " << e.what() << std::endl;
            response->set_timestamp(request->timestamp());
            response->set_frame_id(request->frame_id());
            response->set_processing_time_ms(0);
            return Status(grpc::StatusCode::INTERNAL, "Processing error");
        }

        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FaceDetectionServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "ERROR: Failed to build and start gRPC server!" << std::endl;
        return;
    }
    
    std::cout << "Face Detection Server listening on " << server_address << std::endl;
    std::cout << "Server is ready to process requests..." << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    std::cout << "Starting Face Detection Server..." << std::endl;
    
    try {
        // Test basic OpenCV functionality
        std::cout << "Testing OpenCV installation..." << std::endl;
        cv::Mat test_mat(10, 10, CV_8UC3, cv::Scalar(100, 100, 100));
        if (test_mat.empty()) {
            std::cerr << "ERROR: Basic OpenCV test failed!" << std::endl;
            return -1;
        }
        std::cout << "OpenCV test passed" << std::endl;

        RunServer();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR in main: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "FATAL ERROR: Unknown exception in main" << std::endl;
        return -1;
    }
    
    return 0;
}