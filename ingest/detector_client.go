// package main

// import (
// 	"context"
// 	"fmt"
// 	"log"
// 	"time"

// 	"google.golang.org/grpc"
// 	"google.golang.org/grpc/credentials/insecure"
//      pb "webrtc-pion-face-engine/proto"
// )

// // Default timeout for gRPC connection attempt
// const dialTimeout = 5 * time.Second

// type InferenceClient struct {
// 	conn   *grpc.ClientConn
// 	client pb.FaceDetectionClient
// }

// func NewInferenceClient(addr string) (*InferenceClient, error) {
//     // Create a context with a timeout for the Dial operation
// 	ctx, cancel := context.WithTimeout(context.Background(), dialTimeout)
// 	defer cancel()
    
// 	conn, err := grpc.DialContext(ctx, addr,
// 		grpc.WithTransportCredentials(insecure.NewCredentials()),
//         // grpc.WithBlock() is used to wait until the connection is established
// 		grpc.WithBlock(),
// 	)
// 	if err != nil {
// 		return nil, fmt.Errorf("failed to connect to inference service: %v", err)
// 	}

// 	return &InferenceClient{
// 		conn:   conn,
// 		client: pb.NewFaceDetectionClient(conn),
// 	}, nil
// }

// // DetectFaces sends a frame to gRPC service and returns pb.BoundingBox slice
// func (c *InferenceClient) DetectFaces(frameData []byte, width, height, channels int) ([]*pb.BoundingBox, int64, error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
// 	defer cancel()

//     // --- FIX APPLIED HERE ---
//     // 1. Corrected 'FrameData' to 'EncodedFrame' to match the Protobuf definition (field 7).
//     // 2. Restored the 'Codec' field (field 6) to adhere to the proto schema.
// 	req := &pb.FrameRequest{
// 		EncodedFrame: frameData,  // ✅ Correct (PascalCase)
// 		Width:     int32(width),  // ✅ Correct (PascalCase)
// 		Height:    int32(height), // ✅ Correct (PascalCase)
// 		Channels:  int32(channels), // ✅ Correct (PascalCase)
// 		Timestamp: time.Now().UnixMilli(), // ✅ Correct (PascalCase)
// 		FrameId:   fmt.Sprintf("frame-%d", time.Now().UnixNano()), // ✅ Correct (PascalCase)
// 		// Codec:     "VP8_Y_PLANE", // ✅ Correct (PascalCase)
// 		Codec:         "VP8", // ✅ Correct (PascalCase)

// 	}

// 	// Debug log to verify the request
// 	log.Printf("🔍 Sending gRPC request - Dim: %dx%d, Channels: %d, Data size: %d bytes",
// 		width, height, channels, len(frameData))

// 	resp, err := c.client.DetectFaces(ctx, req)
// 	if err != nil {
// 		return nil, 0, fmt.Errorf("gRPC detection failed: %v", err)
// 	}

// 	// This log line may need to be handled by the caller, but leaving for reference
// 	// log.Printf("C++ detection: %d faces, %d ms", len(resp.Faces), resp.ProcessingTimeMs)
// 	return resp.Faces, int64(resp.ProcessingTimeMs), nil
// }

// func (c *InferenceClient) Close() {
// 	if c.conn != nil {
// 		c.conn.Close()
// 	}
// }



















//DECODING IN C++

package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	pb "webrtc-pion-face-engine/proto"
)

// FIXED: Increased timeouts for better performance
const (
	dialTimeout    = 10 * time.Second
	requestTimeout = 15 * time.Second // Increased from 3s to 15s
)

type InferenceClient struct {
	conn   *grpc.ClientConn
	client pb.FaceDetectionClient
}

func NewInferenceClient(addr string) (*InferenceClient, error) {
	ctx, cancel := context.WithTimeout(context.Background(), dialTimeout)
	defer cancel()
	
	conn, err := grpc.DialContext(ctx, addr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithBlock(),
		// FIXED: Add connection pooling and keepalive
		grpc.WithDefaultCallOptions(
			grpc.MaxCallRecvMsgSize(50*1024*1024), // 50MB max receive
			grpc.MaxCallSendMsgSize(50*1024*1024), // 50MB max send
		),
	)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to inference service: %v", err)
	}

	return &InferenceClient{
		conn:   conn,
		client: pb.NewFaceDetectionClient(conn),
	}, nil
}

// FIXED: DetectFacesFromEncodedFrame with proper timeout and error handling
func (c *InferenceClient) DetectFacesFromEncodedFrame(frameData []byte, width, height, channels int) ([]*pb.BoundingBox, int64, error) {
	// FIXED: Increased timeout to 15 seconds to prevent deadline exceeded
	ctx, cancel := context.WithTimeout(context.Background(), requestTimeout)
	defer cancel()

	// Validate input data
	if len(frameData) == 0 {
		return nil, 0, fmt.Errorf("empty frame data")
	}

	// Create the request with encoded VP8 frame data
	req := &pb.FrameRequest{
		EncodedFrame: frameData,
		Width:        int32(width),
		Height:       int32(height),
		Channels:     int32(channels),
		Timestamp:    time.Now().UnixMilli(),
		FrameId:      fmt.Sprintf("frame-%d", time.Now().UnixNano()),
		Codec:        "VP8",
	}

	log.Printf("🔍 Sending gRPC request - Expected Dim: %dx%d, Channels: %d, Encoded data size: %d bytes",
		width, height, channels, len(frameData))

	resp, err := c.client.DetectFaces(ctx, req)
	if err != nil {
		// FIXED: Better error logging
		log.Printf("❌ gRPC detection failed for frame %s: %v", req.FrameId, err)
		return nil, 0, fmt.Errorf("gRPC detection failed: %v", err)
	}

	// FIXED: Log successful processing time
	if resp.ProcessingTimeMs > 1000 {
		log.Printf("⚠️ Slow processing: %dms for frame %s", resp.ProcessingTimeMs, req.FrameId)
	}

	return resp.Faces, int64(resp.ProcessingTimeMs), nil
}

// DetectFacesFromRawFrame for raw pixel data (alternative method)
func (c *InferenceClient) DetectFacesFromRawFrame(frameData []byte, width, height, channels int) ([]*pb.BoundingBox, int64, error) {
	ctx, cancel := context.WithTimeout(context.Background(), requestTimeout)
	defer cancel()

	req := &pb.FrameRequest{
		EncodedFrame: frameData,
		Width:        int32(width),
		Height:       int32(height),
		Channels:     int32(channels),
		Timestamp:    time.Now().UnixMilli(),
		FrameId:      fmt.Sprintf("frame-%d", time.Now().UnixNano()),
		Codec:        "RAW",
	}

	log.Printf("🔍 Sending gRPC request - Raw frame: %dx%d, Channels: %d, Data size: %d bytes",
		width, height, channels, len(frameData))

	resp, err := c.client.DetectFaces(ctx, req)
	if err != nil {
		return nil, 0, fmt.Errorf("gRPC detection failed: %v", err)
	}

	return resp.Faces, int64(resp.ProcessingTimeMs), nil
}

// Ping checks server connectivity
func (c *InferenceClient) Ping() error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	req := &pb.FrameRequest{
		FrameId:      "ping-test",
		Timestamp:    time.Now().UnixMilli(),
		Width:        1,
		Height:       1,
		Channels:     1,
		Codec:        "RAW",
		EncodedFrame: []byte{0},
	}

	_, err := c.client.DetectFaces(ctx, req)
	return err
}

func (c *InferenceClient) Close() {
	if c.conn != nil {
		c.conn.Close()
	}
}