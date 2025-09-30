#WORKING!!!

# cmake_minimum_required(VERSION 3.10)
# project(FaceDetectionServer)

# set(CMAKE_CXX_STANDARD 17)

# # -----------------------------
# # Find dependencies
# # -----------------------------
# find_package(OpenCV REQUIRED)
# find_package(gRPC REQUIRED)
# find_package(Protobuf REQUIRED)

# # -----------------------------
# # Generated protobuf files
# # -----------------------------
# set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/proto")  # <-- point to 'generated/proto'

# # List the .cc files for the executable
# set(PROTO_SRCS
#     ${GENERATED_DIR}/inference.pb.cc
#     ${GENERATED_DIR}/inference.grpc.pb.cc
# )

# # List the headers for include directories
# set(PROTO_HDRS
#     ${GENERATED_DIR}/inference.pb.h
#     ${GENERATED_DIR}/inference.grpc.pb.h
# )

# # -----------------------------
# # Include directories
# # -----------------------------
# include_directories(
#     ${CMAKE_CURRENT_BINARY_DIR}  # for generated files if needed
#     include                       # your local headers
#     ${GENERATED_DIR}              # important: includes generated pb headers
#     ${OpenCV_INCLUDE_DIRS}        # OpenCV headers
# )

# # -----------------------------
# # Add executable
# # -----------------------------
# add_executable(face_detection_server
#     face_detection_server.cpp
#     face_detection.cpp
#     ${PROTO_SRCS}
# )

# # -----------------------------
# # Link libraries
# # -----------------------------
# target_link_libraries(face_detection_server
#     ${OpenCV_LIBS}
#     gRPC::grpc++
#     gRPC::grpc
#     protobuf::libprotobuf
# )

# # -----------------------------
# # Copy Haar cascade to build dir
# # -----------------------------
# configure_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/haarcascade_frontalface_default.xml
#     ${CMAKE_CURRENT_BINARY_DIR}/haarcascade_frontalface_default.xml
#     COPYONLY
# )

# # -----------------------------
# # Optional: message to verify
# # -----------------------------
# message(STATUS "Building FaceDetectionServer with OpenCV: ${OpenCV_VERSION}")
# message(STATUS "Generated protobuf files: ${PROTO_SRCS}")











########################################################
#DECODING IN C++ SERVER

cmake_minimum_required(VERSION 3.10)
project(FaceDetectionServer)

set(CMAKE_CXX_STANDARD 17)

# -----------------------------
# Find dependencies
# -----------------------------
find_package(OpenCV REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

# Find FFmpeg libraries manually
# Using the actual paths from your system
set(FFMPEG_ROOT /opt/homebrew/opt/ffmpeg)
set(FFMPEG_LIB_DIR /opt/homebrew/Cellar/ffmpeg/8.0_1/lib)

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS ${FFMPEG_ROOT}/include
    PATH_SUFFIXES ffmpeg
)

find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_LIB_DIR})
find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_LIB_DIR})
find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_LIB_DIR})
find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_LIB_DIR})

# Check if all FFmpeg libraries were found
if(NOT AVCODEC_LIBRARY OR NOT AVFORMAT_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY)
    message(FATAL_ERROR "Could not find FFmpeg libraries. Please check your FFmpeg installation.")
endif()

# -----------------------------
# Generated protobuf files
# -----------------------------
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/proto")  # <-- point to 'generated/proto'

# List the .cc files for the executable
set(PROTO_SRCS
    ${GENERATED_DIR}/inference.pb.cc
    ${GENERATED_DIR}/inference.grpc.pb.cc
)

# List the headers for include directories
set(PROTO_HDRS
    ${GENERATED_DIR}/inference.pb.h
    ${GENERATED_DIR}/inference.grpc.pb.h
)

# -----------------------------
# Include directories
# -----------------------------
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}  # for generated files if needed
    include                       # your local headers
    ${GENERATED_DIR}              # important: includes generated pb headers
    ${OpenCV_INCLUDE_DIRS}        # OpenCV headers
    ${FFMPEG_INCLUDE_DIR}         # FFmpeg headers
)

# -----------------------------
# Add executable
# -----------------------------
add_executable(face_detection_server
    face_detection_server.cpp
    face_detection.cpp
    ${PROTO_SRCS}
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(face_detection_server
    ${OpenCV_LIBS}
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
)

# -----------------------------
# Copy Haar cascade to build dir
# -----------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/haarcascade_frontalface_default.xml
    ${CMAKE_CURRENT_BINARY_DIR}/haarcascade_frontalface_default.xml
    COPYONLY
)

# -----------------------------
# Optional: message to verify
# -----------------------------
message(STATUS "Building FaceDetectionServer with OpenCV: ${OpenCV_VERSION}")
message(STATUS "Generated protobuf files: ${PROTO_SRCS}")
message(STATUS "FFmpeg include dir: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "FFmpeg libraries: ${AVCODEC_LIBRARY}, ${AVFORMAT_LIBRARY}, ${AVUTIL_LIBRARY}, ${SWSCALE_LIBRARY}")