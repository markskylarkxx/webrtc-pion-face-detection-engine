
//Rebuild and test

// cd ingest
// go build -o ../bin/ingest-worker .
// cd ..

// # Restart the ingest worker
// export SIGNALING_URL="ws://localhost:8080"
// export WORKER_ID="ingest_worker_1"
// ./bin/ingest-worker


// export SIGNALING_URL="ws://localhost:8080"
//  export WORKER_ID="ingest_worker_1"
// ./bin/ingest-worker



// package main

// import (
// 	"encoding/json"
// 	"log"
// 	"os"
// 	"os/signal"
// 	"sync"
// 	"syscall"
// 	"time"

// 	"github.com/gorilla/websocket"
// 	"github.com/pion/webrtc/v3"
// )

// // --- Types for signaling ---
// type DetectionBoundingBox struct {
// 	X          int     `json:"x"`
// 	Y          int     `json:"y"`
// 	Width      int     `json:"width"`
// 	Height     int     `json:"height"`
// 	Confidence float32 `json:"confidence"`
// }

// type DetectionResult struct {
// 	FacesDetected int                   `json:"faces_detected"`
// 	BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
// 	Timestamp     int64                 `json:"timestamp"`
// }

// // SDP and ICE candidates
// type SignalMessage struct {
// 	Type      string                  `json:"type"`
// 	To        string                  `json:"to,omitempty"`
// 	From      string                  `json:"from,omitempty"`
// 	SDP       string                  `json:"sdp,omitempty"`
// 	Candidate *webrtc.ICECandidateInit `json:"candidate,omitempty"`
// }

// // --- Ingest Worker ---
// type IngestWorker struct {
// 	signalingURL     string
// 	wsConn           *websocket.Conn
// 	wsMutex          sync.Mutex
// 	peerConnection   *webrtc.PeerConnection
// 	dataChannel      *webrtc.DataChannel
// 	currentClient    string
// 	workerID         string
// 	grpcClient       *InferenceClient
// 	queuedCandidates []SignalMessage
// }

// func (w *IngestWorker) sendWebSocketMessage(msg interface{}) error {
// 	w.wsMutex.Lock()
// 	defer w.wsMutex.Unlock()
// 	return w.wsConn.WriteJSON(msg)
// }

// // --- Initialize Peer Connection ---
// func (w *IngestWorker) initializeWebRTC() error {
// 	config := webrtc.Configuration{
// 		ICEServers: []webrtc.ICEServer{
// 			{URLs: []string{"stun:stun.l.google.com:19302"}},
// 			{URLs: []string{"turn:turn.example.com:3478"}, Username: "user", Credential: "pass"},
// 		},
// 	}

// 	pc, err := webrtc.NewPeerConnection(config)
// 	if err != nil {
// 		return err
// 	}
// 	w.peerConnection = pc

// 	// Handle incoming tracks
// 	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
// 		log.Printf("Video track received: %s", track.Codec().MimeType)
// 		go w.processVP8Track(track)
// 	})

// 	// ICE candidates
// 	pc.OnICECandidate(func(c *webrtc.ICECandidate) {
// 		if c == nil || w.currentClient == "" {
// 			return
// 		}
// 		jsonCandidate := c.ToJSON()
// 		msg := SignalMessage{
// 			Type:      "candidate",
// 			To:        w.currentClient,
// 			From:      w.workerID,
// 			Candidate: &jsonCandidate,
// 		}
// 		if err := w.sendWebSocketMessage(msg); err != nil {
// 			log.Printf("Error sending ICE candidate: %v", err)
// 		}
// 	})

// 	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
// 		log.Printf("PeerConnection state changed: %s", state.String())
// 	})

// 	// Data channel
// 	pc.OnDataChannel(func(dc *webrtc.DataChannel) {
// 		log.Printf("Data channel received: %s", dc.Label())
// 		w.dataChannel = dc

// 		dc.OnOpen(func() {
// 			log.Printf("Data channel opened with client %s", w.currentClient)
// 		})
// 		dc.OnClose(func() { log.Printf("Data channel closed") })
// 		dc.OnError(func(err error) { log.Printf("Data channel error: %v", err) })
// 	})

// 	// Flush queued ICE candidates
// 	for _, msg := range w.queuedCandidates {
// 		w.handleCandidate(msg)
// 	}
// 	w.queuedCandidates = nil

// 	return nil
// }

