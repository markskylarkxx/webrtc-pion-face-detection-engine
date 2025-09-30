// class WebRTCClient {
//     constructor() {
//         this.localStream = null;
//         this.peerConnection = null;
//         this.signalingSocket = null;
//         this.clientId = null;
//         this.workerId = 'ingest_worker_1';
//         this.dataChannel = null;
//         this.isConnecting = false;
//         this.iceCandidateQueue = [];
//     }

//     async initialize() {
//         this.clientId = 'client_' + Math.random().toString(36).substr(2, 9);
//         await this.connectSignaling();
//         this.setupUI();
//     }

//     async connectSignaling() {
//         return new Promise((resolve, reject) => {
//             const signalingUrl = 'ws://localhost:8080';
//             this.signalingSocket = new WebSocket(`${signalingUrl}/?client_id=${this.clientId}`);

//             this.signalingSocket.onmessage = (event) => {
//                 const message = JSON.parse(event.data);
//                 this.handleSignalingMessage(message);
//             };

//             this.signalingSocket.onopen = () => {
//                 console.log('✅ Connected to signaling server with ID:', this.clientId);
//                 document.getElementById('connectBtn').disabled = false;
//                 resolve();
//             };

//             this.signalingSocket.onerror = (error) => {
//                 console.error('❌ WebSocket error:', error);
//                 reject(error);
//             };
//         });
//     }

//     async startCamera() {
//         try {
//             this.localStream = await navigator.mediaDevices.getUserMedia({
//                 video: { width: 640, height: 480, frameRate: 30 },
//                 audio: false
//             });
//             document.getElementById('localVideo').srcObject = this.localStream;
//             await new Promise(resolve => {
//                 document.getElementById('localVideo').onloadedmetadata = () => {
//                     document.getElementById('localVideo').play();
//                     resolve();
//                 };
//             });
//             console.log('🎥 Camera started');
//             document.getElementById('startBtn').disabled = true;
//             document.getElementById('connectBtn').disabled = false;
//         } catch (err) {
//             console.error('❌ Camera error:', err);
//         }
//     }

//     async connectToServer() {
//         if (!this.localStream) return alert('Start camera first');
//         if (this.isConnecting) return;

//         this.isConnecting = true;
//         document.getElementById('connectBtn').disabled = true;

//         try {
//             console.log('🔗 Starting WebRTC connection...');

//             // TURN + STUN
//             this.peerConnection = new RTCPeerConnection({
//                 iceServers: [
//                     { urls: 'stun:stun.l.google.com:19302' },
//                     { urls: 'turn:TURN_SERVER_IP:3478', username: 'user', credential: 'pass' }
//                 ],
//                 iceCandidatePoolSize: 10
//             });

//             // Add tracks
//             this.localStream.getTracks().forEach(track => this.peerConnection.addTrack(track, this.localStream));

//             // Create data channel
//             this.createDataChannel();

//             // Queue handling for remote ICE candidates
//             this.peerConnection.onicecandidate = (event) => {
//                 if (event.candidate) {
//                     this.sendSignalingMessage({ type: 'candidate', to: this.workerId, candidate: event.candidate });
//                 } else {
//                     console.log('🧊 All ICE candidates sent');
//                 }
//             };

//             // Connection states
//             this.peerConnection.onconnectionstatechange = () => console.log('🔗 Connection state:', this.peerConnection.connectionState);
//             this.peerConnection.oniceconnectionstatechange = () => console.log('🧊 ICE state:', this.peerConnection.iceConnectionState);

//             // Create offer
//             const offer = await this.peerConnection.createOffer({ offerToReceiveVideo: false });
//             await this.peerConnection.setLocalDescription(offer);

//             // Send offer.sdp (string only!)
//             this.sendSignalingMessage({ type: 'offer', to: this.workerId, sdp: offer.sdp });

//             // Timeout
//             setTimeout(() => {
//                 if (this.peerConnection && this.peerConnection.connectionState !== 'connected') {
//                     this.handleConnectionError('Connection timeout');
//                 }
//             }, 60000);

