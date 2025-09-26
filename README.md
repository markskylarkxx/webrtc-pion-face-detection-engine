# WebRTC Face Detection Engine

Real-time face detection system using WebRTC for video streaming and C++ for inference.

## Architecture
📱 Web Client (WebRTC) → 📡 Signaling Server → 🔄 Ingest Worker → 🖥️ C++ Inference → 📊 Results (DataChannel)


## Quick Start

### Prerequisites
- Go 1.21+
- C++17 compiler
- CMake 3.16+
- Modern web browser with WebRTC support

### Build
```bash
chmod +x scripts/*.sh
./scripts/build_all.sh


### Run
./scripts/run_dev.sh


Then open client/web/index.html in your browser.

#Components
Signaling Server
WebSocket-based signaling
Client/worker coordination
SDP/ICE candidate exchange


#Ingest Worker
WebRTC peer connection management
Video frame reception and routing
Inference result delivery via DataChannel

#Inference Engine
C++ face detection (stub implementation)
Easy TensorFlow integration point
FFI-ready C interface



#Web Client
Camera access and video streaming
Real-time result display
Bounding box visualization

#Development
Adding Real Face Detection
Integrate TensorFlow in inference/face_detection.cpp
Load pre-trained face detection model
Implement proper frame processing in processFrame()



#Scaling
Add multiple ingest workers
Implement load balancing in signaling server
Add Redis for session management



## Running the System

1. **Build everything:**
```bash
chmod +x scripts/*.sh
./scripts/build_all.sh



#Start the services:
./scripts/run_dev.sh


#Open the client:
Open client/web/index.html in a web browser
Click "Start Camera" to enable your webcam
Click "Connect to Server" to start face detection

This implementation provides a complete foundation that you can extend with actual TensorFlow models and production-ready features. The stub face detection will show placeholder results that you can replace with real inference logic.




#Step 1: Start the signaling server (Terminal 1)
 ./bin/signaling-server

 You should see:
Signaling server starting on :8080


#Step 2: Start the ingest worker (Terminal 2)
export SIGNALING_URL="ws://localhost:8080"
export WORKER_ID="ingest_worker_1"
./bin/ingest-worker

You should see:
Starting ingest worker: ingest_worker_1
Connected to signaling server as ingest_worker_1

#Step 3: Start the web server (Terminal 3)
python3 -m http.server 8000


#Step 4: Open the browser
Go to: http://localhost:8000/client/web/index.html

#Step 5: Test the application
Click "Start Camera" - allow camera access
Click "Connect to Server" - initiate WebRTC connection
Check the terminals for connection message


















Changes / Additions:

inference/face_detection_server.cpp → gRPC server for C++ detection.

inference/inference_client.go → Go client to call gRPC server.

ingest/main.go → now calls inference_client.go to send frames.

proto/inference.proto → unchanged, defines RPC service.

proto/inference.pb.go → generated with protoc --go_out=. --go-grpc_out=. proto/inference.proto.

CMakeLists.txt → updated to build face_detection_server with gRPC + OpenCV.




#RELINK THE LATEST PROTOBUF:
brew unlink protobuf
brew link --overwrite --force protobuf
This ensures /opt/homebrew/opt/protobuf points to version 32.1.


# Reinstall gRPC (so the plugin links against the correct protobuf)
brew reinstall grpc
This will rebuild grpc_cpp_plugin against your current protobuf (32.1).

#Verify the plugin
otool -L $(which grpc_cpp_plugin)
You should see it now points to /opt/homebrew/Cellar/protobuf/32.1/lib/libprotoc.dylib (no 29.3 reference).



Generate your protobuf & gRPC C++ files
Make sure you already generated them. From your project root:

# Generate protobuf C++ files
protoc -I=proto --cpp_out=inference proto/inference.proto

# Generate gRPC C++ files
protoc -I=proto --grpc_out=inference --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` proto/inference.proto

After this, you should see these files in your inference/ folder:
inference.pb.h
inference.pb.cc
inference.grpc.pb.h
inference.grpc.pb.cc




I want my Go ingest worker to:

Receive VP8 video streams from WebRTC clients

Extract video frames from the RTP packets

Send these frames to a separate C++ service for face detection

Receive bounding box results from the C++ service

Send these real bounding boxes back to the client via DataChannel (JSON)

Without using cgo - using gRPC instead for clean separation.