// // --- Handle incoming offer ---
// func (w *IngestWorker) handleOffer(msg SignalMessage) {
// 	log.Printf("Received offer from %s", msg.From)
// 	w.currentClient = msg.From

// 	if w.peerConnection != nil {
// 		w.peerConnection.Close()
// 		w.peerConnection = nil
// 		w.dataChannel = nil
// 	}

// 	if err := w.initializeWebRTC(); err != nil {
// 		log.Printf("Failed to init WebRTC: %v", err)
// 		return
// 	}

// 	offer := webrtc.SessionDescription{
// 		Type: webrtc.SDPTypeOffer,
// 		SDP:  msg.SDP,
// 	}

// 	if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
// 		log.Printf("Error setting remote description: %v", err)
// 		return
// 	}

// 	answer, err := w.peerConnection.CreateAnswer(nil)
// 	if err != nil {
// 		log.Printf("Error creating answer: %v", err)
// 		return
// 	}

// 	if err := w.peerConnection.SetLocalDescription(answer); err != nil {
// 		log.Printf("Error setting local description: %v", err)
// 		return
// 	}

// 	resp := SignalMessage{
// 		Type: "answer",
// 		To:   msg.From,
// 		From: w.workerID,
// 		SDP:  answer.SDP,
// 	}
// 	w.sendWebSocketMessage(resp)
// }

// // --- Handle ICE candidate ---
// func (w *IngestWorker) handleCandidate(msg SignalMessage) {
// 	if w.peerConnection == nil {
// 		w.queuedCandidates = append(w.queuedCandidates, msg)
// 		return
// 	}

// 	if msg.Candidate == nil {
// 		return
// 	}

// 	if err := w.peerConnection.AddICECandidate(*msg.Candidate); err != nil {
// 		log.Printf("Error adding ICE candidate: %v", err)
// 	}
// }

// // --- Handle signaling messages ---
// func (w *IngestWorker) handleSignalingMessages() {
// 	for {
// 		var msg SignalMessage
// 		if err := w.wsConn.ReadJSON(&msg); err != nil {
// 			log.Printf("WebSocket read error: %v", err)
// 			return
// 		}

// 		if msg.To != "" && msg.To != w.workerID {
// 			continue
// 		}

// 		switch msg.Type {
// 		case "offer":
// 			w.handleOffer(msg)
// 		case "candidate":
// 			w.handleCandidate(msg)
// 		}
// 	}
// }

// // --- VP8 video track processing ---
// func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
// 	var frameBuffer []byte
// 	var lastTimestamp uint32
// 	frameCount := 0

// 	for {
// 		rtpPacket, _, err := track.ReadRTP()
// 		if err != nil {
// 			log.Printf("Error reading RTP packet: %v", err)
// 			return
// 		}

// 		if rtpPacket.Marker || (lastTimestamp != 0 && rtpPacket.Timestamp != lastTimestamp) {
// 			if len(frameBuffer) > 0 {
// 				frameCount++
// 				if frameCount%10 == 0 {
// 					go w.processVideoFrame(frameBuffer, 640, 480, 3)
// 				}
// 				frameBuffer = nil
// 			}
// 		}
// 		frameBuffer = append(frameBuffer, rtpPacket.Payload...)
// 		lastTimestamp = rtpPacket.Timestamp
// 	}
// }

// // --- Process a single video frame ---
// func (w *IngestWorker) processVideoFrame(frame []byte, width, height, frameCount int) {
// 	if w.grpcClient == nil {
// 		log.Printf("gRPC client not initialized")
// 		return
// 	}

// 	faces, _, err := w.grpcClient.DetectFaces(frame, width, height, 3)
// 	if err != nil {
// 		log.Printf("Face detection error: %v", err)
// 		return
// 	}

// 	var boxes []DetectionBoundingBox
// 	for _, f := range faces {
// 		boxes = append(boxes, DetectionBoundingBox{
// 			X:          int(f.X),
// 			Y:          int(f.Y),
// 			Width:      int(f.Width),
// 			Height:     int(f.Height),
// 			Confidence: f.Confidence,
// 		})
// 	}

// 	result := DetectionResult{
// 		FacesDetected: len(boxes),
// 		BoundingBoxes: boxes,
// 		Timestamp:     time.Now().UnixMilli(),
// 	}

