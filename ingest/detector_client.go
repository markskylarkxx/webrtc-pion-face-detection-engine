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

// Default timeout for gRPC connection attempt
const dialTimeout = 5 * time.Second

type InferenceClient struct {
	conn   *grpc.ClientConn
	client pb.FaceDetectionClient
}

func NewInferenceClient(addr string) (*InferenceClient, error) {
    // Create a context with a timeout for the Dial operation
	ctx, cancel := context.WithTimeout(context.Background(), dialTimeout)
	defer cancel()
    
	conn, err := grpc.DialContext(ctx, addr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
        // grpc.WithBlock() is used to wait until the connection is established
		grpc.WithBlock(),
	)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to inference service: %v", err)
	}

	return &InferenceClient{
		conn:   conn,
		client: pb.NewFaceDetectionClient(conn),
	}, nil
}

// DetectFaces sends a frame to gRPC service and returns pb.BoundingBox slice
func (c *InferenceClient) DetectFaces(frameData []byte, width, height, channels int) ([]*pb.BoundingBox, int64, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()

    // --- FIX APPLIED HERE ---
    // 1. Corrected 'FrameData' to 'EncodedFrame' to match the Protobuf definition (field 7).
    // 2. Restored the 'Codec' field (field 6) to adhere to the proto schema.
	req := &pb.FrameRequest{
		EncodedFrame: frameData,  // ✅ Correct (PascalCase)
		Width:     int32(width),  // ✅ Correct (PascalCase)
		Height:    int32(height), // ✅ Correct (PascalCase)
		Channels:  int32(channels), // ✅ Correct (PascalCase)
		Timestamp: time.Now().UnixMilli(), // ✅ Correct (PascalCase)
		FrameId:   fmt.Sprintf("frame-%d", time.Now().UnixNano()), // ✅ Correct (PascalCase)
		Codec:     "VP8_Y_PLANE", // ✅ Correct (PascalCase)
	}

	// Debug log to verify the request
	log.Printf("🔍 Sending gRPC request - Dim: %dx%d, Channels: %d, Data size: %d bytes",
		width, height, channels, len(frameData))

	resp, err := c.client.DetectFaces(ctx, req)
	if err != nil {
		return nil, 0, fmt.Errorf("gRPC detection failed: %v", err)
	}

	// This log line may need to be handled by the caller, but leaving for reference
	// log.Printf("C++ detection: %d faces, %d ms", len(resp.Faces), resp.ProcessingTimeMs)
	return resp.Faces, int64(resp.ProcessingTimeMs), nil
}

func (c *InferenceClient) Close() {
	if c.conn != nil {
		c.conn.Close()
	}
}
