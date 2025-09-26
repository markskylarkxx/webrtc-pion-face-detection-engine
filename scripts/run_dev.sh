#!/bin/bash

# Start signaling server
echo "Starting signaling server..."
./bin/signaling-server &

# Wait a moment for signaling server to start
sleep 2

# Start ingest worker
echo "Starting ingest worker..."
./bin/ingest-worker &

echo "Development environment started!"
echo "Open client/web/index.html in a web browser"
echo "Press Ctrl+C to stop all services"

# Wait for Ctrl+C
trap 'kill $(jobs -p); exit' INT
wait