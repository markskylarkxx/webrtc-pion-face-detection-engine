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

# cmake_minimum_required(VERSION 3.10)
# project(FaceDetectionServer)

# set(CMAKE_CXX_STANDARD 17)

# # -----------------------------
# # Find dependencies
# # -----------------------------
# find_package(OpenCV REQUIRED)
# find_package(gRPC REQUIRED)
# find_package(Protobuf REQUIRED)

# # Find FFmpeg libraries manually
# # Using the actual paths from your system
# set(FFMPEG_ROOT /opt/homebrew/opt/ffmpeg)
# set(FFMPEG_LIB_DIR /opt/homebrew/Cellar/ffmpeg/8.0_1/lib)

# find_path(FFMPEG_INCLUDE_DIR
#     NAMES libavcodec/avcodec.h
#     PATHS ${FFMPEG_ROOT}/include
#     PATH_SUFFIXES ffmpeg
# )

# find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_LIB_DIR})
# find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_LIB_DIR})
# find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_LIB_DIR})
# find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_LIB_DIR})

# # Check if all FFmpeg libraries were found
# if(NOT AVCODEC_LIBRARY OR NOT AVFORMAT_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY)
#     message(FATAL_ERROR "Could not find FFmpeg libraries. Please check your FFmpeg installation.")
# endif()

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
#     ${FFMPEG_INCLUDE_DIR}         # FFmpeg headers
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
#     ${AVCODEC_LIBRARY}
#     ${AVFORMAT_LIBRARY}
#     ${AVUTIL_LIBRARY}
#     ${SWSCALE_LIBRARY}
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
# message(STATUS "FFmpeg include dir: ${FFMPEG_INCLUDE_DIR}")
# message(STATUS "FFmpeg libraries: ${AVCODEC_LIBRARY}, ${AVFORMAT_LIBRARY}, ${AVUTIL_LIBRARY}, ${SWSCALE_LIBRARY}")




















cmake_minimum_required(VERSION 3.10)
project(FaceDetectionServer)

set(CMAKE_CXX_STANDARD 17)

# -----------------------------
# Get parent directory (root where third_party is located)
# -----------------------------
get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
message(STATUS "Parent directory: ${PARENT_DIR}")
message(STATUS "TensorFlow path: ${PARENT_DIR}/third_party/tensorflow")

# -----------------------------
# Find dependencies
# -----------------------------
find_package(OpenCV REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

# -----------------------------
# FFmpeg libraries
# -----------------------------
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

if(NOT AVCODEC_LIBRARY OR NOT AVFORMAT_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY)
    message(FATAL_ERROR "Could not find FFmpeg libraries. Please check your FFmpeg installation.")
endif()

# -----------------------------
# TensorFlow Lite setup
# -----------------------------
# Your tensorflow folder is actually AT third_party/tensorflow (not inside another tensorflow folder)
# So we need to include third_party, not third_party/tensorflow
set(TENSORFLOW_SOURCE ${PARENT_DIR}/third_party/tensorflow)

# Check if you have the flat structure (tensorflow/lite/...) or nested (tensorflow/tensorflow/lite/...)
if(EXISTS ${TENSORFLOW_SOURCE}/lite/interpreter.h)
    message(STATUS "Found TensorFlow with flat structure")
    set(TENSORFLOW_INCLUDE ${PARENT_DIR}/third_party)  # Include third_party so tensorflow/lite/... works
elseif(EXISTS ${TENSORFLOW_SOURCE}/tensorflow/lite/interpreter.h)
    message(STATUS "Found TensorFlow with nested structure")
    set(TENSORFLOW_INCLUDE ${TENSORFLOW_SOURCE})
else()
    # Try Homebrew installation
    message(STATUS "Local TensorFlow not found, checking system installation...")
    find_path(TENSORFLOW_INCLUDE
        NAMES tensorflow/lite/interpreter.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /opt/homebrew/Cellar/tensorflow/*/include
    )
    if(NOT TENSORFLOW_INCLUDE)
        message(FATAL_ERROR "TensorFlow headers not found anywhere!")
    endif()
    message(STATUS "Using system TensorFlow from: ${TENSORFLOW_INCLUDE}")
endif()

# Find TensorFlow Lite library
find_library(TENSORFLOWLITE_LIB
    NAMES tensorflowlite tensorflow-lite libtensorflow_lite libtensorflowlite
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/lib              # Check inference/lib first
        ${TENSORFLOW_SOURCE}/bazel-bin/tensorflow/lite
        ${TENSORFLOW_SOURCE}/lite/build
        /opt/homebrew/lib
        /usr/local/lib
        /opt/homebrew/Cellar/tensorflow/*/lib
    NO_DEFAULT_PATH
)

# If not found with NO_DEFAULT_PATH, try default paths
if(NOT TENSORFLOWLITE_LIB)
    find_library(TENSORFLOWLITE_LIB
        NAMES tensorflowlite tensorflow-lite libtensorflow_lite libtensorflowlite
    )
endif()

if(NOT TENSORFLOWLITE_LIB)
    message(FATAL_ERROR "TensorFlow Lite library not found. Expected at: ${CMAKE_CURRENT_SOURCE_DIR}/lib")
else()
    message(STATUS "TensorFlow Lite library: ${TENSORFLOWLITE_LIB}")
endif()

# -----------------------------
# Neptune library
# -----------------------------
# Neptune source files are in src/ directory
set(NEPTUNE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TfLiteEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Preprocess.cpp
)

message(STATUS "Neptune source files: ${NEPTUNE_SOURCES}")

# -----------------------------
# Generated protobuf files
# -----------------------------
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/proto")

set(PROTO_SRCS
    ${GENERATED_DIR}/inference.pb.cc
    ${GENERATED_DIR}/inference.grpc.pb.cc
)

set(PROTO_HDRS
    ${GENERATED_DIR}/inference.pb.h
    ${GENERATED_DIR}/inference.grpc.pb.h
)

# -----------------------------
# Include directories - FIXED: Only include TensorFlow root
# -----------------------------
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    include
    ${GENERATED_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
    ${TENSORFLOW_INCLUDE}                           # TensorFlow headers
)

# -----------------------------
# Add executable
# -----------------------------
add_executable(face_detection_server
    face_detection_server.cpp
    face_detection.cpp
    ${NEPTUNE_SOURCES}  # Add Neptune sources if found
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
    ${TENSORFLOWLITE_LIB}
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
# Optional messages
# -----------------------------
message(STATUS "Building FaceDetectionServer with OpenCV: ${OpenCV_VERSION}")
message(STATUS "Generated protobuf files: ${PROTO_SRCS}")
message(STATUS "FFmpeg include dir: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "Parent directory: ${PARENT_DIR}")
message(STATUS "TensorFlow include: ${TENSORFLOW_INCLUDE}")
message(STATUS "TensorFlow Lite library: ${TENSORFLOWLITE_LIB}")