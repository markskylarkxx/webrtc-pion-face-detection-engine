package main

import (
    "log"
    "net/http"
    "os"
)

func main() {
    port := os.Getenv("PORT")
    if port == "" {
        port = "8080"
    }

    server := NewSignalingServer()
    
    log.Printf("Signaling server starting on :%s", port)
    log.Fatal(http.ListenAndServe(":"+port, server))
}