//         } catch (err) {
//             console.error('❌ Connect error:', err);
//             this.handleConnectionError(err.message);
//         }
//     }

//     createDataChannel() {
//         this.dataChannel = this.peerConnection.createDataChannel('face-results', { ordered: true });
//         this.dataChannel.onopen = () => { console.log('✅ Data channel open'); this.isConnecting = false; };
//         this.dataChannel.onmessage = (event) => console.log('Face data:', event.data);
//         this.dataChannel.onclose = () => console.log('Data channel closed');
//         this.dataChannel.onerror = (err) => console.error('Data channel error:', err);
//     }

//     handleSignalingMessage(msg) {
//         switch (msg.type) {
//             case 'answer':
//                 if (this.peerConnection) {
//                     const desc = { type: 'answer', sdp: msg.sdp };
//                     this.peerConnection.setRemoteDescription(desc).then(() => {
//                         console.log('✅ Remote description set');
//                         // Add any queued ICE candidates
//                         this.iceCandidateQueue.forEach(c => this.peerConnection.addIceCandidate(c));
//                         this.iceCandidateQueue = [];
//                     });
//                 }
//                 break;
//             case 'candidate':
//                 if (this.peerConnection && this.peerConnection.remoteDescription) {
//                     this.peerConnection.addIceCandidate(msg.candidate);
//                 } else {
//                     // Queue if peerConnection not ready
//                     this.iceCandidateQueue.push(msg.candidate);
//                 }
//                 break;
//         }
//     }

//     sendSignalingMessage(msg) {
//         msg.from = this.clientId;
//         if (this.signalingSocket?.readyState === WebSocket.OPEN) {
//             this.signalingSocket.send(JSON.stringify(msg));
//         }
//     }

//     handleConnectionError(msg) {
//         console.error('❌ Connection error:', msg);
//         this.isConnecting = false;
//         if (this.peerConnection) this.peerConnection.close();
//         this.peerConnection = null;
//         document.getElementById('connectBtn').disabled = false;
//     }

//     setupUI() {
//         document.getElementById('startBtn').onclick = () => this.startCamera();
//         document.getElementById('connectBtn').onclick = () => this.connectToServer();
//     }
// }

// window.addEventListener('load', () => {
//     const client = new WebRTCClient();
//     client.initialize().catch(console.error);
// });



// decoing in c++
class WebRTCClient {
    constructor() {
        this.localStream = null;
        this.peerConnection = null;
        this.signalingSocket = null;
        this.clientId = null;
        this.workerId = 'ingest_worker_1';
        this.dataChannel = null;
        this.isConnecting = false;
        this.iceCandidateQueue = [];
        
        // Canvas for drawing bounding boxes
        this.canvas = null;
        this.ctx = null;
        this.faceBoxes = [];
        this.videoWidth = 640;
        this.videoHeight = 480;
    }

    async initialize() {
        this.clientId = 'client_' + Math.random().toString(36).substr(2, 9);
        console.log('🚀 Initializing WebRTC client with ID:', this.clientId);
        await this.connectSignaling();
        this.setupUI();
        this.setupCanvas();
    }

    setupCanvas() {
        console.log('🎨 Setting up canvas for bounding boxes...');
        
        // Create canvas overlay for bounding boxes
        this.canvas = document.createElement('canvas');
        this.ctx = this.canvas.getContext('2d');
        
        // Get the LOCAL video element
        const localVideo = document.getElementById('localVideo');
        const videoContainer = document.getElementById('videoContainer');
        
        // Style the canvas to match video
        this.canvas.style.position = 'absolute';
        this.canvas.style.top = '0';
        this.canvas.style.left = '0';
        this.canvas.style.pointerEvents = 'none';
        this.canvas.style.zIndex = '10';
        
        // Add canvas to video container
        videoContainer.appendChild(this.canvas);
        
        console.log('✅ Canvas created and positioned over local video');
        
        // Set up resize handling
        localVideo.addEventListener('loadedmetadata', () => {
            console.log('📹 Video metadata loaded, resizing canvas...');
            this.resizeCanvas();
        });
        
        // Initial resize attempt
        setTimeout(() => this.resizeCanvas(), 100);
    }

