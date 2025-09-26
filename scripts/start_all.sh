#!/bin/bash     

# start all: chmod +x scripts/start_all.sh
#      ./scripts/start_all.sh

echo "🚀 Starting WebRTC Face Detection System..."

# Kill any existing processes
pkill -f signaling-server || true
pkill -f ingest-worker || true

echo "📡 Starting signaling server..."
./bin/signaling-server &
SIGNAL_PID=$!
sleep 3

echo "🔄 Starting ingest worker..."
export SIGNALING_URL="ws://localhost:8080"
export WORKER_ID="ingest_worker_1"
./bin/ingest-worker &
INGEST_PID=$!
sleep 2

echo "✅ Services started!"
echo "   Signaling server PID: $SIGNAL_PID"
echo "   Ingest worker PID: $INGEST_PID"
echo ""
echo "🌐 Now open a NEW terminal and run:"
echo "   cd webrtc-pion-face-engine"
echo "   python3 -m http.server 8000"
echo ""
echo "📋 Then open: http://localhost:8000/client/web/index.html"
echo ""
echo "🎯 Test sequence:"
echo "   1. Click 'Start Camera'"
echo "   2. Click 'Connect to Server'"
echo "   3. Watch terminals for connection messages"
echo ""
echo "🛑 Press Ctrl+C to stop services"

cleanup() {
    echo "🔄 Shutting down services..."
    kill $SIGNAL_PID 2>/dev/null || true
    kill $INGEST_PID 2>/dev/null || true
    echo "✅ Services stopped."
    exit 0
}

trap cleanup INT TERM

# Wait indefinitely
while true; do
    sleep 10
    echo "⏰ Services still running... Press Ctrl+C to stop"
done