// 	if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
// 		data, _ := json.Marshal(result)
// 		if err := w.dataChannel.Send(data); err != nil {
// 			log.Printf("Error sending detection results: %v", err)
// 		} else {
// 			log.Printf("Sent detection results: %d faces", result.FacesDetected)
// 		}
// 	}
// }

// // --- Main ---
// func main() {
// 	signalingURL := os.Getenv("SIGNALING_URL")
// 	if signalingURL == "" {
// 		signalingURL = "ws://localhost:8080"
// 	}

// 	workerID := os.Getenv("WORKER_ID")
// 	if workerID == "" {
// 		workerID = "ingest_worker_1"
// 	}

// 	log.Printf("Starting ingest worker: %s", workerID)

// 	conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
// 	if err != nil {
// 		log.Fatal("Failed to connect to signaling server:", err)
// 	}
// 	defer conn.Close()

// 	worker := &IngestWorker{
// 		signalingURL: signalingURL,
// 		workerID:     workerID,
// 		wsConn:       conn,
// 	}

// 	// Initialize gRPC client
// 	grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
// 	if grpcEndpoint == "" {
// 		grpcEndpoint = "localhost:50051"
// 	}
// 	client, err := NewInferenceClient(grpcEndpoint)
// 	if err != nil {
// 		log.Fatal("Failed to initialize gRPC client:", err)
// 	}
// 	defer client.Close()
// 	worker.grpcClient = client

// 	go worker.handleSignalingMessages()
// 	log.Printf("Ingest worker ready: %s", worker.workerID)

// 	sigChan := make(chan os.Signal, 1)
// 	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
// 	<-sigChan

// 	log.Println("Shutting down ingest worker...")
// 	if worker.peerConnection != nil {
// 		worker.peerConnection.Close()
// 	}
// }











///////////////////////////////////////////////////////////////////////////





// package main

// import (
// 	"encoding/json"
// 	"log"
// 	"os"
// 	"os/signal"
// 	"sync"
// 	"syscall"
// 	"time"

// 	"github.com/gorilla/websocket"
// 	"github.com/pion/webrtc/v3"
// )

// // --- Types for signaling ---
// type DetectionBoundingBox struct {
// 	X          int     `json:"x"`
// 	Y          int     `json:"y"`
// 	Width      int     `json:"width"`
// 	Height     int     `json:"height"`
// 	Confidence float32 `json:"confidence"`
// }

// type DetectionResult struct {
// 	FacesDetected int                   `json:"faces_detected"`
// 	BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
// 	Timestamp     int64                 `json:"timestamp"`
// }

// // SDP and ICE candidates
// type SignalMessage struct {
// 	Type      string                  `json:"type"`
// 	To        string                  `json:"to,omitempty"`
// 	From      string                  `json:"from,omitempty"`
// 	SDP       string                  `json:"sdp,omitempty"`
// 	Candidate *webrtc.ICECandidateInit `json:"candidate,omitempty"`
// }

// // --- Ingest Worker ---
// type IngestWorker struct {
// 	signalingURL     string
// 	wsConn           *websocket.Conn
// 	wsMutex          sync.Mutex
// 	peerConnection   *webrtc.PeerConnection
// 	dataChannel      *webrtc.DataChannel
// 	currentClient    string
// 	workerID         string
// 	grpcClient       *InferenceClient
// 	queuedCandidates []SignalMessage
// }

// func (w *IngestWorker) sendWebSocketMessage(msg interface{}) error {
// 	w.wsMutex.Lock()
// 	defer w.wsMutex.Unlock()
// 	return w.wsConn.WriteJSON(msg)
// }

// // --- Initialize Peer Connection ---
// func (w *IngestWorker) initializeWebRTC() error {
// 	config := webrtc.Configuration{
// 		ICEServers: []webrtc.ICEServer{
// 			{URLs: []string{"stun:stun.l.google.com:19302"}},
// 			{URLs: []string{"turn:turn.example.com:3478"}, Username: "user", Credential: "pass"},
// 		},
// 	}

// 	pc, err := webrtc.NewPeerConnection(config)
// 	if err != nil {
// 		return err
// 	}
// 	w.peerConnection = pc

// 	// Handle incoming tracks
// 	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
// 		log.Printf("Video track received: %s", track.Codec().MimeType)
// 		go w.processVP8Track(track)
// 	})