    resizeCanvas() {
        const video = document.getElementById('localVideo');
        const container = document.getElementById('videoContainer');
        
        if (video.videoWidth && video.videoHeight) {
            this.videoWidth = video.videoWidth;
            this.videoHeight = video.videoHeight;
            
            // Set canvas to match video display size
            this.canvas.width = container.clientWidth;
            this.canvas.height = container.clientHeight;
            
            console.log(`✅ Canvas resized: ${this.canvas.width}x${this.canvas.height}`);
            console.log(`📏 Video source: ${this.videoWidth}x${this.videoHeight}`);
            
        } else {
            console.log('⏳ Video dimensions not available yet, retrying...');
            setTimeout(() => this.resizeCanvas(), 500);
        }
    }

    async connectSignaling() {
        return new Promise((resolve, reject) => {
            const signalingUrl = 'ws://localhost:8080';
            console.log('🔗 Connecting to signaling server:', signalingUrl);
            
            this.signalingSocket = new WebSocket(`${signalingUrl}/?client_id=${this.clientId}`);

            this.signalingSocket.onmessage = (event) => {
                const message = JSON.parse(event.data);
                console.log('📨 Received signaling message:', message.type);
                this.handleSignalingMessage(message);
            };

            this.signalingSocket.onopen = () => {
                console.log('✅ Connected to signaling server with ID:', this.clientId);
                document.getElementById('connectBtn').disabled = false;
                resolve();
            };

            this.signalingSocket.onerror = (error) => {
                console.error('❌ WebSocket error:', error);
                reject(error);
            };
        });
    }

    async startCamera() {
        try {
            console.log('📷 Starting camera...');
            this.localStream = await navigator.mediaDevices.getUserMedia({
                video: { width: 640, height: 480, frameRate: 15 }, // Reduced framerate for performance
                audio: false
            });
            
            const videoElement = document.getElementById('localVideo');
            videoElement.srcObject = this.localStream;
            
            await new Promise(resolve => {
                videoElement.onloadedmetadata = () => {
                    videoElement.play();
                    this.videoWidth = videoElement.videoWidth;
                    this.videoHeight = videoElement.videoHeight;
                    console.log('🎥 Camera started. Video dimensions:', 
                        this.videoWidth, 'x', this.videoHeight);
                    this.resizeCanvas();
                    resolve();
                };
            });
            
            console.log('✅ Camera started successfully');
            document.getElementById('startBtn').disabled = true;
            document.getElementById('connectBtn').disabled = false;
            
        } catch (err) {
            console.error('❌ Camera error:', err);
            alert('Cannot access camera. Please check permissions.');
        }
    }

    async connectToServer() {
        if (!this.localStream) return alert('Start camera first');
        if (this.isConnecting) return;

        this.isConnecting = true;
        document.getElementById('connectBtn').disabled = true;
        console.log('🔗 Starting WebRTC connection to server...');

        try {
            this.peerConnection = new RTCPeerConnection({
                iceServers: [
                    { urls: 'stun:stun.l.google.com:19302' }
                ],
                iceCandidatePoolSize: 10
            });

            // Add tracks
            this.localStream.getTracks().forEach(track => {
                this.peerConnection.addTrack(track, this.localStream);
                console.log('✅ Added video track to peer connection');
            });

            // Create data channel
            this.createDataChannel();

            // ICE candidates
            this.peerConnection.onicecandidate = (event) => {
                if (event.candidate) {
                    this.sendSignalingMessage({ type: 'candidate', to: this.workerId, candidate: event.candidate });
                }
            };

            // Connection states
            this.peerConnection.onconnectionstatechange = () => {
                const state = this.peerConnection.connectionState;
                console.log('🔗 Connection state:', state);
                
                if (state === 'connected') {
                    console.log('🎉 WebRTC connection established!');
                } else if (state === 'failed') {
                    this.handleConnectionError('Connection failed');
                }
            };

            this.peerConnection.oniceconnectionstatechange = () => {
                console.log('🧊 ICE state:', this.peerConnection.iceConnectionState);
            };

            // Create offer
            const offer = await this.peerConnection.createOffer({ 
                offerToReceiveVideo: false,
                offerToReceiveAudio: false
            });
            await this.peerConnection.setLocalDescription(offer);
            this.sendSignalingMessage({ type: 'offer', to: this.workerId, sdp: offer.sdp });

            console.log('✅ WebRTC offer created and sent');

        } catch (err) {
            console.error('❌ Connect error:', err);
            this.handleConnectionError(err.message);
        }
    }

