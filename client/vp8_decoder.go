// package main

// import (
//     "errors"
//     "fmt"
//     "log"
//     "unsafe"
// )

// /*
// #cgo pkg-config: vpx
// #include <vpx/vpx_decoder.h>
// #include <vpx/vp8dx.h>
// #include <stdlib.h>

// // Helper function to get codec interface
// vpx_codec_iface_t* get_vp8_decoder_interface() {
//     return vpx_codec_vp8_dx();
// }
// */
// import "C"

// // VP8Decoder wraps libvpx VP8 decoder
// type VP8Decoder struct {
//     ctx       C.vpx_codec_ctx_t
//     iter      C.vpx_codec_iter_t
//     init      bool
// }

// // NewVP8Decoder creates a new VP8 decoder instance
// func NewVP8Decoder() (*VP8Decoder, error) {
//     decoder := &VP8Decoder{}
    
//     // Initialize decoder
//     iface := C.get_vp8_decoder_interface()
//     if iface == nil {
//         return nil, errors.New("failed to get VP8 decoder interface")
//     }
    
//     cfg := C.vpx_codec_dec_cfg_t{}
//     cfg.threads = 4 // Use 4 threads for better performance
    
//     res := C.vpx_codec_dec_init(&decoder.ctx, iface, &cfg, 0)
//     if res != C.VPX_CODEC_OK {
//         return nil, fmt.Errorf("failed to initialize VP8 decoder: %s", 
//             C.GoString(C.vpx_codec_error(&decoder.ctx)))
//     }
    
//     decoder.init = true
//     log.Printf("✅ VP8 decoder initialized successfully")
//     return decoder, nil
// }

// // DecodeFrame decodes a VP8 frame and returns raw YUV data
// func (d *VP8Decoder) DecodeFrame(frameData []byte) ([]byte, int, int, error) {
//     if !d.init {
//         return nil, 0, 0, errors.New("decoder not initialized")
//     }
    
//     if len(frameData) == 0 {
//         return nil, 0, 0, errors.New("empty frame data")
//     }
    
//     // Decode the frame
//     data := (*C.uchar)(unsafe.Pointer(&frameData[0]))
//     size := C.uint(len(frameData))
    
//     res := C.vpx_codec_decode(&d.ctx, data, size, nil, 0)
//     if res != C.VPX_CODEC_OK {
//         return nil, 0, 0, fmt.Errorf("VP8 decode error: %s", 
//             C.GoString(C.vpx_codec_error(&d.ctx)))
//     }
    
//     // Get the decoded image
//     d.iter = nil
//     img := C.vpx_codec_get_frame(&d.ctx, &d.iter)
//     if img == nil {
//         return nil, 0, 0, errors.New("no frame available")
//     }
    
//     width := int(img.d_w)
//     height := int(img.d_h)
    
//     if width <= 0 || height <= 0 {
//         return nil, 0, 0, fmt.Errorf("invalid dimensions: %dx%d", width, height)
//     }
    
//     // Extract YUV420 data
//     yPlane := unsafe.Pointer(img.planes[0])
//     uPlane := unsafe.Pointer(img.planes[1])
//     vPlane := unsafe.Pointer(img.planes[2])
    
//     yStride := int(img.stride[0])
//     uvStride := int(img.stride[1])
    
//     // Calculate sizes
//     ySize := width * height
//     uvSize := (width / 2) * (height / 2)
//     totalSize := ySize + (2 * uvSize)
    
//     // Allocate output buffer
//     output := make([]byte, totalSize)
    
//     // Copy Y plane
//     yData := (*[1 << 30]byte)(yPlane)[:ySize:ySize]
//     if yStride == width {
//         // Contiguous data, copy all at once
//         copy(output[:ySize], yData)
//     } else {
//         // Copy row by row
//         for i := 0; i < height; i++ {
//             srcOffset := i * yStride
//             dstOffset := i * width
//             copy(output[dstOffset:dstOffset+width], 
//                  yData[srcOffset:srcOffset+width])
//         }
//     }
    
//     // Copy U plane
//     uData := (*[1 << 30]byte)(uPlane)[:uvSize:uvSize]
//     uvWidth := width / 2
//     uvHeight := height / 2
    
//     if uvStride == uvWidth {
//         copy(output[ySize:ySize+uvSize], uData)
//     } else {
//         for i := 0; i < uvHeight; i++ {
//             srcOffset := i * uvStride
//             dstOffset := ySize + (i * uvWidth)
//             copy(output[dstOffset:dstOffset+uvWidth], 
//                  uData[srcOffset:srcOffset+uvWidth])
//         }
//     }
    
//     // Copy V plane
//     vData := (*[1 << 30]byte)(vPlane)[:uvSize:uvSize]
//     vOffset := ySize + uvSize
    
//     if uvStride == uvWidth {
//         copy(output[vOffset:vOffset+uvSize], vData)
//     } else {
//         for i := 0; i < uvHeight; i++ {
//             srcOffset := i * uvStride
//             dstOffset := vOffset + (i * uvWidth)
//             copy(output[dstOffset:dstOffset+uvWidth], 
//                  vData[srcOffset:srcOffset+uvWidth])
//         }
//     }
    
//     log.Printf("🎬 Decoded VP8 frame: %dx%d, %d bytes YUV420", width, height, totalSize)
//     return output, width, height, nil
// }

// // Close releases decoder resources
// func (d *VP8Decoder) Close() error {
//     if d.init {
//         res := C.vpx_codec_destroy(&d.ctx)
//         if res != C.VPX_CODEC_OK {
//             return fmt.Errorf("error destroying VP8 decoder: %s", 
//                 C.GoString(C.vpx_codec_error(&d.ctx)))
//         }
//         d.init = false
//         log.Printf("✅ VP8 decoder closed")
//     }
//     return nil
// }

// // Alternative implementation using FFmpeg (if libvpx is not available)
// // This would require CGO bindings to FFmpeg libraries
// func NewFFmpegVP8Decoder() (*VP8Decoder, error) {
//     // Implementation would go here using FFmpeg's libavcodec
//     // For now, return an error to indicate this needs implementation
//     return nil, errors.New("FFmpeg VP8 decoder not implemented - use libvpx version")
// }
