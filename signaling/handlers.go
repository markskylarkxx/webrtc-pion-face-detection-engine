package main

import (
    "log"
    "math/rand"
    "net/http"
    "sync"
    "time"

    "github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
    CheckOrigin: func(r *http.Request) bool {
        return true // Allow all origins for development
    },
}

type SignalingServer struct {
    mu          sync.RWMutex
    connections map[string]*websocket.Conn
}

type SignalMessage struct {
    Type      string      `json:"type"`
    To        string      `json:"to,omitempty"`
    From      string      `json:"from,omitempty"`
    SDP       interface{} `json:"sdp,omitempty"`
    Candidate interface{} `json:"candidate,omitempty"`
}

func NewSignalingServer() *SignalingServer {
    return &SignalingServer{
        connections: make(map[string]*websocket.Conn),
    }
}

func randomString(n int) string {
    var letters = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
    rand.Seed(time.Now().UnixNano())
    b := make([]rune, n)
    for i := range b {
        b[i] = letters[rand.Intn(len(letters))]
    }
    return string(b)
}

func (s *SignalingServer) ServeHTTP(w http.ResponseWriter, r *http.Request) {
    conn, err := upgrader.Upgrade(w, r, nil)
    if err != nil {
        log.Printf("WebSocket upgrade failed: %v", err)
        return
    }
    defer conn.Close()

    clientID := r.URL.Query().Get("client_id")
    if clientID == "" {
        clientID = "client_" + randomString(10)
    }

    s.mu.Lock()
    s.connections[clientID] = conn
    s.mu.Unlock()

    log.Printf("Client connected: %s", clientID)

    // Send welcome message with client ID
    welcomeMsg := SignalMessage{
        Type: "welcome",
        From: clientID,
    }
    conn.WriteJSON(welcomeMsg)

    for {
        var msg SignalMessage
        err := conn.ReadJSON(&msg)
        if err != nil {
            log.Printf("Read error: %v", err)
            break
        }

        log.Printf("Received message from %s to %s: %s", clientID, msg.To, msg.Type)

        // Route message to target client
        if msg.To != "" {
            s.mu.RLock()
            targetConn, exists := s.connections[msg.To]
            s.mu.RUnlock()

            if exists {
                msg.From = clientID
                targetConn.WriteJSON(msg)
            }
        }
    }

    s.mu.Lock()
    delete(s.connections, clientID)
    s.mu.Unlock()
}