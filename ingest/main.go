
//Rebuild and test

// cd ingest
// go build -o ../bin/ingest-worker .
// cd ..

// # Restart the ingest worker
// export SIGNALING_URL="ws://localhost:8080"
// export WORKER_ID="ingest_worker_1"
// ./bin/ingest-worker





//TEST WITH DUMMY VALUES///////////////////
// package main

// import (
//     "encoding/json"
//     "log"

//     "os"
//     "os/signal"
//     "sync"
//     "syscall"
//     "time"

//     "github.com/gorilla/websocket"
//     "github.com/pion/rtp/codecs"
//     "github.com/pion/webrtc/v3"
    
//     // REMOVED: pb "webrtc-pion-face-engine/proto" - This import is only used in detector_client.go
// )

// // --- Types for signaling ---
// type DetectionBoundingBox struct {
//     X          int     `json:"x"`
//     Y          int     `json:"y"`
//     Width      int     `json:"width"`
//     Height     int     `json:"height"`
//     Confidence float32 `json:"confidence"`
// }

// type DetectionResult struct {
//     FacesDetected int                   `json:"faces_detected"`
//     BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
//     Timestamp     int64                 `json:"timestamp"`
// }

// // Fixed SignalMessage structure to match JavaScript format
// type SignalMessage struct {
//     Type      string                 `json:"type"`
//     To        string                 `json:"to,omitempty"`
//     From      string                 `json:"from,omitempty"`
//     SDP       string                 `json:"sdp,omitempty"`
//     Candidate map[string]interface{} `json:"candidate,omitempty"`
// }

// // --- Ingest Worker ---
// type IngestWorker struct {
//     signalingURL     string
//     wsConn           *websocket.Conn
//     wsMutex          sync.Mutex
//     peerConnection   *webrtc.PeerConnection
//     dataChannel      *webrtc.DataChannel
//     currentClient    string
//     workerID         string
//     grpcClient       *InferenceClient
//     queuedCandidates []SignalMessage
// }

// func (w *IngestWorker) sendWebSocketMessage(msg interface{}) error {
//     w.wsMutex.Lock()
//     defer w.wsMutex.Unlock()
//     return w.wsConn.WriteJSON(msg)
// }

// // --- Initialize Peer Connection ---
// func (w *IngestWorker) initializeWebRTC() error {
//     config := webrtc.Configuration{
//         ICEServers: []webrtc.ICEServer{
//             {URLs: []string{"stun:stun.l.google.com:19302"}},
//             {URLs: []string{"stun:stun1.l.google.com:19302"}},
//         },
//     }

//     pc, err := webrtc.NewPeerConnection(config)
//     if err != nil {
//         return err
//     }
//     w.peerConnection = pc

//     // Handle incoming tracks
//     pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
//         log.Printf("🎥 Video track received: %s, codec: %s", track.Kind().String(), track.Codec().MimeType)
        
//         if track.Codec().MimeType == "video/VP8" {
//             go w.processVP8Track(track)
//         } else {
//             log.Printf("❌ Unsupported codec: %s", track.Codec().MimeType)
//         }
//     })

//     // ICE candidates
//     pc.OnICECandidate(func(candidate *webrtc.ICECandidate) {
//         if candidate == nil || w.currentClient == "" {
//             return
//         }
        
//         candidateJSON := candidate.ToJSON()
//         msg := SignalMessage{
//             Type: "candidate",
//             To:   w.currentClient,
//             From: w.workerID,
//             Candidate: map[string]interface{}{
//                 "candidate":     candidateJSON.Candidate,
//                 "sdpMLineIndex": candidateJSON.SDPMLineIndex,
//                 "sdpMid":        candidateJSON.SDPMid,
//             },
//         }
        
//         if err := w.sendWebSocketMessage(msg); err != nil {
//             log.Printf("❌ Error sending ICE candidate: %v", err)
//         } else {
//             log.Printf("🧊 Sent ICE candidate to client")
//         }
//     })

//     // Enhanced connection state handling
//     pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
//         log.Printf("🔗 PeerConnection state: %s", state.String())
        
//         switch state {
//         case webrtc.PeerConnectionStateConnected:
//             log.Printf("✅ WebRTC connection established with client %s", w.currentClient)
//         case webrtc.PeerConnectionStateFailed:
//             log.Printf("❌ WebRTC connection failed - check firewall/NAT configuration")
//         case webrtc.PeerConnectionStateDisconnected:
//             log.Printf("⚠️ WebRTC connection disconnected")
//         }
//     })

//     // ICE connection state for better debugging
//     pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
//         log.Printf("🧊 ICE connection state: %s", state.String())
//     })

//     // Data channel
//     pc.OnDataChannel(func(dc *webrtc.DataChannel) {
//         log.Printf("📡 Data channel received: %s", dc.Label())
//         w.dataChannel = dc

//         dc.OnOpen(func() {
//             log.Printf("✅ Data channel opened with client %s", w.currentClient)
//         })

//         dc.OnClose(func() { 
//             log.Printf("🔒 Data channel closed") 
//         })
        
//         dc.OnError(func(err error) { 
//             log.Printf("❌ Data channel error: %v", err) 
//         })
//     })

//     // Flush queued ICE candidates
//     for _, msg := range w.queuedCandidates {
//         w.handleCandidate(msg)
//     }
//     w.queuedCandidates = nil

//     log.Printf("✅ WebRTC peer connection initialized")
//     return nil
// }

// // --- Handle incoming offer ---
// func (w *IngestWorker) handleOffer(msg SignalMessage) {
//     log.Printf("📥 Received offer from %s", msg.From)
//     w.currentClient = msg.From

//     // Clean up existing connection
//     if w.peerConnection != nil {
//         w.peerConnection.Close()
//         w.peerConnection = nil
//         w.dataChannel = nil
//         log.Printf("🔄 Cleaned up previous peer connection")
//     }

//     // Initialize new WebRTC connection
//     if err := w.initializeWebRTC(); err != nil {
//         log.Printf("❌ Failed to initialize WebRTC: %v", err)
//         return
//     }

//     // Set remote description from offer
//     offer := webrtc.SessionDescription{
//         Type: webrtc.SDPTypeOffer,
//         SDP:  msg.SDP,
//     }

//     if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
//         log.Printf("❌ Error setting remote description: %v", err)
//         return
//     }
//     log.Printf("✅ Set remote description from offer")

//     // Create and set local answer
//     answer, err := w.peerConnection.CreateAnswer(nil)
//     if err != nil {
//         log.Printf("❌ Error creating answer: %v", err)
//         return
//     }

//     if err := w.peerConnection.SetLocalDescription(answer); err != nil {
//         log.Printf("❌ Error setting local description: %v", err)
//         return
//     }
//     log.Printf("✅ Created and set local answer")

//     // Send answer back to client
//     resp := SignalMessage{
//         Type: "answer",
//         To:   msg.From,
//         From: w.workerID,
//         SDP:  answer.SDP,
//     }
    