// 	// ICE candidates
// 	pc.OnICECandidate(func(c *webrtc.ICECandidate) {
// 		if c == nil || w.currentClient == "" {
// 			return
// 		}
// 		jsonCandidate := c.ToJSON()
// 		msg := SignalMessage{
// 			Type:      "candidate",
// 			To:        w.currentClient,
// 			From:      w.workerID,
// 			Candidate: &jsonCandidate,
// 		}
// 		if err := w.sendWebSocketMessage(msg); err != nil {
// 			log.Printf("Error sending ICE candidate: %v", err)
// 		}
// 	})

// 	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
// 		log.Printf("PeerConnection state changed: %s", state.String())
// 	})

// 	// Data channel
// 	pc.OnDataChannel(func(dc *webrtc.DataChannel) {
// 		log.Printf("Data channel received: %s", dc.Label())
// 		w.dataChannel = dc

// 		dc.OnOpen(func() {
// 			log.Printf("Data channel opened with client %s", w.currentClient)
// 		})
// 		dc.OnClose(func() { log.Printf("Data channel closed") })
// 		dc.OnError(func(err error) { log.Printf("Data channel error: %v", err) })
// 	})

// 	// Flush queued ICE candidates
// 	for _, msg := range w.queuedCandidates {
// 		w.handleCandidate(msg)
// 	}
// 	w.queuedCandidates = nil

// 	return nil
// }

// // --- Handle incoming offer ---
// func (w *IngestWorker) handleOffer(msg SignalMessage) {
// 	log.Printf("Received offer from %s", msg.From)
// 	w.currentClient = msg.From

// 	if w.peerConnection != nil {
// 		w.peerConnection.Close()
// 		w.peerConnection = nil
// 		w.dataChannel = nil
// 	}

// 	if err := w.initializeWebRTC(); err != nil {
// 		log.Printf("Failed to init WebRTC: %v", err)
// 		return
// 	}

// 	offer := webrtc.SessionDescription{
// 		Type: webrtc.SDPTypeOffer,
// 		SDP:  msg.SDP,
// 	}

// 	if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
// 		log.Printf("Error setting remote description: %v", err)
// 		return
// 	}

// 	answer, err := w.peerConnection.CreateAnswer(nil)
// 	if err != nil {
// 		log.Printf("Error creating answer: %v", err)
// 		return
// 	}

// 	if err := w.peerConnection.SetLocalDescription(answer); err != nil {
// 		log.Printf("Error setting local description: %v", err)
// 		return
// 	}

// 	resp := SignalMessage{
// 		Type: "answer",
// 		To:   msg.From,
// 		From: w.workerID,
// 		SDP:  answer.SDP,
// 	}
// 	w.sendWebSocketMessage(resp)
// }

// // --- Handle ICE candidate ---
// func (w *IngestWorker) handleCandidate(msg SignalMessage) {
// 	if w.peerConnection == nil {
// 		w.queuedCandidates = append(w.queuedCandidates, msg)
// 		return
// 	}

// 	if msg.Candidate == nil {
// 		return
// 	}

// 	if err := w.peerConnection.AddICECandidate(*msg.Candidate); err != nil {
// 		log.Printf("Error adding ICE candidate: %v", err)
// 	}
// }

// // --- Handle signaling messages ---
// func (w *IngestWorker) handleSignalingMessages() {
// 	for {
// 		var msg SignalMessage
// 		if err := w.wsConn.ReadJSON(&msg); err != nil {
// 			log.Printf("WebSocket read error: %v", err)
// 			return
// 		}

// 		if msg.To != "" && msg.To != w.workerID {
// 			continue
// 		}

// 		switch msg.Type {
// 		case "offer":
// 			w.handleOffer(msg)
// 		case "candidate":
// 			w.handleCandidate(msg)
// 		}
// 	}
// }

// // --- VP8 video track processing ---
// func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
// 	var frameBuffer []byte
// 	var lastTimestamp uint32
// 	frameCount := 0

// 	for {
// 		rtpPacket, _, err := track.ReadRTP()
// 		if err != nil {
// 			log.Printf("Error reading RTP packet: %v", err)
// 			return
// 		}

// 		if rtpPacket.Marker || (lastTimestamp != 0 && rtpPacket.Timestamp != lastTimestamp) {
// 			if len(frameBuffer) > 0 {
// 				frameCount++

// 				// Extract width/height from VP8 frame header
// 				width, height := extractVP8Dimensions(frameBuffer)
// 				if width > 0 && height > 0 {
// 					go w.processVideoFrame(frameBuffer, width, height, 3)
// 				} else {
// 					log.Printf("Invalid VP8 frame dimensions, skipping frame")
// 				}

