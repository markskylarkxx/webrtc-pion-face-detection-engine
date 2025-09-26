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

type InferenceClient struct {
	conn   *grpc.ClientConn
	client pb.FaceDetectionClient
}

func NewInferenceClient(addr string) (*InferenceClient, error) {
	conn, err := grpc.Dial(addr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
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

	req := &pb.FrameRequest{
		FrameData: frameData,
		Width:     int32(width),
		Height:    int32(height),
		Channels:  int32(channels),
		Timestamp: time.Now().UnixMilli(),
		FrameId:   fmt.Sprintf("frame-%d", time.Now().UnixNano()),
	}

	resp, err := c.client.DetectFaces(ctx, req)
	if err != nil {
		return nil, 0, fmt.Errorf("gRPC detection failed: %v", err)
	}

	log.Printf("C++ detection: %d faces, %d ms", len(resp.Faces), resp.ProcessingTimeMs)
	return resp.Faces, int64(resp.ProcessingTimeMs), nil  // Fixed: cast int32 to int64
}

func (c *InferenceClient) Close() {
	if c.conn != nil {
		c.conn.Close()
	}
}