//     if err := w.sendWebSocketMessage(resp); err != nil {
//         log.Printf("❌ Error sending answer: %v", err)
//     } else {
//         log.Printf("📤 Sent answer to client %s", msg.From)
//     }
// }

// // --- Handle ICE candidate ---
// func (w *IngestWorker) handleCandidate(msg SignalMessage) {
//     if w.peerConnection == nil {
//         w.queuedCandidates = append(w.queuedCandidates, msg)
//         log.Printf("⏳ Queued ICE candidate (waiting for peer connection)")
//         return
//     }

//     if msg.Candidate == nil {
//         log.Printf("❌ Candidate data is nil")
//         return
//     }

//     ice := webrtc.ICECandidateInit{}
    
//     // Extract candidate string
//     candidateStr, ok := msg.Candidate["candidate"].(string)
//     if !ok || candidateStr == "" {
//         log.Printf("❌ Failed to extract candidate string from message")
//         return
//     }
//     ice.Candidate = candidateStr
    
//     // Extract optional SDP mid
//     if sdpMid, ok := msg.Candidate["sdpMid"].(string); ok && sdpMid != "" {
//         ice.SDPMid = &sdpMid
//     }
    
//     // Extract optional SDP line index
//     if sdpMLineIndex, ok := msg.Candidate["sdpMLineIndex"].(float64); ok {
//         idx := uint16(sdpMLineIndex)
//         ice.SDPMLineIndex = &idx
//     }
    
//     if err := w.peerConnection.AddICECandidate(ice); err != nil {
//         log.Printf("❌ Error adding ICE candidate: %v", err)
//     } else {
//         log.Printf("✅ Added ICE candidate from client")
//     }
// }

// // --- Handle signaling messages ---
// func (w *IngestWorker) handleSignalingMessages() {
//     log.Printf("📡 Listening for signaling messages...")
    
//     for {
//         var msg SignalMessage
//         if err := w.wsConn.ReadJSON(&msg); err != nil {
//             log.Printf("❌ WebSocket read error: %v", err)
//             // Attempt to reconnect after a delay
//             time.Sleep(5 * time.Second)
//             continue
//         }

//         log.Printf("📨 Received %s message from %s", msg.Type, msg.From)

//         // Filter messages not intended for this worker
//         if msg.To != "" && msg.To != w.workerID {
//             log.Printf("⚠️ Message not for this worker (target: %s)", msg.To)
//             continue
//         }

//         switch msg.Type {
//         case "offer":
//             w.handleOffer(msg)
//         case "candidate":
//             w.handleCandidate(msg)
//         case "welcome":
//             log.Printf("🎉 Welcome message received")
//         default:

//             log.Printf("⚠️ Unknown message type: %s", msg.Type)
//         }
//     }
// }

// // // --- Proper VP8 frame reconstruction ---
// func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
//     frameCount := 0
//     lastLogTime := time.Now()

//     log.Printf("🔍 Starting VP8 frame processing...")

//     for {
//         rtpPacket, _, err := track.ReadRTP()
//         if err != nil {
//             log.Printf("❌ Error reading RTP packet: %v", err)
//             return
//         }

//         // Create a new VP8Packet for each RTP packet
//         vp8Packet := &codecs.VP8Packet{}
        
//         // Fixed: Capture both return values from Unmarshal
//         payload, err := vp8Packet.Unmarshal(rtpPacket.Payload)
//         if err != nil {
//             log.Printf("❌ VP8 depacketizing error: %v", err)
//             continue
//         }

//         frameCount++

//         // Process every 10th frame to reduce load
//         if frameCount%10 == 0 && len(payload) > 0 {
//             // Use reasonable default dimensions for VP8
//             width, height := 640, 480
            
//             log.Printf("🖼️ Processed VP8 packet %d: %d bytes, %dx%d pixels", 
//                 frameCount, len(payload), width, height)
//             go w.processVideoFrame(payload, width, height, frameCount)
//         }



//         // Log progress every 5 seconds
//         if time.Since(lastLogTime) > 5*time.Second {
//             log.Printf("📊 Processed %d RTP packets", frameCount)
//             lastLogTime = time.Now()
//         }
//     }
// }


// // --- Process a single video frame ---
// func (w *IngestWorker) processVideoFrame(frameData []byte, width, height, frameCount int) {
//     log.Printf("🔍 Frame %d data: %d bytes (first 20 bytes: %x...)", 
//     frameCount, len(frameData), frameData[:min(20, len(frameData))])
    
//     // Check if this looks like a valid VP8 frame
//     if len(frameData) < 10 || frameData[0] != 0x9D || frameData[1] != 0x01 || frameData[2] != 0x2A {
//         log.Printf("⚠️ Data doesn't look like a valid VP8 frame!")
//     }
   
   
//     if w.grpcClient == nil {
//         log.Printf("❌ gRPC client not initialized")
//         return
//     }

//     // Validate dimensions before sending to face detection
//     if width <= 0 || height <= 0 || width > 4096 || height > 4096 {
//         log.Printf("⚠️ Invalid dimensions %dx%d, using defaults", width, height)
//         width, height = 640, 480
//     }

//     // Skip VP8 encoded data for now - we need to decode it first
//     // For now, create a dummy grayscale frame to test the pipeline
//     dummyFrameSize := width * height
//     dummyFrame := make([]byte, dummyFrameSize)
    
//     // Fill with a simple pattern to simulate grayscale image data
//     for i := 0; i < dummyFrameSize; i++ {
//         dummyFrame[i] = byte((i % 255) + frameCount%50) // Simple pattern that changes over time
//     }

//     // Log VP8 data info for debugging
//     if frameCount%50 == 0 {
//         log.Printf("🔍 VP8 frame debug - Original size: %d bytes, Creating dummy %dx%d grayscale", 
//             len(frameData), width, height)
//     }

//     // Perform face detection via gRPC using dummy frame
//     // Updated to match the corrected detector_client.go
//     faces, processingTime, err := w.grpcClient.DetectFaces(dummyFrame, width, height, 1)
//     if err != nil {
//         log.Printf("❌ Face detection error: %v", err)
//         return
//     }

//     // Convert results to JSON format - Updated to handle []*pb.BoundingBox
//     var boundingBoxes []DetectionBoundingBox
//     for _, f := range faces {
//         boundingBoxes = append(boundingBoxes, DetectionBoundingBox{
//             X:          int(f.GetX()),      // Use GetX() for protobuf fields
//             Y:          int(f.GetY()),      // Use GetY() for protobuf fields
//             Width:      int(f.GetWidth()),  // Use GetWidth() for protobuf fields
//             Height:     int(f.GetHeight()), // Use GetHeight() for protobuf fields
//             Confidence: f.GetConfidence(),  // Use GetConfidence() for protobuf fields
//         })
//     }