// 				frameBuffer = nil
// 			}
// 		}

// 		frameBuffer = append(frameBuffer, rtpPacket.Payload...)
// 		lastTimestamp = rtpPacket.Timestamp
// 	}
// }

// // --- Extract width/height from VP8 frame header ---
// func extractVP8Dimensions(payload []byte) (int, int) {
// 	if len(payload) < 10 {
// 		return 0, 0
// 	}

// 	tag := payload[0]

// 	// Only key frames contain width/height info
// 	if tag&0x01 != 0 {
// 		return 0, 0
// 	}

// 	if len(payload) < 10 {
// 		return 0, 0
// 	}

// 	width := int(payload[6]) | int(payload[7])<<8
// 	height := int(payload[8]) | int(payload[9])<<8

// 	width &= 0x3FFF
// 	height &= 0x3FFF

// 	return width, height
// }

// // --- Process a single video frame ---
// func (w *IngestWorker) processVideoFrame(frame []byte, width, height, frameCount int) {
// 	if w.grpcClient == nil {
// 		log.Printf("gRPC client not initialized")
// 		return
// 	}

// 	faces, _, err := w.grpcClient.DetectFaces(frame, width, height, 3)
// 	if err != nil {
// 		log.Printf("Face detection error: %v", err)
// 		return
// 	}

// 	var boxes []DetectionBoundingBox
// 	for _, f := range faces {
// 		boxes = append(boxes, DetectionBoundingBox{
// 			X:          int(f.X),
// 			Y:          int(f.Y),
// 			Width:      int(f.Width),
// 			Height:     int(f.Height),
// 			Confidence: f.Confidence,
// 		})
// 	}

// 	result := DetectionResult{
// 		FacesDetected: len(boxes),
// 		BoundingBoxes: boxes,
// 		Timestamp:     time.Now().UnixMilli(),
// 	}

// 	if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
// 		data, _ := json.Marshal(result)
// 		if err := w.dataChannel.Send(data); err != nil {
// 			log.Printf("Error sending detection results: %v", err)
// 		} else {
// 			log.Printf("Sent detection results: %d faces", result.FacesDetected)
// 		}
// 	}
// }

// // --- Main ---
// func main() {
// 	signalingURL := os.Getenv("SIGNALING_URL")
// 	if signalingURL == "" {
// 		signalingURL = "ws://localhost:8080"
// 	}

// 	workerID := os.Getenv("WORKER_ID")
// 	if workerID == "" {
// 		workerID = "ingest_worker_1"
// 	}

// 	log.Printf("Starting ingest worker: %s", workerID)

// 	conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
// 	if err != nil {
// 		log.Fatal("Failed to connect to signaling server:", err)
// 	}
// 	defer conn.Close()

// 	worker := &IngestWorker{
// 		signalingURL: signalingURL,
// 		workerID:     workerID,
// 		wsConn:       conn,
// 	}

// 	// Initialize gRPC client
// 	grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
// 	if grpcEndpoint == "" {
// 		grpcEndpoint = "localhost:50051"
// 	}
// 	client, err := NewInferenceClient(grpcEndpoint)
// 	if err != nil {
// 		log.Fatal("Failed to initialize gRPC client:", err)
// 	}
// 	defer client.Close()
// 	worker.grpcClient = client

// 	go worker.handleSignalingMessages()
// 	log.Printf("Ingest worker ready: %s", worker.workerID)

// 	sigChan := make(chan os.Signal, 1)
// 	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
// 	<-sigChan

// 	log.Println("Shutting down ingest worker...")
// 	if worker.peerConnection != nil {
// 		worker.peerConnection.Close()
// 	}
// }




//3///////////////////////////////////////


package main

import (
	"encoding/json"
	"log"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/gorilla/websocket"
	"github.com/pion/webrtc/v3"
)

// --- Types for signaling ---
type DetectionBoundingBox struct {
	X          int     `json:"x"`
	Y          int     `json:"y"`
	Width      int     `json:"width"`
	Height     int     `json:"height"`
	Confidence float32 `json:"confidence"`
}

type DetectionResult struct {
	FacesDetected int                   `json:"faces_detected"`
	BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
	Timestamp     int64                 `json:"timestamp"`
}

