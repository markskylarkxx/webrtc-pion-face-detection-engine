
# #brew install ffmpeg


# cmake_minimum_required(VERSION 3.10)
# project(FaceDetectionServer)

# set(CMAKE_CXX_STANDARD 14)

# find_package(OpenCV REQUIRED)
# find_package(gRPC REQUIRED)
# find_package(Protobuf REQUIRED)

# # Generated gRPC/Protobuf files
# set(PROTO_SRCS
#     generated/inference.pb.cc
#     generated/inference.grpc.pb.cc
# )

# set(PROTO_HDS
#     generated/inference.pb.h
#     generated/inference.grpc.pb.h
# )

# include_directories(
#     ${CMAKE_CURRENT_BINARY_DIR}
#     include
#     ${OpenCV_INCLUDE_DIRS}
# )

# add_executable(face_detection_server
#     face_detection_server.cpp
#     ${PROTO_SRCS}
# )

# target_link_libraries(face_detection_server
#     ${OpenCV_LIBS}
#     gRPC::grpc++
#     gRPC::grpc
#     protobuf::libprotobuf
# )

# # Copy the Haar cascade file to build directory
# configure_file(haarcascade_frontalface_default.xml 
#                ${CMAKE_CURRENT_BINARY_DIR}/haarcascade_frontalface_default.xml COPYONLY)







cmake_minimum_required(VERSION 3.10)
project(FaceDetectionServer)

set(CMAKE_CXX_STANDARD 17)

# -----------------------------
# Find dependencies
# -----------------------------
find_package(OpenCV REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

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