//     result := DetectionResult{
//         FacesDetected: len(boundingBoxes),
//         BoundingBoxes: boundingBoxes,
//         Timestamp:     time.Now().UnixMilli(),
//     }

//     // Send results via data channel if available
//     if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
//         data, err := json.Marshal(result)
//         if err != nil {
//             log.Printf("❌ JSON marshaling error: %v", err)
//             return
//         }

//         if err := w.dataChannel.Send(data); err != nil {
//             log.Printf("❌ Error sending detection results: %v", err)
//         } else if frameCount%30 == 0 { // Log every 30 processed frames
//             log.Printf("✅ Sent detection results: %d faces (frame %d), Processing time: %d ms", 
//                 result.FacesDetected, frameCount, processingTime)
//         }
//     } else {
//         log.Printf("⚠️ Data channel not available for sending results")
//     }
// }

// func (w *IngestWorker) decodeVP8Frame(vp8Data []byte, width, height int) ([]byte, error) {
//     // For now, return an error to trigger the test pattern fallback
//     // This avoids the complex VP8 decoding issues
//     return nil, fmt.Errorf("VP8 decoding temporarily disabled - using test pattern")
// }

// func (w *IngestWorker) createTestPattern(width, height, frameCount int) []byte {
//     frameSize := width * height
//     testFrame := make([]byte, frameSize)
    
//     // Simple test pattern without math package
//     centerX, centerY := width/2, height/2
//     faceRadius := width / 3
//     if height < width {
//         faceRadius = height / 3
//     }
    
//     for y := 0; y < height; y++ {
//         for x := 0; x < width; x++ {
//             // Calculate distance squared (avoid math.Sqrt)
//             dx, dy := x-centerX, y-centerY
//             distanceSquared := dx*dx + dy*dy
//             radiusSquared := faceRadius * faceRadius
            
//             if distanceSquared < radiusSquared {
//                 // Simple gradient based on distance
//                 intensity := byte(200 * (radiusSquared - distanceSquared) / radiusSquared)
//                 testFrame[y*width+x] = intensity
//             } else {
//                 testFrame[y*width+x] = 30
//             }
//         }
//     }
    
//     log.Printf("🎨 Created test pattern: %d bytes", len(testFrame))
//     return testFrame
// }

// // ✅ VP8 Decoding function (you need to implement this properly)
// func (w *IngestWorker) decodeVP8Frame(vp8Data []byte, width, height int) ([]byte, error) {
//     // TODO: Implement proper VP8 decoding using the libvpx-go library
//     // For now, return an error to trigger the fallback
//     return nil, fmt.Errorf("VP8 decoding not yet implemented")
// }

// // ✅ Fallback test pattern function
// func (w *IngestWorker) createTestPattern(width, height, frameCount int) []byte {
//     frameSize := width * height
//     testFrame := make([]byte, frameSize)
    
//     // Create a realistic face-like pattern
//     centerX, centerY := width/2, height/2
//     faceRadius := min(width, height) / 3
    
//     for y := 0; y < height; y++ {
//         for x := 0; x < width; x++ {
//             distance := math.Sqrt(float64((x-centerX)*(x-centerX) + (y-centerY)*(y-centerY)))
            
//             if distance < float64(faceRadius) {
//                 // Face area - gradient
//                 intensity := byte(200 * (1 - distance/float64(faceRadius)))
//                 testFrame[y*width+x] = intensity
//             } else {
//                 // Background
//                 testFrame[y*width+x] = 30
//             }
//         }
//     }
    
//     log.Printf("🎨 Created test pattern: %d bytes", len(testFrame))
//     return testFrame
// }

// func min(a, b int) int {
//     if a < b {
//         return a
//     }
//     return b
// }

// // --- Cleanup resources ---
// func (w *IngestWorker) cleanup() {
//     log.Printf("🧹 Cleaning up resources...")
    
//     if w.peerConnection != nil {
//         w.peerConnection.Close()
//         w.peerConnection = nil
//     }
    
//     if w.dataChannel != nil {
//         w.dataChannel.Close()
//         w.dataChannel = nil
//     }
    
//     log.Printf("✅ Cleanup completed")
// }

// // --- Main function ---
// func main() {
//     // Configure from environment variables
//     signalingURL := os.Getenv("SIGNALING_URL")
//     if signalingURL == "" {
//         signalingURL = "ws://localhost:8080"
//     }

//     workerID := os.Getenv("WORKER_ID")
//     if workerID == "" {
//         workerID = "ingest_worker_1"
//     }

//     log.Printf("🚀 Starting ingest worker: %s", workerID)
//     log.Printf("📡 Signaling server: %s", signalingURL)

//     // Connect to signaling server
//     conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
//     if err != nil {
//         log.Fatal("❌ Failed to connect to signaling server:", err)
//     }
//     defer conn.Close()

//     // Create worker instance
//     worker := &IngestWorker{
//         signalingURL: signalingURL,
//         workerID:     workerID,
//         wsConn:       conn,
//     }

//     // Initialize gRPC client for face detection
//     grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
//     if grpcEndpoint == "" {
//         grpcEndpoint = "localhost:50051"
//     }
    
//     log.Printf("🔗 Connecting to gRPC endpoint: %s", grpcEndpoint)
//     client, err := NewInferenceClient(grpcEndpoint)
//     if err != nil {
//         log.Fatal("❌ Failed to initialize gRPC client:", err)
//     }
//     defer client.Close()
//     worker.grpcClient = client

//     // Start handling signaling messages
//     go worker.handleSignalingMessages()
//     log.Printf("✅ Ingest worker %s ready and listening for WebRTC connections...", worker.workerID)

//     // Wait for shutdown signal
//     sigChan := make(chan os.Signal, 1)
//     signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
    
//     <-sigChan
//     log.Println("🛑 Shutting down ingest worker...")
//     worker.cleanup()
//     log.Println("✅ Ingest worker shutdown complete")
// }




























//WORKING!!!   TEST PATTERN!///////////////////
// package main

// import (
//     "encoding/json"
//     "log"
//     "os"
//     "os/signal"
//     "sync"
//     "syscall"
//     "time"

//     "github.com/gorilla/websocket"
//     "github.com/pion/rtp/codecs"
//     "github.com/pion/webrtc/v3"
// )

// // --- Types for signaling ---
// type DetectionBoundingBox struct {
//     X          int     `json:"x"`
//     Y          int     `json:"y"`
//     Width      int     `json:"width"`
//     Height     int     `json:"height"`
//     Confidence float32 `json:"confidence"`
// }

// type DetectionResult struct {
//     FacesDetected int                   `json:"faces_detected"`
//     BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
//     Timestamp     int64                 `json:"timestamp"`
// }