// Fixed SignalMessage structure to match JavaScript format
type SignalMessage struct {
	Type      string                 `json:"type"`
	To        string                 `json:"to,omitempty"`
	From      string                 `json:"from,omitempty"`
	SDP       string                 `json:"sdp,omitempty"`
	Candidate map[string]interface{} `json:"candidate,omitempty"` // Changed to map to match JS
}

// --- Ingest Worker ---
type IngestWorker struct {
	signalingURL     string
	wsConn           *websocket.Conn
	wsMutex          sync.Mutex
	peerConnection   *webrtc.PeerConnection
	dataChannel      *webrtc.DataChannel
	currentClient    string
	workerID         string
	grpcClient       *InferenceClient
	queuedCandidates []SignalMessage
}

func (w *IngestWorker) sendWebSocketMessage(msg interface{}) error {
	w.wsMutex.Lock()
	defer w.wsMutex.Unlock()
	return w.wsConn.WriteJSON(msg)
}

// --- Initialize Peer Connection ---
func (w *IngestWorker) initializeWebRTC() error {
	config := webrtc.Configuration{
		ICEServers: []webrtc.ICEServer{
			{URLs: []string{"stun:stun.l.google.com:19302"}},
			{URLs: []string{"stun:stun1.l.google.com:19302"}},
		},
	}

	pc, err := webrtc.NewPeerConnection(config)
	if err != nil {
		return err
	}
	w.peerConnection = pc

	// Handle incoming tracks
	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		log.Printf("🎥 Video track received: %s, codec: %s", track.Kind().String(), track.Codec().MimeType)
		
		if track.Codec().MimeType == "video/VP8" {
			go w.processVP8Track(track)
		} else {
			log.Printf("❌ Unsupported codec: %s", track.Codec().MimeType)
		}
	})

	// ICE candidates - FIXED to match JavaScript format
	pc.OnICECandidate(func(candidate *webrtc.ICECandidate) {
		if candidate == nil || w.currentClient == "" {
			return
		}
		
		candidateJSON := candidate.ToJSON()
		msg := SignalMessage{
			Type: "candidate",
			To:   w.currentClient,
			From: w.workerID,
			Candidate: map[string]interface{}{
				"candidate":     candidateJSON.Candidate,
				"sdpMLineIndex": candidateJSON.SDPMLineIndex,
				"sdpMid":        candidateJSON.SDPMid,
			},
		}
		
		if err := w.sendWebSocketMessage(msg); err != nil {
			log.Printf("❌ Error sending ICE candidate: %v", err)
		} else {
			log.Printf("🧊 Sent ICE candidate to client")
		}
	})

	// Enhanced connection state handling
	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		log.Printf("🔗 PeerConnection state: %s", state.String())
		
		switch state {
		case webrtc.PeerConnectionStateConnected:
			log.Printf("✅ WebRTC connection established with client %s", w.currentClient)
		case webrtc.PeerConnectionStateFailed:
			log.Printf("❌ WebRTC connection failed - check firewall/NAT configuration")
		case webrtc.PeerConnectionStateDisconnected:
			log.Printf("⚠️ WebRTC connection disconnected")
		}
	})

	// ICE connection state for better debugging
	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		log.Printf("🧊 ICE connection state: %s", state.String())
	})

	// Data channel
	pc.OnDataChannel(func(dc *webrtc.DataChannel) {
		log.Printf("📡 Data channel received: %s", dc.Label())
		w.dataChannel = dc

		dc.OnOpen(func() {
			log.Printf("✅ Data channel opened with client %s", w.currentClient)
		})

		dc.OnClose(func() { 
			log.Printf("🔒 Data channel closed") 
		})
		
		dc.OnError(func(err error) { 
			log.Printf("❌ Data channel error: %v", err) 
		})
		
		dc.OnMessage(func(msg webrtc.DataChannelMessage) {
			log.Printf("📨 Received message on data channel: %d bytes", len(msg.Data))
		})
	})

	// Flush queued ICE candidates
	for _, msg := range w.queuedCandidates {
		w.handleCandidate(msg)
	}
	w.queuedCandidates = nil

	log.Printf("✅ WebRTC peer connection initialized")
	return nil
}