    createDataChannel() {
        console.log('📡 Creating data channel for face results...');
        
        this.dataChannel = this.peerConnection.createDataChannel('face-results', { 
            ordered: true,
            maxRetransmits: 3
        });
        
        this.dataChannel.onopen = () => { 
            console.log('✅✅✅ DATA CHANNEL OPEN - Ready to receive face detection!'); 
            this.isConnecting = false;
            document.getElementById('connectBtn').disabled = true;
            document.getElementById('stopBtn').disabled = false;
            
            // Clear any test content
            this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        };
        
        this.dataChannel.onmessage = (event) => {
            try {
                const faceData = JSON.parse(event.data);
                console.log(`🎯 Face detection: ${faceData.faces_detected} faces`);
                
                if (faceData.bounding_boxes && faceData.bounding_boxes.length > 0) {
                    console.log(`📦 Found ${faceData.bounding_boxes.length} bounding boxes`);
                }
                
                // Update results display
                this.updateResultsDisplay(faceData);
                
                // Store and draw bounding boxes
                this.faceBoxes = faceData.bounding_boxes || [];
                this.drawBoundingBoxes();
                
            } catch (error) {
                console.error('❌ Error parsing face data:', error);
            }
        };
        
        this.dataChannel.onclose = () => {
            console.log('Data channel closed');
            this.clearBoundingBoxes();
        };
        
        this.dataChannel.onerror = (err) => {
            console.error('Data channel error:', err);
        };
    }

    updateResultsDisplay(faceData) {
        const resultText = document.getElementById('resultText');
        let html = '';
        
        if (faceData.faces_detected > 0) {
            html = `<div style="color: #00ff00; font-weight: bold;">
                ✅ Detected ${faceData.faces_detected} face(s)
            </div>`;
            
            if (faceData.bounding_boxes && faceData.bounding_boxes.length > 0) {
                faceData.bounding_boxes.forEach((bbox, index) => {
                    html += `<div style="font-size: 12px; margin-left: 10px;">
                        Face ${index + 1}: (${bbox.x}, ${bbox.y}) ${bbox.width}x${bbox.height} - ${(bbox.confidence * 100).toFixed(1)}%
                    </div>`;
                });
            }
        } else {
            html = `<div style="color: #ff4444; font-weight: bold;">
                ❌ No faces detected
            </div>`;
        }
        
        resultText.innerHTML = html;
    }