// // Fixed SignalMessage structure to match JavaScript format
// type SignalMessage struct {
//     Type      string                 `json:"type"`
//     To        string                 `json:"to,omitempty"`
//     From      string                 `json:"from,omitempty"`
//     SDP       string                 `json:"sdp,omitempty"`
//     Candidate map[string]interface{} `json:"candidate,omitempty"`
// }

// // --- Ingest Worker ---
// type IngestWorker struct {
//     signalingURL     string
//     wsConn           *websocket.Conn
//     wsMutex          sync.Mutex
//     peerConnection   *webrtc.PeerConnection
//     dataChannel      *webrtc.DataChannel
//     currentClient    string
//     workerID         string
//     grpcClient       *InferenceClient
//     queuedCandidates []SignalMessage
// }

// func (w *IngestWorker) sendWebSocketMessage(msg interface{}) error {
//     w.wsMutex.Lock()
//     defer w.wsMutex.Unlock()
//     return w.wsConn.WriteJSON(msg)
// }

// // --- Initialize Peer Connection ---
// func (w *IngestWorker) initializeWebRTC() error {
//     config := webrtc.Configuration{
//         ICEServers: []webrtc.ICEServer{
//             {URLs: []string{"stun:stun.l.google.com:19302"}},
//             {URLs: []string{"stun:stun1.l.google.com:19302"}},
//         },
//     }

//     pc, err := webrtc.NewPeerConnection(config)
//     if err != nil {
//         return err
//     }
//     w.peerConnection = pc

//     // Handle incoming tracks
//     pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
//         log.Printf("🎥 Video track received: %s, codec: %s", track.Kind().String(), track.Codec().MimeType)
        
//         if track.Codec().MimeType == "video/VP8" {
//             go w.processVP8Track(track)
//         } else {
//             log.Printf("❌ Unsupported codec: %s", track.Codec().MimeType)
//         }
//     })

//     // ICE candidates
//     pc.OnICECandidate(func(candidate *webrtc.ICECandidate) {
//         if candidate == nil || w.currentClient == "" {
//             return
//         }
        
//         candidateJSON := candidate.ToJSON()
//         msg := SignalMessage{
//             Type: "candidate",
//             To:   w.currentClient,
//             From: w.workerID,
//             Candidate: map[string]interface{}{
//                 "candidate":     candidateJSON.Candidate,
//                 "sdpMLineIndex": candidateJSON.SDPMLineIndex,
//                 "sdpMid":        candidateJSON.SDPMid,
//             },
//         }
        
//         if err := w.sendWebSocketMessage(msg); err != nil {
//             log.Printf("❌ Error sending ICE candidate: %v", err)
//         } else {
//             log.Printf("🧊 Sent ICE candidate to client")
//         }
//     })

//     // Enhanced connection state handling
//     pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
//         log.Printf("🔗 PeerConnection state: %s", state.String())
        
//         switch state {
//         case webrtc.PeerConnectionStateConnected:
//             log.Printf("✅ WebRTC connection established with client %s", w.currentClient)
//         case webrtc.PeerConnectionStateFailed:
//             log.Printf("❌ WebRTC connection failed - check firewall/NAT configuration")
//         case webrtc.PeerConnectionStateDisconnected:
//             log.Printf("⚠️ WebRTC connection disconnected")
//         }
//     })

//     // ICE connection state for better debugging
//     pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
//         log.Printf("🧊 ICE connection state: %s", state.String())
//     })

//     // Data channel
//     pc.OnDataChannel(func(dc *webrtc.DataChannel) {
//         log.Printf("📡 Data channel received: %s", dc.Label())
//         w.dataChannel = dc

//         dc.OnOpen(func() {
//             log.Printf("✅ Data channel opened with client %s", w.currentClient)
//         })

//         dc.OnClose(func() { 
//             log.Printf("🔒 Data channel closed") 
//         })
        
//         dc.OnError(func(err error) { 
//             log.Printf("❌ Data channel error: %v", err) 
//         })
//     })

//     // Flush queued ICE candidates
//     for _, msg := range w.queuedCandidates {
//         w.handleCandidate(msg)
//     }
//     w.queuedCandidates = nil

//     log.Printf("✅ WebRTC peer connection initialized")
//     return nil
// }

// // --- Handle incoming offer ---
// func (w *IngestWorker) handleOffer(msg SignalMessage) {
//     log.Printf("📥 Received offer from %s", msg.From)
//     w.currentClient = msg.From

//     // Clean up existing connection
//     if w.peerConnection != nil {
//         w.peerConnection.Close()
//         w.peerConnection = nil
//         w.dataChannel = nil
//         log.Printf("🔄 Cleaned up previous peer connection")
//     }

//     // Initialize new WebRTC connection
//     if err := w.initializeWebRTC(); err != nil {
//         log.Printf("❌ Failed to initialize WebRTC: %v", err)
//         return
//     }

//     // Set remote description from offer
//     offer := webrtc.SessionDescription{
//         Type: webrtc.SDPTypeOffer,
//         SDP:  msg.SDP,
//     }

//     if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
//         log.Printf("❌ Error setting remote description: %v", err)
//         return
//     }
//     log.Printf("✅ Set remote description from offer")

//     // Create and set local answer
//     answer, err := w.peerConnection.CreateAnswer(nil)
//     if err != nil {
//         log.Printf("❌ Error creating answer: %v", err)
//         return
//     }

//     if err := w.peerConnection.SetLocalDescription(answer); err != nil {
//         log.Printf("❌ Error setting local description: %v", err)
//         return
//     }
//     log.Printf("✅ Created and set local answer")

//     // Send answer back to client
//     resp := SignalMessage{
//         Type: "answer",
//         To:   msg.From,
//         From: w.workerID,
//         SDP:  answer.SDP,
//     }
    
//     if err := w.sendWebSocketMessage(resp); err != nil {
//         log.Printf("❌ Error sending answer: %v", err)
//     } else {
//         log.Printf("📤 Sent answer to client %s", msg.From)
//     }
// }

// // --- Handle ICE candidate ---
// func (w *IngestWorker) handleCandidate(msg SignalMessage) {
//     if w.peerConnection == nil {
//         w.queuedCandidates = append(w.queuedCandidates, msg)
//         log.Printf("⏳ Queued ICE candidate (waiting for peer connection)")
//         return
//     }

//     if msg.Candidate == nil {
//         log.Printf("❌ Candidate data is nil")
//         return
//     }

//     ice := webrtc.ICECandidateInit{}
    
//     // Extract candidate string
//     candidateStr, ok := msg.Candidate["candidate"].(string)
//     if !ok || candidateStr == "" {
//         log.Printf("❌ Failed to extract candidate string from message")
//         return
//     }
//     ice.Candidate = candidateStr
    
//     // Extract optional SDP mid
//     if sdpMid, ok := msg.Candidate["sdpMid"].(string); ok && sdpMid != "" {
//         ice.SDPMid = &sdpMid
//     }
    