// --- Handle incoming offer ---
func (w *IngestWorker) handleOffer(msg SignalMessage) {
	log.Printf("📥 Received offer from %s", msg.From)
	w.currentClient = msg.From

	// Clean up existing connection
	if w.peerConnection != nil {
		w.peerConnection.Close()
		w.peerConnection = nil
		w.dataChannel = nil
		log.Printf("🔄 Cleaned up previous peer connection")
	}

	// Initialize new WebRTC connection
	if err := w.initializeWebRTC(); err != nil {
		log.Printf("❌ Failed to initialize WebRTC: %v", err)
		return
	}

	// Set remote description from offer
	offer := webrtc.SessionDescription{
		Type: webrtc.SDPTypeOffer,
		SDP:  msg.SDP,
	}

	if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
		log.Printf("❌ Error setting remote description: %v", err)
		return
	}
	log.Printf("✅ Set remote description from offer")

	// Create and set local answer
	answer, err := w.peerConnection.CreateAnswer(nil)
	if err != nil {
		log.Printf("❌ Error creating answer: %v", err)
		return
	}

	if err := w.peerConnection.SetLocalDescription(answer); err != nil {
		log.Printf("❌ Error setting local description: %v", err)
		return
	}
	log.Printf("✅ Created and set local answer")

	// Send answer back to client
	resp := SignalMessage{
		Type: "answer",
		To:   msg.From,
		From: w.workerID,
		SDP:  answer.SDP,
	}
	
	if err := w.sendWebSocketMessage(resp); err != nil {
		log.Printf("❌ Error sending answer: %v", err)
	} else {
		log.Printf("📤 Sent answer to client %s", msg.From)
	}
}

// --- Handle ICE candidate - FIXED to match JavaScript format ---
func (w *IngestWorker) handleCandidate(msg SignalMessage) {
	if w.peerConnection == nil {
		w.queuedCandidates = append(w.queuedCandidates, msg)
		log.Printf("⏳ Queued ICE candidate (waiting for peer connection)")
		return
	}

	if msg.Candidate == nil {
		log.Printf("❌ Candidate data is nil")
		return
	}

	ice := webrtc.ICECandidateInit{}
	
	// Extract candidate string
	candidateStr, ok := msg.Candidate["candidate"].(string)
	if !ok || candidateStr == "" {
		log.Printf("❌ Failed to extract candidate string from message")
		return
	}
	ice.Candidate = candidateStr
	
	// Extract optional SDP mid
	if sdpMid, ok := msg.Candidate["sdpMid"].(string); ok && sdpMid != "" {
		ice.SDPMid = &sdpMid
	}
	
	// Extract optional SDP line index
	if sdpMLineIndex, ok := msg.Candidate["sdpMLineIndex"].(float64); ok {
		idx := uint16(sdpMLineIndex)
		ice.SDPMLineIndex = &idx
	}
	
	if err := w.peerConnection.AddICECandidate(ice); err != nil {
		log.Printf("❌ Error adding ICE candidate: %v", err)
	} else {
		log.Printf("✅ Added ICE candidate from client")
	}
}

// --- Handle signaling messages ---
func (w *IngestWorker) handleSignalingMessages() {
	log.Printf("📡 Listening for signaling messages...")
	
	for {
		var msg SignalMessage
		if err := w.wsConn.ReadJSON(&msg); err != nil {
			log.Printf("❌ WebSocket read error: %v", err)
			// Attempt to reconnect after a delay
			time.Sleep(5 * time.Second)
			continue
		}

		log.Printf("📨 Received %s message from %s", msg.Type, msg.From)

		// Filter messages not intended for this worker
		if msg.To != "" && msg.To != w.workerID {
			log.Printf("⚠️ Message not for this worker (target: %s)", msg.To)
			continue
		}

		switch msg.Type {
		case "offer":
			w.handleOffer(msg)
		case "candidate":
			w.handleCandidate(msg)
		case "welcome":
			log.Printf("🎉 Welcome message received")
		default:
			log.Printf("⚠️ Unknown message type: %s", msg.Type)
		}
	}
}

// --- VP8 video track processing ---
func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
	var frameBuffer []byte
	var lastTimestamp uint32
	frameCount := 0
	lastLogTime := time.Now()

	log.Printf("🔍 Starting VP8 frame processing...")

	for {
		rtpPacket, _, err := track.ReadRTP()
		if err != nil {
			log.Printf("❌ Error reading RTP packet: %v", err)
			return
		}

		// Simple frame reconstruction based on RTP markers/timestamps
		if rtpPacket.Marker || (lastTimestamp != 0 && rtpPacket.Timestamp != lastTimestamp) {
			if len(frameBuffer) > 0 {
				frameCount++
				
				// Process every 10th frame to reduce load
				if frameCount%10 == 0 {
					go w.processVideoFrame(frameBuffer, 640, 480, frameCount)
				}
				frameBuffer = nil
			}
		}
		
		frameBuffer = append(frameBuffer, rtpPacket.Payload...)
		lastTimestamp = rtpPacket.Timestamp

		// Log progress every 5 seconds
		if time.Since(lastLogTime) > 5*time.Second {
			log.Printf("📊 Processed %d frames from VP8 track", frameCount)
			lastLogTime = time.Now()
		}
	}
}

