


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
    }

    async initialize() {
        this.clientId = 'client_' + Math.random().toString(36).substr(2, 9);
        await this.connectSignaling();
        this.setupUI();
    }

    async connectSignaling() {
        return new Promise((resolve, reject) => {
            const signalingUrl = 'ws://localhost:8080';
            this.signalingSocket = new WebSocket(`${signalingUrl}/?client_id=${this.clientId}`);

            this.signalingSocket.onmessage = (event) => {
                const message = JSON.parse(event.data);
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
            this.localStream = await navigator.mediaDevices.getUserMedia({
                video: { width: 640, height: 480, frameRate: 30 },
                audio: false
            });
            document.getElementById('localVideo').srcObject = this.localStream;
            await new Promise(resolve => {
                document.getElementById('localVideo').onloadedmetadata = () => {
                    document.getElementById('localVideo').play();
                    resolve();
                };
            });
            console.log('🎥 Camera started');
            document.getElementById('startBtn').disabled = true;
            document.getElementById('connectBtn').disabled = false;
        } catch (err) {
            console.error('❌ Camera error:', err);
        }
    }

    async connectToServer() {
        if (!this.localStream) return alert('Start camera first');
        if (this.isConnecting) return;

        this.isConnecting = true;
        document.getElementById('connectBtn').disabled = true;

        try {
            console.log('🔗 Starting WebRTC connection...');

            // TURN + STUN
            this.peerConnection = new RTCPeerConnection({
                iceServers: [
                    { urls: 'stun:stun.l.google.com:19302' },
                    { urls: 'turn:TURN_SERVER_IP:3478', username: 'user', credential: 'pass' }
                ],
                iceCandidatePoolSize: 10
            });

            // Add tracks
            this.localStream.getTracks().forEach(track => this.peerConnection.addTrack(track, this.localStream));

            // Create data channel
            this.createDataChannel();

            // Queue handling for remote ICE candidates
            this.peerConnection.onicecandidate = (event) => {
                if (event.candidate) {
                    this.sendSignalingMessage({ type: 'candidate', to: this.workerId, candidate: event.candidate });
                } else {
                    console.log('🧊 All ICE candidates sent');
                }
            };

            // Connection states
            this.peerConnection.onconnectionstatechange = () => console.log('🔗 Connection state:', this.peerConnection.connectionState);
            this.peerConnection.oniceconnectionstatechange = () => console.log('🧊 ICE state:', this.peerConnection.iceConnectionState);

            // Create offer
            const offer = await this.peerConnection.createOffer({ offerToReceiveVideo: false });
            await this.peerConnection.setLocalDescription(offer);

            // Send offer.sdp (string only!)
            this.sendSignalingMessage({ type: 'offer', to: this.workerId, sdp: offer.sdp });

            // Timeout
            setTimeout(() => {
                if (this.peerConnection && this.peerConnection.connectionState !== 'connected') {
                    this.handleConnectionError('Connection timeout');
                }
            }, 60000);

        } catch (err) {
            console.error('❌ Connect error:', err);
            this.handleConnectionError(err.message);
        }
    }

    createDataChannel() {
        this.dataChannel = this.peerConnection.createDataChannel('face-results', { ordered: true });
        this.dataChannel.onopen = () => { console.log('✅ Data channel open'); this.isConnecting = false; };
        this.dataChannel.onmessage = (event) => console.log('Face data:', event.data);
        this.dataChannel.onclose = () => console.log('Data channel closed');
        this.dataChannel.onerror = (err) => console.error('Data channel error:', err);
    }

    handleSignalingMessage(msg) {
        switch (msg.type) {
            case 'answer':
                if (this.peerConnection) {
                    const desc = { type: 'answer', sdp: msg.sdp };
                    this.peerConnection.setRemoteDescription(desc).then(() => {
                        console.log('✅ Remote description set');
                        // Add any queued ICE candidates
                        this.iceCandidateQueue.forEach(c => this.peerConnection.addIceCandidate(c));
                        this.iceCandidateQueue = [];
                    });
                }
                break;
            case 'candidate':
                if (this.peerConnection && this.peerConnection.remoteDescription) {
                    this.peerConnection.addIceCandidate(msg.candidate);
                } else {
                    // Queue if peerConnection not ready
                    this.iceCandidateQueue.push(msg.candidate);
                }
                break;
        }
    }

    sendSignalingMessage(msg) {
        msg.from = this.clientId;
        if (this.signalingSocket?.readyState === WebSocket.OPEN) {
            this.signalingSocket.send(JSON.stringify(msg));
        }
    }

    handleConnectionError(msg) {
        console.error('❌ Connection error:', msg);
        this.isConnecting = false;
        if (this.peerConnection) this.peerConnection.close();
        this.peerConnection = null;
        document.getElementById('connectBtn').disabled = false;
    }

    setupUI() {
        document.getElementById('startBtn').onclick = () => this.startCamera();
        document.getElementById('connectBtn').onclick = () => this.connectToServer();
    }
}

window.addEventListener('load', () => {
    const client = new WebRTCClient();
    client.initialize().catch(console.error);
});