//     // Extract optional SDP line index
//     if sdpMLineIndex, ok := msg.Candidate["sdpMLineIndex"].(float64); ok {
//         idx := uint16(sdpMLineIndex)
//         ice.SDPMLineIndex = &idx
//     }
    
//     if err := w.peerConnection.AddICECandidate(ice); err != nil {
//         log.Printf("❌ Error adding ICE candidate: %v", err)
//     } else {
//         log.Printf("✅ Added ICE candidate from client")
//     }
// }

// // --- Handle signaling messages ---
// func (w *IngestWorker) handleSignalingMessages() {
//     log.Printf("📡 Listening for signaling messages...")
    
//     for {
//         var msg SignalMessage
//         if err := w.wsConn.ReadJSON(&msg); err != nil {
//             log.Printf("❌ WebSocket read error: %v", err)
//             // Attempt to reconnect after a delay
//             time.Sleep(5 * time.Second)
//             continue
//         }

//         log.Printf("📨 Received %s message from %s", msg.Type, msg.From)

//         // Filter messages not intended for this worker
//         if msg.To != "" && msg.To != w.workerID {
//             log.Printf("⚠️ Message not for this worker (target: %s)", msg.To)
//             continue
//         }

//         switch msg.Type {
//         case "offer":
//             w.handleOffer(msg)
//         case "candidate":
//             w.handleCandidate(msg)
//         case "welcome":
//             log.Printf("🎉 Welcome message received")
//         default:
//             log.Printf("⚠️ Unknown message type: %s", msg.Type)
//         }
//     }
// }

// // --- Proper VP8 frame reconstruction ---
// func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
//     frameCount := 0
//     lastLogTime := time.Now()
//     var frameBuffer []byte
//     var lastTimestamp uint32

//     log.Printf("🔍 Starting VP8 frame processing...")

//     for {
//         rtpPacket, _, err := track.ReadRTP()
//         if err != nil {
//             log.Printf("❌ Error reading RTP packet: %v", err)
//             return
//         }

//         vp8Packet := &codecs.VP8Packet{}
//         payload, err := vp8Packet.Unmarshal(rtpPacket.Payload)
//         if err != nil {
//             log.Printf("❌ VP8 depacketizing error: %v", err)
//             continue
//         }

//         // Frame reassembly logic
//         if lastTimestamp != 0 && rtpPacket.Timestamp != lastTimestamp {
//             // Complete frame assembled
//             if len(frameBuffer) > 0 {
//                 frameCount++
//                 if frameCount%10 == 0 { // Process every 10th frame
//                     width, height := 640, 480
//                     log.Printf("🖼️ Assembled complete VP8 frame %d: %d bytes", frameCount, len(frameBuffer))
//                     go w.processVideoFrame(frameBuffer, width, height, frameCount)
//                 }
//                 frameBuffer = nil
//             }
//         }

//         frameBuffer = append(frameBuffer, payload...)
//         lastTimestamp = rtpPacket.Timestamp

//         // Log progress every 5 seconds
//         if time.Since(lastLogTime) > 5*time.Second {
//             log.Printf("📊 Processed %d RTP packets", frameCount)
//             lastLogTime = time.Now()
//         }
//     }
// }

// // --- Process a single video frame ---
// func (w *IngestWorker) processVideoFrame(frameData []byte, width, height, frameCount int) {
//     expectedRawSize := width * height * 1 // 307,200 for 640x480 grayscale
    
//     // Check if we're receiving compressed data
//     if len(frameData) < expectedRawSize/10 {
//         log.Printf("🔴 Compressed VP8 data: %d bytes (expected raw: %d bytes)", 
//             len(frameData), expectedRawSize)
//     }
   
//     if w.grpcClient == nil {
//         log.Printf("❌ gRPC client not initialized")
//         return
//     }

//     // Validate dimensions
//     if width <= 0 || height <= 0 || width > 4096 || height > 4096 {
//         log.Printf("⚠️ Invalid dimensions %dx%d, using defaults", width, height)
//         width, height = 640, 480
//     }

//     // ✅ ALWAYS USE TEST PATTERN (properly sized raw pixels)
//     rawFrameData := w.createTestPattern(width, height, frameCount)
    
//     log.Printf("✅ Sending %d bytes of raw frame data to face detection", len(rawFrameData))

//     // Send properly formatted raw frame to face detection
//     faces, processingTime, err := w.grpcClient.DetectFaces(rawFrameData, width, height, 1)
//     if err != nil {
//         log.Printf("❌ Face detection error: %v", err)
//         return
//     }

//     // Convert results to JSON format
//     var boundingBoxes []DetectionBoundingBox
//     for _, f := range faces {
//         boundingBoxes = append(boundingBoxes, DetectionBoundingBox{
//             X:          int(f.GetX()),
//             Y:          int(f.GetY()),
//             Width:      int(f.GetWidth()),
//             Height:     int(f.GetHeight()),
//             Confidence: f.GetConfidence(),
//         })
//     }

//     result := DetectionResult{
//         FacesDetected: len(boundingBoxes),
//         BoundingBoxes: boundingBoxes,
//         Timestamp:     time.Now().UnixMilli(),
//     }

//     // Send results via data channel if available
//     if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
//         data, err := json.Marshal(result)
//         if err != nil {
//             log.Printf("❌ JSON marshaling error: %v", err)
//             return
//         }

//         if err := w.dataChannel.Send(data); err != nil {
//             log.Printf("❌ Error sending detection results: %v", err)
//         } else {
//             log.Printf("✅ Face detection results: %d faces (frame %d), Processing time: %d ms", 
//                 result.FacesDetected, frameCount, processingTime)
//         }
//     } else {
//         log.Printf("⚠️ Data channel not available for sending results")
//     }
// }

// // --- Create properly sized test pattern for face detection ---
// func (w *IngestWorker) createTestPattern(width, height, frameCount int) []byte {
//     frameSize := width * height
//     testFrame := make([]byte, frameSize)
    
//     // Initialize with dark background
//     for i := range testFrame {
//         testFrame[i] = 30 // Dark background
//     }
    
//     centerX, centerY := width/2, height/2
//     faceRadius := min(width, height) / 3
    
//     // Create face oval (brighter)
//     for y := centerY - faceRadius; y <= centerY + faceRadius; y++ {
//         if y < 0 || y >= height { continue }
        
//         for x := centerX - faceRadius; x <= centerX + faceRadius; x++ {
//             if x < 0 || x >= width { continue }
            
//             // Oval shape (wider than tall)
//             dx, dy := float64(x-centerX), float64(y-centerY)
//             // Oval equation: (dx/a)^2 + (dy/b)^2 <= 1
//             a, b := float64(faceRadius), float64(faceRadius)*0.8
//             if (dx*dx)/(a*a) + (dy*dy)/(b*b) <= 1.0 {
//                 testFrame[y*width+x] = 200 // Bright face area
//             }
//         }
//     }
    