// --- Process a single video frame ---
func (w *IngestWorker) processVideoFrame(frameData []byte, width, height, frameCount int) {
	if w.grpcClient == nil {
		log.Printf("❌ gRPC client not initialized")
		return
	}

	// Validate dimensions before sending to face detection
	if width <= 0 || height <= 0 || width > 4096 || height > 4096 {
		log.Printf("⚠️ Invalid dimensions %dx%d, using defaults", width, height)
		width, height = 640, 480
	}

	// Perform face detection via gRPC
	faces, _, err := w.grpcClient.DetectFaces(frameData, width, height, 3)
	if err != nil {
		log.Printf("❌ Face detection error: %v", err)
		return
	}

	// Convert results to JSON format
	var boundingBoxes []DetectionBoundingBox
	for _, f := range faces {
		boundingBoxes = append(boundingBoxes, DetectionBoundingBox{
			X:          int(f.X),
			Y:          int(f.Y),
			Width:      int(f.Width),
			Height:     int(f.Height),
			Confidence: f.Confidence,
		})
	}

	result := DetectionResult{
		FacesDetected: len(boundingBoxes),
		BoundingBoxes: boundingBoxes,
		Timestamp:     time.Now().UnixMilli(),
	}

	// Send results via data channel if available
	if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
		data, err := json.Marshal(result)
		if err != nil {
			log.Printf("❌ JSON marshaling error: %v", err)
			return
		}

		if err := w.dataChannel.Send(data); err != nil {
			log.Printf("❌ Error sending detection results: %v", err)
		} else if frameCount%30 == 0 { // Log every 30 processed frames
			log.Printf("✅ Sent detection results: %d faces (frame %d)", result.FacesDetected, frameCount)
		}
	} else {
		log.Printf("⚠️ Data channel not available for sending results")
	}
}

// --- Cleanup resources ---
func (w *IngestWorker) cleanup() {
	log.Printf("🧹 Cleaning up resources...")
	
	if w.peerConnection != nil {
		w.peerConnection.Close()
		w.peerConnection = nil
	}
	
	if w.dataChannel != nil {
		w.dataChannel.Close()
		w.dataChannel = nil
	}
	
	log.Printf("✅ Cleanup completed")
}

// --- Main function ---
func main() {
	// Configure from environment variables
	signalingURL := os.Getenv("SIGNALING_URL")
	if signalingURL == "" {
		signalingURL = "ws://localhost:8080"
	}

	workerID := os.Getenv("WORKER_ID")
	if workerID == "" {
		workerID = "ingest_worker_1"
	}

	log.Printf("🚀 Starting ingest worker: %s", workerID)
	log.Printf("📡 Signaling server: %s", signalingURL)

	// Connect to signaling server
	conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
	if err != nil {
		log.Fatal("❌ Failed to connect to signaling server:", err)
	}
	defer conn.Close()

	// Create worker instance
	worker := &IngestWorker{
		signalingURL: signalingURL,
		workerID:     workerID,
		wsConn:       conn,
	}

	// Initialize gRPC client for face detection
	grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
	if grpcEndpoint == "" {
		grpcEndpoint = "localhost:50051"
	}
	
	log.Printf("🔗 Connecting to gRPC endpoint: %s", grpcEndpoint)
	client, err := NewInferenceClient(grpcEndpoint)
	if err != nil {
		log.Fatal("❌ Failed to initialize gRPC client:", err)
	}
	defer client.Close()
	worker.grpcClient = client

	// Start handling signaling messages
	go worker.handleSignalingMessages()
	log.Printf("✅ Ingest worker %s ready and listening for WebRTC connections...", worker.workerID)

	// Wait for shutdown signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	
	<-sigChan
	log.Println("🛑 Shutting down ingest worker...")
	worker.cleanup()
	log.Println("✅ Ingest worker shutdown complete")
}
