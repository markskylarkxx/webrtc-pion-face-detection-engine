# #!/bin/bash

# set -e

# echo "Building WebRTC Face Detection System..."

# # Build signaling server
# echo "Building signaling server..."
# cd signaling
# go mod tidy
# go build -o ../bin/signaling-server .
# cd ..

# # Build ingest worker
# echo "Building ingest worker..."
# cd ingest
# go mod tidy
# go build -o ../bin/ingest-worker .
# cd ..

# # Build inference engine
# echo "Building inference engine..."
# cd inference
# mkdir -p build
# cd build
# cmake ..
# make
# cp libface_detection.so ../../bin/
# cd ../..

# echo "Build complete! Output in bin/ directory"


##############################################################



#!/bin/bash

set -e

echo "Building all components..."

# Download Haar cascade
./scripts/download_haarcascade.sh

# Build C++ inference server
echo "Building C++ inference server..."
cd inference
mkdir -p build
cd build
cmake ..
make -j4
cd ../..

# Generate Go protobuf files
echo "Generating Go protobuf files..."
protoc --go_out=. --go-grpc_out=. proto/inference.proto

# Build Go components
echo "Building Go components..."
cd ingest
go build -o ../bin/ingest-worker
cd ..

cd signaling
go build -o ../bin/signaling-server
cd ..

echo "Build completed successfully!"