//     // Add clear, distinct eyes
//     eyeRadius := faceRadius / 6
//     leftEyeX, rightEyeX := centerX-faceRadius/3, centerX+faceRadius/3
//     eyeY := centerY - faceRadius/4
    
//     // Left eye (dark circle)
//     for y := eyeY - eyeRadius; y <= eyeY + eyeRadius; y++ {
//         if y < 0 || y >= height { continue }
//         for x := leftEyeX - eyeRadius; x <= leftEyeX + eyeRadius; x++ {
//             if x < 0 || x >= width { continue }
//             dx, dy := x-leftEyeX, y-eyeY
//             if dx*dx+dy*dy <= eyeRadius*eyeRadius {
//                 testFrame[y*width+x] = 50 // Dark eyes
//             }
//         }
//     }
    
//     // Right eye (dark circle)
//     for y := eyeY - eyeRadius; y <= eyeY + eyeRadius; y++ {
//         if y < 0 || y >= height { continue }
//         for x := rightEyeX - eyeRadius; x <= rightEyeX + eyeRadius; x++ {
//             if x < 0 || x >= width { continue }
//             dx, dy := x-rightEyeX, y-eyeY
//             if dx*dx+dy*dy <= eyeRadius*eyeRadius {
//                 testFrame[y*width+x] = 50 // Dark eyes
//             }
//         }
//     }
    
//     // Add a mouth (horizontal line)
//     mouthY := centerY + faceRadius/3
//     mouthWidth := faceRadius / 2
//     for x := centerX - mouthWidth; x <= centerX + mouthWidth; x++ {
//         if x >= 0 && x < width && mouthY >= 0 && mouthY < height {
//             testFrame[mouthY*width+x] = 50 // Dark mouth
//         }
//     }
    
//     log.Printf("🎨 Created realistic face pattern with eyes and mouth: %d bytes", len(testFrame))
//     return testFrame
// }





// func min(a, b int) int {
//     if a < b {
//         return a
//     }
//     return b
// }

// // --- Cleanup resources ---
// func (w *IngestWorker) cleanup() {
//     log.Printf("🧹 Cleaning up resources...")
    
//     if w.peerConnection != nil {
//         w.peerConnection.Close()
//         w.peerConnection = nil
//     }
    
//     if w.dataChannel != nil {
//         w.dataChannel.Close()
//         w.dataChannel = nil
//     }
    
//     log.Printf("✅ Cleanup completed")
// }

// // --- Main function ---
// func main() {
//     // Configure from environment variables
//     signalingURL := os.Getenv("SIGNALING_URL")
//     if signalingURL == "" {
//         signalingURL = "ws://localhost:8080"
//     }

//     workerID := os.Getenv("WORKER_ID")
//     if workerID == "" {
//         workerID = "ingest_worker_1"
//     }

//     log.Printf("🚀 Starting ingest worker: %s", workerID)
//     log.Printf("📡 Signaling server: %s", signalingURL)

//     // Connect to signaling server
//     conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
//     if err != nil {
//         log.Fatal("❌ Failed to connect to signaling server:", err)
//     }
//     defer conn.Close()

//     // Create worker instance
//     worker := &IngestWorker{
//         signalingURL: signalingURL,
//         workerID:     workerID,
//         wsConn:       conn,
//     }

//     // Initialize gRPC client for face detection
//     grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
//     if grpcEndpoint == "" {
//         grpcEndpoint = "localhost:50051"
//     }
    
//     log.Printf("🔗 Connecting to gRPC endpoint: %s", grpcEndpoint)
//     client, err := NewInferenceClient(grpcEndpoint)
//     if err != nil {
//         log.Fatal("❌ Failed to initialize gRPC client:", err)
//     }
//     defer client.Close()
//     worker.grpcClient = client

//     // Start handling signaling messages
//     go worker.handleSignalingMessages()
//     log.Printf("✅ Ingest worker %s ready and listening for WebRTC connections...", worker.workerID)

//     // Wait for shutdown signal
//     sigChan := make(chan os.Signal, 1)
//     signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
    
//     <-sigChan
//     log.Println("🛑 Shutting down ingest worker...")
//     worker.cleanup()
//     log.Println("✅ Ingest worker shutdown complete")
// }



















//DECODING in C++ SERVER
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
	"github.com/pion/rtp"
	"github.com/pion/rtp/codecs"
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
	FacesDetected int                    `json:"faces_detected"`
	BoundingBoxes []DetectionBoundingBox `json:"bounding_boxes,omitempty"`
	Timestamp     int64                  `json:"timestamp"`
	FrameType     string                 `json:"frame_type,omitempty"`
}

type SignalMessage struct {
	Type      string                 `json:"type"`
	To        string                 `json:"to,omitempty"`
	From      string                 `json:"from,omitempty"`
	SDP       string                 `json:"sdp,omitempty"`
	Candidate map[string]interface{} `json:"candidate,omitempty"`
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
	
	// VP8 stream tracking
	keyframeReceived bool
	streamActive     bool
	actualWidth      int
	actualHeight     int
	
	// Frame processing control
	frameMutex        sync.Mutex
	processingFrames  map[int]bool
	frameCounter      int
	lastProcessTime   time.Time
	framesPerSecond   int
	
	// FIXED: VP8 frame assembly with proper state management
	frameAssembler    *VP8FrameAssembler
}

// VP8FrameAssembler handles proper VP8 frame assembly from RTP packets
type VP8FrameAssembler struct {
	buffer           []byte
	currentTimestamp uint32
	lastSeqNum       uint16
	frameStarted     bool
	waitForKeyframe  bool
	packetCount      int
	frameCount       int
	lastLogTime      time.Time
}

func NewVP8FrameAssembler() *VP8FrameAssembler {
	return &VP8FrameAssembler{
		buffer:          make([]byte, 0, 100000),
		waitForKeyframe: true,
		lastLogTime:     time.Now(),
	}
}

