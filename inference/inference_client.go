package inference

import (
    "context"
    "time"
    pb "webrtc-pion-face-engine/proto"

    "google.golang.org/grpc"
)

type FaceClient struct {
    client pb.FaceDetectionClient
}

func NewFaceClient(address string) (*FaceClient, error) {
    conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock(), grpc.WithTimeout(3*time.Second))
    if err != nil {
        return nil, err
    }
    client := pb.NewFaceDetectionClient(conn)
    return &FaceClient{client: client}, nil
}

func (f *FaceClient) Detect(frame []byte, width, height int) (*pb.DetectionResponse, error) {
    ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
    defer cancel()

    req := &pb.FrameRequest{
        FrameData: frame,
        Width:     int32(width),
        Height:    int32(height),
    }

    return f.client.DetectFaces(ctx, req)
}
