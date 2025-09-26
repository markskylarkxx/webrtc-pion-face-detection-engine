package main

import (
    "log"

    "github.com/pion/webrtc/v3"
)

type MediaRouter struct {
    peerConnection *webrtc.PeerConnection
    inferenceChan  chan []byte
    dataChannel    *webrtc.DataChannel
}

func NewMediaRouter() (*MediaRouter, error) {
    config := webrtc.Configuration{
        ICEServers: []webrtc.ICEServer{
            {URLs: []string{"stun:stun.l.google.com:19302"}},
        },
    }

    peerConnection, err := webrtc.NewPeerConnection(config)
    if err != nil {
        return nil, err
    }

    router := &MediaRouter{
        peerConnection: peerConnection,
        inferenceChan:  make(chan []byte, 100),
    }

    // Set up data channel for sending results back to client
    router.setupDataChannel()
    
    // Handle incoming video tracks
    router.setupVideoHandler()

    return router, nil
}

func (m *MediaRouter) setupDataChannel() {
    dataChannel, err := m.peerConnection.CreateDataChannel("results", nil)
    if err != nil {
        log.Printf("Failed to create data channel: %v", err)
        return
    }

    m.dataChannel = dataChannel

    dataChannel.OnOpen(func() {
        log.Println("Data channel opened")
    })

    dataChannel.OnMessage(func(msg webrtc.DataChannelMessage) {
        log.Printf("Received message on data channel: %s", string(msg.Data))
    })
}

func (m *MediaRouter) setupVideoHandler() {
    m.peerConnection.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
        log.Printf("Track received: %s, codec: %s", track.Kind().String(), track.Codec().MimeType)

        if track.Kind() == webrtc.RTPCodecTypeVideo {
            go m.handleVideoTrack(track)
        }
    })
}

func (m *MediaRouter) handleVideoTrack(track *webrtc.TrackRemote) {
    for {
        _, _, err := track.ReadRTP() // Remove unused rtpPacket variable
        if err != nil {
            log.Printf("Error reading RTP packet: %v", err)
            return
        }

        // For now, just log that we're receiving packets
        // In production, you'd decode this and send to inference engine
        if len(m.inferenceChan) < cap(m.inferenceChan) {
            // Send frame data to inference engine (stub)
            frameData := []byte("frame_data") // This would be actual decoded frame
            m.inferenceChan <- frameData
        }
    }
}

func (m *MediaRouter) SendResult(result InferenceResult) {
    if m.dataChannel != nil && m.dataChannel.ReadyState() == webrtc.DataChannelStateOpen {
        // For now, just log - we'll implement JSON marshaling when needed
        log.Printf("Would send result: %+v", result)
    }
}

func (m *MediaRouter) CreateOffer() (*webrtc.SessionDescription, error) {
    offer, err := m.peerConnection.CreateOffer(nil)
    if err != nil {
        return nil, err
    }

    err = m.peerConnection.SetLocalDescription(offer)
    if err != nil {
        return nil, err
    }

    return &offer, nil
}

func (m *MediaRouter) SetRemoteAnswer(answer webrtc.SessionDescription) error {
    return m.peerConnection.SetRemoteDescription(answer)
}