// ProcessPacket handles incoming RTP packets and assembles complete frames
func (vfa *VP8FrameAssembler) ProcessPacket(rtpPacket *rtp.Packet) ([]byte, bool, error) {
	// Parse VP8 payload header
	vp8Packet := &codecs.VP8Packet{}
	payload, err := vp8Packet.Unmarshal(rtpPacket.Payload)
	if err != nil {
		return nil, false, err
	}

	if len(payload) == 0 {
		return nil, false, nil
	}

	vfa.packetCount++

	// Detect packet loss
	if vfa.lastSeqNum != 0 && rtpPacket.SequenceNumber != 0 {
		expectedSeq := vfa.lastSeqNum + 1
		if rtpPacket.SequenceNumber != expectedSeq {
			gap := int(rtpPacket.SequenceNumber - expectedSeq)
			if gap < 0 {
				gap += 65536 // Handle wraparound
			}
			if gap > 0 && gap < 100 {
				log.Printf("Packet loss: %d packets, resetting frame assembly", gap)
				vfa.Reset()
			}
		}
	}
	vfa.lastSeqNum = rtpPacket.SequenceNumber

	// Check for new frame
	isStartPacket := vp8Packet.S == 1
	isNewTimestamp := rtpPacket.Timestamp != vfa.currentTimestamp

	// Complete previous frame if starting new one
	if vfa.frameStarted && (isNewTimestamp || isStartPacket) {
		if len(vfa.buffer) > 0 {
			return vfa.CompleteFrame()
		}
		vfa.Reset()
	}

	// Start new frame
	if isStartPacket || (!vfa.frameStarted && isNewTimestamp) {
		vfa.currentTimestamp = rtpPacket.Timestamp
		vfa.frameStarted = true
		vfa.buffer = vfa.buffer[:0] // Reset buffer
	}

	// Append payload
	if vfa.frameStarted {
		vfa.buffer = append(vfa.buffer, payload...)
	}

	// Check for frame completion marker
	if rtpPacket.Marker && vfa.frameStarted && len(vfa.buffer) > 0 {
		return vfa.CompleteFrame()
	}

	// Safety check
	if len(vfa.buffer) > 500000 {
		log.Printf("Buffer overflow, resetting")
		vfa.Reset()
	}

	return nil, false, nil
}

func (vfa *VP8FrameAssembler) CompleteFrame() ([]byte, bool, error) {
	if len(vfa.buffer) == 0 {
		vfa.Reset()
		return nil, false, nil
	}

	// Validate minimum frame size
	if len(vfa.buffer) < 10 {
		vfa.Reset()
		return nil, false, nil
	}

	vfa.frameCount++
	
	// Check if keyframe
	isKeyframe := vfa.IsKeyframe(vfa.buffer)
	
	if isKeyframe {
		log.Printf("KEYFRAME #%d: %d bytes, %d packets", vfa.frameCount, len(vfa.buffer), vfa.packetCount)
		vfa.waitForKeyframe = false
	} else if vfa.waitForKeyframe {
		// Skip P-frames until we get a keyframe
		vfa.Reset()
		return nil, false, nil
	}

	// Make copy of frame data
	frame := make([]byte, len(vfa.buffer))
	copy(frame, vfa.buffer)
	
	// Periodic stats
	if time.Since(vfa.lastLogTime) > 10*time.Second {
		log.Printf("Stats: %d frames assembled", vfa.frameCount)
		vfa.lastLogTime = time.Now()
	}

	vfa.Reset()
	return frame, isKeyframe, nil
}

func (vfa *VP8FrameAssembler) IsKeyframe(data []byte) bool {
	if len(data) < 10 {
		return false
	}
	
	// VP8 bitstream format:
	// - First bit of first byte: 0=keyframe, 1=interframe
	frameTag := data[0]
	isKeyframe := (frameTag & 0x01) == 0
	
	if isKeyframe && len(data) >= 6 {
		// Keyframes have start code: 0x9d 0x01 0x2a
		hasStartCode := data[3] == 0x9d && data[4] == 0x01 && data[5] == 0x2a
		return hasStartCode
	}
	
	return isKeyframe
}

func (vfa *VP8FrameAssembler) Reset() {
	vfa.buffer = vfa.buffer[:0]
	vfa.frameStarted = false
	vfa.packetCount = 0
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
		},
	}

	pc, err := webrtc.NewPeerConnection(config)
	if err != nil {
		return err
	}
	w.peerConnection = pc

	// Reset state
	w.keyframeReceived = false
	w.streamActive = false
	w.actualWidth = 640
	w.actualHeight = 480
	w.processingFrames = make(map[int]bool)
	w.frameCounter = 0
	w.lastProcessTime = time.Now()
	w.framesPerSecond = 5
	w.frameAssembler = NewVP8FrameAssembler()

	// Handle incoming tracks
	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		log.Printf("Track received: %s, codec: %s", track.Kind().String(), track.Codec().MimeType)
		
		if track.Codec().MimeType == "video/VP8" {
			w.streamActive = true
			go w.processVP8Track(track)
		} else {
			log.Printf("Unsupported codec: %s", track.Codec().MimeType)
		}
	})

	// ICE candidates
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
			log.Printf("Error sending ICE candidate: %v", err)
		}
	})

	// Connection state
	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		log.Printf("Connection state: %s", state.String())
		
		switch state {
		case webrtc.PeerConnectionStateConnected:
			log.Printf("WebRTC connected with %s", w.currentClient)
		case webrtc.PeerConnectionStateFailed, webrtc.PeerConnectionStateDisconnected:
			w.streamActive = false
		}
	})

	// Data channel
	pc.OnDataChannel(func(dc *webrtc.DataChannel) {
		log.Printf("Data channel: %s", dc.Label())
		w.dataChannel = dc

		dc.OnOpen(func() {
			log.Printf("Data channel opened")
		})

		dc.OnClose(func() { 
			log.Printf("Data channel closed") 
		})
	})

	// Process queued candidates
	for _, msg := range w.queuedCandidates {
		w.handleCandidate(msg)
	}
	w.queuedCandidates = nil

	log.Printf("WebRTC initialized")
	return nil
}

// --- Handle offer ---
func (w *IngestWorker) handleOffer(msg SignalMessage) {
	log.Printf("Offer from %s", msg.From)
	w.currentClient = msg.From

	if w.peerConnection != nil {
		w.peerConnection.Close()
		w.peerConnection = nil
		w.dataChannel = nil
	}

	if err := w.initializeWebRTC(); err != nil {
		log.Printf("WebRTC init failed: %v", err)
		return
	}

	offer := webrtc.SessionDescription{
		Type: webrtc.SDPTypeOffer,
		SDP:  msg.SDP,
	}

	if err := w.peerConnection.SetRemoteDescription(offer); err != nil {
		log.Printf("Set remote description failed: %v", err)
		return
	}

	answer, err := w.peerConnection.CreateAnswer(nil)
	if err != nil {
		log.Printf("Create answer failed: %v", err)
		return
	}

	if err := w.peerConnection.SetLocalDescription(answer); err != nil {
		log.Printf("Set local description failed: %v", err)
		return
	}

	resp := SignalMessage{
		Type: "answer",
		To:   msg.From,
		From: w.workerID,
		SDP:  answer.SDP,
	}
	
	if err := w.sendWebSocketMessage(resp); err != nil {
		log.Printf("Send answer failed: %v", err)
	} else {
		log.Printf("Answer sent to %s", msg.From)
	}
}