    drawBoundingBoxes() {
        if (this.faceBoxes.length === 0) {
            this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
            return;
        }
        
        // Clear canvas
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Calculate scaling factors - FIXED COORDINATE SYSTEM
        const scaleX = this.canvas.width / this.videoWidth;
        const scaleY = this.canvas.height / this.videoHeight;
        
        console.log(`📏 Scaling - Canvas: ${this.canvas.width}x${this.canvas.height}, Video: ${this.videoWidth}x${this.videoHeight}, Scale: ${scaleX.toFixed(2)}x${scaleY.toFixed(2)}`);
        
        // Draw each bounding box
        this.faceBoxes.forEach((bbox, index) => {
            // Scale coordinates to canvas size
            const x = bbox.x * scaleX;
            const y = bbox.y * scaleY;
            const width = bbox.width * scaleX;
            const height = bbox.height * scaleY;
            
            console.log(`📦 Face ${index + 1}: Original(${bbox.x},${bbox.y},${bbox.width},${bbox.height}) -> Scaled(${x.toFixed(0)},${y.toFixed(0)},${width.toFixed(0)},${height.toFixed(0)})`);
            
            // Draw bounding box
            this.ctx.strokeStyle = '#00FF00';
            this.ctx.lineWidth = 3;
            this.ctx.strokeRect(x, y, width, height);
            
            // Draw label background
            this.ctx.fillStyle = 'rgba(0, 0, 0, 0.7)';
            this.ctx.fillRect(x - 2, y - 25, 80, 20);
            
            // Draw confidence text
            this.ctx.fillStyle = '#00FF00';
            this.ctx.font = 'bold 14px Arial';
            this.ctx.fillText(
                `${(bbox.confidence * 100).toFixed(1)}%`, 
                x, 
                y - 10
            );
            
            // Draw face number below box
            this.ctx.fillText(
                `Face ${index + 1}`,
                x + width / 2 - 25,
                y + height + 20
            );
        });
        
        console.log('🎉 Finished drawing bounding boxes');
    }

    clearBoundingBoxes() {
        this.faceBoxes = [];
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        document.getElementById('resultText').innerHTML = 'No results yet';
    }

    handleSignalingMessage(msg) {
        console.log('📨 Handling signaling message:', msg.type);
        
        switch (msg.type) {
            case 'answer':
                if (this.peerConnection) {
                    const desc = { type: 'answer', sdp: msg.sdp };
                    this.peerConnection.setRemoteDescription(desc).then(() => {
                        console.log('✅ Remote description set');
                        this.iceCandidateQueue.forEach(c => this.peerConnection.addIceCandidate(c));
                        this.iceCandidateQueue = [];
                    }).catch(error => {
                        console.error('❌ Error setting remote description:', error);
                    });
                }
                break;
                
            case 'candidate':
                if (this.peerConnection && this.peerConnection.remoteDescription) {
                    this.peerConnection.addIceCandidate(msg.candidate).catch(error => {
                        console.error('❌ Error adding ICE candidate:', error);
                    });
                } else {
                    this.iceCandidateQueue.push(msg.candidate);
                    console.log('🧊 Queued ICE candidate');
                }
                break;
        }
    }

    sendSignalingMessage(msg) {
        msg.from = this.clientId;
        if (this.signalingSocket?.readyState === WebSocket.OPEN) {
            this.signalingSocket.send(JSON.stringify(msg));
        } else {
            console.error('❌ WebSocket not connected');
        }
    }

    handleConnectionError(msg) {
        console.error('❌ Connection error:', msg);
        this.isConnecting = false;
        if (this.peerConnection) this.peerConnection.close();
        this.clearBoundingBoxes();
        document.getElementById('connectBtn').disabled = false;
        document.getElementById('stopBtn').disabled = true;
    }

    setupUI() {
        document.getElementById('startBtn').onclick = () => this.startCamera();
        document.getElementById('connectBtn').onclick = () => this.connectToServer();
        document.getElementById('stopBtn').onclick = () => {
            console.log('🛑 Stopping connection...');
            if (this.peerConnection) this.peerConnection.close();
            if (this.localStream) {
                this.localStream.getTracks().forEach(track => track.stop());
            }
            this.clearBoundingBoxes();
            document.getElementById('connectBtn').disabled = false;
            document.getElementById('stopBtn').disabled = true;
            document.getElementById('startBtn').disabled = false;
        };
    }
}

window.addEventListener('load', () => {
    console.log('🚀 WebRTC Face Detection starting...');
    const client = new WebRTCClient();
    client.initialize().catch(console.error);
});