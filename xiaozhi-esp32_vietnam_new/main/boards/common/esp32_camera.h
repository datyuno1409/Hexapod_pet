#pragma once
#include "sdkconfig.h"

#ifndef CONFIG_IDF_TARGET_ESP32
#include <lvgl.h>
#include <thread>
#include <memory>
#include <vector>
#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "camera.h"
#include "jpg/image_to_jpeg.h"
#include "esp_video_init.h"

struct JpegChunk {
    uint8_t* data;
    size_t len;
};

class Esp32Camera : public Camera {
private:
    struct FrameBuffer {
        uint8_t *data = nullptr;
        size_t len = 0;
        uint16_t width = 0;
        uint16_t height = 0;
        v4l2_pix_fmt_t format = 0;
    } frame_;

    // Double-buffer for async capture pipeline (used by stream tasks)
    static constexpr int kDoubleBufferCount = 2;
    FrameBuffer stream_buffers_[kDoubleBufferCount];
    std::atomic<int> ready_index_{0};   // Index of the most recently captured frame (available for encode)
    std::atomic<int> capture_index_{1}; // Index of the buffer currently being captured into
    SemaphoreHandle_t capture_sem_ = nullptr;   // Signaled when a new frame is captured
    SemaphoreHandle_t encode_sem_ = nullptr;    // Signaled when JPEG encoding is done

    v4l2_pix_fmt_t sensor_format_ = 0;
    uint16_t sensor_width_ = 0;
    uint16_t sensor_height_ = 0;
    int video_fd_ = -1;
    bool streaming_on_ = false;
    struct MmapBuffer { void *start = nullptr; size_t length = 0; };
    std::vector<MmapBuffer> mmap_buffers_;
    std::string explain_url_;
    std::string explain_token_;
    std::thread encoder_thread_;

    /**
     * @brief Internal capture into a specific FrameBuffer.
     * Handles VIDIOC_DQBUF, copy/downscale, VIDIOC_QBUF.
     * Optionally shows preview on display.
     */
    bool CaptureInto(FrameBuffer& buf, bool show_preview);

public:
    Esp32Camera(const esp_video_init_config_t& config, uint16_t width = 0, uint16_t height = 0);
    ~Esp32Camera();

    virtual void SetExplainUrl(const std::string& url, const std::string& token);
    virtual bool Capture();
    void SetStreaming(bool enabled) { streaming_on_ = enabled; }
    // 翻转控制函数
    virtual bool SetHMirror(bool enabled) override;
    virtual bool SetVFlip(bool enabled) override;
    virtual std::string Explain(const std::string& question);
    virtual bool GetFrame(uint8_t** data, size_t* len, uint16_t* width, uint16_t* height, uint32_t* format) override;

    /**
     * @brief Capture a frame into the stream double-buffer (for streaming pipeline).
     * Captures into stream_buffers_[capture_index_] and swaps ready_index_.
     * Signals capture_sem_ when done.
     * @param show_preview Whether to show preview on display (default false for streaming)
     * @return true if capture succeeded
     */
    bool CaptureAsync(bool show_preview = false);

    /**
     * @brief Set software-scaled stream output size without touching sensor XCLK.
     */
    void SetStreamFrameSize(uint16_t width, uint16_t height);

    /**
     * @brief Get the most recently captured frame from the stream double-buffer.
     * Thread-safe: can be called from a different task than CaptureAsync().
     */
    bool GetReadyFrame(uint8_t** data, size_t* len, uint16_t* width, uint16_t* height, uint32_t* format);

    /**
     * @brief Get the capture semaphore (for stream pipeline synchronization).
     * The capture task signals this semaphore after each frame capture.
     */
    SemaphoreHandle_t GetCaptureSemaphore() const { return capture_sem_; }

    /**
     * @brief Get the encode semaphore (for stream pipeline synchronization).
     * The encode task signals this semaphore after encoding each frame.
     */
    SemaphoreHandle_t GetEncodeSemaphore() const { return encode_sem_; }
};

#endif // ndef CONFIG_IDF_TARGET_ESP32