// --- Handle ICE candidate ---
func (w *IngestWorker) handleCandidate(msg SignalMessage) {
	if w.peerConnection == nil {
		w.queuedCandidates = append(w.queuedCandidates, msg)
		return
	}

	if msg.Candidate == nil {
		return
	}

	ice := webrtc.ICECandidateInit{}
	
	if candidateStr, ok := msg.Candidate["candidate"].(string); ok && candidateStr != "" {
		ice.Candidate = candidateStr
	} else {
		return
	}
	
	if sdpMid, ok := msg.Candidate["sdpMid"].(string); ok && sdpMid != "" {
		ice.SDPMid = &sdpMid
	}
	
	if sdpMLineIndex, ok := msg.Candidate["sdpMLineIndex"].(float64); ok {
		idx := uint16(sdpMLineIndex)
		ice.SDPMLineIndex = &idx
	}
	
	if err := w.peerConnection.AddICECandidate(ice); err != nil {
		log.Printf("Add ICE candidate failed: %v", err)
	}
}

// --- Handle signaling ---
func (w *IngestWorker) handleSignalingMessages() {
	log.Printf("Listening for signaling messages...")
	
	for {
		var msg SignalMessage
		if err := w.wsConn.ReadJSON(&msg); err != nil {
			log.Printf("WebSocket read error: %v", err)
			time.Sleep(5 * time.Second)
			continue
		}

		if msg.To != "" && msg.To != w.workerID {
			continue
		}

		switch msg.Type {
		case "offer":
			w.handleOffer(msg)
		case "candidate":
			w.handleCandidate(msg)
		}
	}
}

// FIXED: Process VP8 track with proper frame assembly
func (w *IngestWorker) processVP8Track(track *webrtc.TrackRemote) {
	log.Printf("Starting VP8 processing (target: %d FPS)", w.framesPerSecond)

	frameSkipCounter := 0
	framesToSkip := 30 / w.framesPerSecond

	for w.streamActive {
		rtpPacket, _, err := track.ReadRTP()
		if err != nil {
			log.Printf("RTP read error: %v", err)
			return
		}

		// Process packet and get complete frame if available
		frameData, isKeyframe, err := w.frameAssembler.ProcessPacket(rtpPacket)
		if err != nil {
			continue
		}

		// No complete frame yet
		if frameData == nil {
			continue
		}

		// Frame rate limiting
		frameSkipCounter++
		if frameSkipCounter < framesToSkip {
			continue
		}
		frameSkipCounter = 0

		// Process frame asynchronously
		frameCopy := make([]byte, len(frameData))
		copy(frameCopy, frameData)
		
		frameType := "P-frame"
		if isKeyframe {
			frameType = "Keyframe"
			w.keyframeReceived = true
		}
		
		go w.processEncodedFrame(frameCopy, w.frameAssembler.frameCount, frameType)
	}
}

// Process encoded frame
func (w *IngestWorker) processEncodedFrame(frameData []byte, frameNum int, frameType string) {
	if w.grpcClient == nil || len(frameData) == 0 {
		return
	}

	// Rate limiting
	now := time.Now()
	if now.Sub(w.lastProcessTime) < time.Second/time.Duration(w.framesPerSecond) {
		return
	}

	// Backpressure control
	w.frameMutex.Lock()
	if len(w.processingFrames) > 2 {
		w.frameMutex.Unlock()
		return
	}
	w.frameCounter++
	currentFrameID := w.frameCounter
	w.processingFrames[currentFrameID] = true
	w.lastProcessTime = now
	w.frameMutex.Unlock()

	defer func() {
		w.frameMutex.Lock()
		delete(w.processingFrames, currentFrameID)
		w.frameMutex.Unlock()
	}()

	log.Printf("Processing %s #%d (%d bytes)", frameType, frameNum, len(frameData))

	// Detect faces
	faces, processingTime, err := w.grpcClient.DetectFacesFromEncodedFrame(
		frameData,
		w.actualWidth,
		w.actualHeight,
		3,
	)
	
	if err != nil {
		log.Printf("Detection error: %v", err)
		return
	}

	// Prepare results
	var boundingBoxes []DetectionBoundingBox
	for _, f := range faces {
		boundingBoxes = append(boundingBoxes, DetectionBoundingBox{
			X:          int(f.GetX()),
			Y:          int(f.GetY()),
			Width:      int(f.GetWidth()),
			Height:     int(f.GetHeight()),
			Confidence: f.GetConfidence(),
		})
	}

	result := DetectionResult{
		FacesDetected: len(boundingBoxes),
		BoundingBoxes: boundingBoxes,
		Timestamp:     now.UnixMilli(),
		FrameType:     frameType,
	}

	// Send results
	if w.dataChannel != nil && w.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
		data, err := json.Marshal(result)
		if err != nil {
			log.Printf("JSON marshal error: %v", err)
			return
		}

		if err := w.dataChannel.Send(data); err != nil {
			log.Printf("Send error: %v", err)
		} else {
			if result.FacesDetected > 0 {
				log.Printf("SUCCESS: Frame %d (%s): %d faces, %dms", 
					frameNum, frameType, result.FacesDetected, processingTime)
			}
		}
	}
}

// --- Cleanup ---
func (w *IngestWorker) cleanup() {
	log.Printf("Cleaning up...")
	
	w.streamActive = false
	
	if w.peerConnection != nil {
		w.peerConnection.Close()
		w.peerConnection = nil
	}
	
	if w.dataChannel != nil {
		w.dataChannel.Close()
		w.dataChannel = nil
	}

	if w.grpcClient != nil {
		w.grpcClient.Close()
		w.grpcClient = nil
	}
}

// --- Main ---
func main() {
	signalingURL := os.Getenv("SIGNALING_URL")
	if signalingURL == "" {
		signalingURL = "ws://localhost:8080"
	}

	workerID := os.Getenv("WORKER_ID")
	if workerID == "" {
		workerID = "ingest_worker_1"
	}

	grpcEndpoint := os.Getenv("GRPC_ENDPOINT")
	if grpcEndpoint == "" {
		grpcEndpoint = "localhost:50051"
	}

	log.Printf("Starting worker: %s", workerID)
	log.Printf("Signaling: %s", signalingURL)
	log.Printf("gRPC: %s", grpcEndpoint)

	conn, _, err := websocket.DefaultDialer.Dial(signalingURL+"?client_id="+workerID, nil)
	if err != nil {
		log.Fatal("Signaling connection failed:", err)
	}
	defer conn.Close()

	worker := &IngestWorker{
		signalingURL: signalingURL,
		workerID:     workerID,
		wsConn:       conn,
		actualWidth:  640,
		actualHeight: 480,
	}

	log.Printf("Connecting to gRPC...")
	client, err := NewInferenceClient(grpcEndpoint)
	if err != nil {
		log.Fatal("gRPC connection failed:", err)
	}
	defer client.Close()
	worker.grpcClient = client

	if err := client.Ping(); err != nil {
		log.Printf("gRPC ping failed: %v", err)
	} else {
		log.Printf("gRPC connected")
	}

	go worker.handleSignalingMessages()
	log.Printf("Worker %s ready", worker.workerID)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	
	<-sigChan
	log.Println("Shutting down...")
	worker.cleanup()
	log.Println("Shutdown complete")
}