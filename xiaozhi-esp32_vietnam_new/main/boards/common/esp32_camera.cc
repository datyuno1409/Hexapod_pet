#include "esp32_camera.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>
#include "board.h"
#include "display.h"
#include "esp_imgfx_color_convert.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "jpg/image_to_jpeg.h"
#include "linux/videodev2.h"
#include "lvgl_display.h"
#include <utility>
#include "mcp_server.h"
#include "system_info.h"

#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL MAX(CONFIG_LOG_DEFAULT_LEVEL, ESP_LOG_DEBUG)
#endif  // CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE

#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "driver/ppa.h"
#if defined(CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE_90)
#define IMAGE_ROTATION_ANGLE (PPA_SRM_ROTATION_ANGLE_270)
#elif defined(CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE_270)
#define IMAGE_ROTATION_ANGLE (PPA_SRM_ROTATION_ANGLE_90)
#else
#error "CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE is not set"
#endif  // angle
#else   // target
#include "esp_imgfx_rotate.h"
#if defined(CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE_90)
#define IMAGE_ROTATION_ANGLE (90)
#elif defined(CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE_270)
#define IMAGE_ROTATION_ANGLE (270)
#else
#error "CONFIG_XIAOZHI_CAMERA_IMAGE_ROTATION_ANGLE is not set"
#endif  // angle
#endif  // target
#endif  // CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE

#include <errno.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <cstdio>
#include <cstring>

#define TAG "Esp32Camera"

#if defined(CONFIG_CAMERA_SENSOR_SWAP_PIXEL_BYTE_ORDER) || defined(CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP)
#warning \
    "CAMERA_SENSOR_SWAP_PIXEL_BYTE_ORDER or CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP is enabled, which may cause image corruption in YUV422 format!"
#endif

#if CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE
#define CAM_PRINT_FOURCC(pixelformat)       \
    char fourcc[5];                         \
    fourcc[0] = pixelformat & 0xFF;         \
    fourcc[1] = (pixelformat >> 8) & 0xFF;  \
    fourcc[2] = (pixelformat >> 16) & 0xFF; \
    fourcc[3] = (pixelformat >> 24) & 0xFF; \
    fourcc[4] = '\0';                       \
    ESP_LOGD(TAG, "FOURCC: '%c%c%c%c'", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

// for compatibility with old esp_video version
#ifndef MAP_FAILED
#define MAP_FAILED nullptr
#endif

__attribute__((weak)) esp_err_t esp_video_deinit(void) {
    return ESP_ERR_NOT_SUPPORTED;
}
// end of for compatibility with old esp_video version

static void log_available_video_devices() {
    for (int i = 0; i < 50; i++) {
        char path[16];
        snprintf(path, sizeof(path), "/dev/video%d", i);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            ESP_LOGD(TAG, "found video device: %s", path);
            close(fd);
        }
    }
}
#else
#define CAM_PRINT_FOURCC(pixelformat) (void)0;
#endif  // CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE

static void downscale_yuyv(const uint8_t* src, uint16_t src_w, uint16_t src_h,
                           uint8_t* dst, uint16_t dst_w, uint16_t dst_h) {
    uint16_t* sx_pair_lut = (uint16_t*)__builtin_alloca((dst_w / 2) * sizeof(uint16_t));
    for (uint16_t dx_pair = 0; dx_pair < dst_w / 2; dx_pair++) {
        sx_pair_lut[dx_pair] = (dx_pair * src_w) / dst_w;
    }
    uint16_t* sy_lut = (uint16_t*)__builtin_alloca(dst_h * sizeof(uint16_t));
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        sy_lut[dy] = (dy * src_h) / dst_h;
    }
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        uint16_t sy = sy_lut[dy];
        const uint32_t* src_row = (const uint32_t*)(src + sy * src_w * 2);
        uint32_t* dst_row = (uint32_t*)(dst + dy * dst_w * 2);
        for (uint16_t dx_pair = 0; dx_pair < dst_w / 2; dx_pair++) {
            dst_row[dx_pair] = src_row[sx_pair_lut[dx_pair]];
        }
    }
}

static void downscale_rgb565(const uint8_t* src, uint16_t src_w, uint16_t src_h,
                             uint8_t* dst, uint16_t dst_w, uint16_t dst_h) {
    const uint16_t* src16 = (const uint16_t*)src;
    uint16_t* dst16 = (uint16_t*)dst;
    uint16_t* sx_lut = (uint16_t*)__builtin_alloca(dst_w * sizeof(uint16_t));
    for (uint16_t dx = 0; dx < dst_w; dx++) {
        sx_lut[dx] = (dx * src_w) / dst_w;
    }
    uint16_t* sy_lut = (uint16_t*)__builtin_alloca(dst_h * sizeof(uint16_t));
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        sy_lut[dy] = (dy * src_h) / dst_h;
    }
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        uint16_t sy = sy_lut[dy];
        const uint16_t* src_row = src16 + sy * src_w;
        uint16_t* dst_row = dst16 + dy * dst_w;
        for (uint16_t dx = 0; dx < dst_w; dx++) {
            dst_row[dx] = src_row[sx_lut[dx]];
        }
    }
}

static void downscale_grey(const uint8_t* src, uint16_t src_w, uint16_t src_h,
                            uint8_t* dst, uint16_t dst_w, uint16_t dst_h) {
    uint16_t* sx_lut = (uint16_t*)__builtin_alloca(dst_w * sizeof(uint16_t));
    for (uint16_t dx = 0; dx < dst_w; dx++) {
        sx_lut[dx] = (dx * src_w) / dst_w;
    }
    uint16_t* sy_lut = (uint16_t*)__builtin_alloca(dst_h * sizeof(uint16_t));
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        sy_lut[dy] = (dy * src_h) / dst_h;
    }
    for (uint16_t dy = 0; dy < dst_h; dy++) {
        uint16_t sy = sy_lut[dy];
        const uint8_t* src_row = src + sy * src_w;
        uint8_t* dst_row = dst + dy * dst_w;
        for (uint16_t dx = 0; dx < dst_w; dx++) {
            dst_row[dx] = src_row[sx_lut[dx]];
        }
    }
}

static void downscale_yuv422p(const uint8_t* src, uint16_t src_w, uint16_t src_h,
                              uint8_t* dst, uint16_t dst_w, uint16_t dst_h) {
    // 1. Y plane: src_w * src_h -> dst_w * dst_h
    const uint8_t* src_y = src;
    uint8_t* dst_y = dst;
    downscale_grey(src_y, src_w, src_h, dst_y, dst_w, dst_h);

    // 2. U plane: (src_w / 2) * src_h -> (dst_w / 2) * dst_h
    const uint8_t* src_u = src + (int)src_w * (int)src_h;
    uint8_t* dst_u = dst + (int)dst_w * (int)dst_h;
    downscale_grey(src_u, src_w / 2, src_h, dst_u, dst_w / 2, dst_h);

    // 3. V plane: (src_w / 2) * src_h -> (dst_w / 2) * dst_h
    const uint8_t* src_v = src_u + ((int)src_w / 2) * (int)src_h;
    uint8_t* dst_v = dst_u + ((int)dst_w / 2) * (int)dst_h;
    downscale_grey(src_v, src_w / 2, src_h, dst_v, dst_w / 2, dst_h);
}


Esp32Camera::Esp32Camera(const esp_video_init_config_t& config, uint16_t width, uint16_t height) {
    if (esp_video_init(&config) != ESP_OK) {
        ESP_LOGE(TAG, "esp_video_init failed");
        return;
    }

#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif  // CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE

    const char* video_device_name = nullptr;

    if (false) { /* ç”¨äºŽæž„å»º else if */
    }
#if CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE
    else if (config.csi != nullptr) {
        video_device_name = ESP_VIDEO_MIPI_CSI_DEVICE_NAME;
    }
#endif
#if CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE
    else if (config.dvp != nullptr) {
        video_device_name = ESP_VIDEO_DVP_DEVICE_NAME;
    }
#endif
#if CONFIG_ESP_VIDEO_ENABLE_HW_JPEG_VIDEO_DEVICE
    else if (config.jpeg != nullptr) {
        video_device_name = ESP_VIDEO_JPEG_DEVICE_NAME;
    }
#endif
#if CONFIG_ESP_VIDEO_ENABLE_SPI_VIDEO_DEVICE
    else if (config.spi != nullptr) {
        video_device_name = ESP_VIDEO_SPI_DEVICE_NAME;
    }
#endif
#if CONFIG_ESP_VIDEO_ENABLE_USB_UVC_VIDEO_DEVICE
    else if (config.usb_uvc != nullptr) {
        video_device_name = ESP_VIDEO_USB_UVC_DEVICE_NAME(config.usb_uvc->uvc.uvc_dev_num);
    }
#endif

    if (video_device_name == nullptr) {
        ESP_LOGE(TAG, "no video device is enabled");
        return;
    }

    // Try preferred device first, then fallback to search for any available video device
    video_fd_ = open(video_device_name, O_RDWR);

    if (video_fd_ < 0) {
        ESP_LOGW(TAG, "Preferred device %s failed, searching for alternative...", video_device_name);
        for (int i = 0; i < 8; i++) {
            char path[32];
            snprintf(path, sizeof(path), "/dev/video%d", i);
            int fd = open(path, O_RDWR);
            if (fd >= 0) {
                ESP_LOGI(TAG, "Found alternative video device: %s", path);
                video_fd_ = fd;
                break;
            }
        }
    }

    if (video_fd_ < 0) {
        ESP_LOGE(TAG, "open %s failed and no alternatives found, errno=%d(%s)", video_device_name, errno, strerror(errno));
#if CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE
        log_available_video_devices();
#endif  // CONFIG_XIAOZHI_ENABLE_CAMERA_DEBUG_MODE
        return;
    }

    struct v4l2_capability cap = {};
    if (ioctl(video_fd_, VIDIOC_QUERYCAP, &cap) != 0) {
        ESP_LOGE(TAG, "VIDIOC_QUERYCAP failed, errno=%d(%s)", errno, strerror(errno));
        close(video_fd_);
        video_fd_ = -1;
        return;
    }

    ESP_LOGI(
        TAG,
        "VIDIOC_QUERYCAP: driver=%s, card=%s, bus_info=%s, version=0x%08lx, capabilities=0x%08lx, device_caps=0x%08lx",
        cap.driver, cap.card, cap.bus_info, cap.version, cap.capabilities, cap.device_caps);

    struct v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd_, VIDIOC_G_FMT, &format) != 0) {
        ESP_LOGE(TAG, "VIDIOC_G_FMT failed, errno=%d(%s)", errno, strerror(errno));
        close(video_fd_);
        video_fd_ = -1;
        return;
    }
    ESP_LOGI(TAG, "VIDIOC_G_FMT: pixelformat=0x%08lx, width=%ld, height=%ld", format.fmt.pix.pixelformat,
             format.fmt.pix.width, format.fmt.pix.height);
    CAM_PRINT_FOURCC(format.fmt.pix.pixelformat);

    struct v4l2_format setformat = {};
    setformat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // Configure sensor to output requested resolution directly (avoids software downscale).
    // Fallback to native resolution if S_FMT fails.
    setformat.fmt.pix.width = width ? width : format.fmt.pix.width;
    setformat.fmt.pix.height = height ? height : format.fmt.pix.height;

    struct v4l2_fmtdesc fmtdesc = {};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    uint32_t best_fmt = 0;
    int best_rank = 1 << 30;  // large number

    // æ³¨: å½“å‰ç‰ˆæœ¬ esp_video ä¸­ YUV422P å®žé™…è¾“å‡ºä¸º YUYVã€‚
#if defined(CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE) && defined(CONFIG_SOC_PPA_SUPPORTED)
    auto get_rank = [](uint32_t fmt) -> int {
        switch (fmt) {
            case V4L2_PIX_FMT_RGB24:
                return 0;
            case V4L2_PIX_FMT_RGB565:
                return 1;
            case V4L2_PIX_FMT_YUV420:
#ifdef CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_ENCODER
                return 2;
#else
                // è½¯ä»¶ JPEG ç¼–ç å™¨ä¸æ”¯æŒ YUV420 æ ¼å¼
                [[fallthrough]];
#endif  // CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_ENCODER
            case V4L2_PIX_FMT_GREY:
            case V4L2_PIX_FMT_YUV422P:
            default:
                return 1 << 29;  // unsupported
        }
    };
#else
    auto get_rank = [](uint32_t fmt) -> int {
        switch (fmt) {
            case V4L2_PIX_FMT_JPEG:
                return 0;
            case V4L2_PIX_FMT_MJPEG:
                return 1;
            case V4L2_PIX_FMT_RGB565:
                return 2;
            case V4L2_PIX_FMT_RGB24:
                return 3;
#ifdef CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_ENCODER
            case V4L2_PIX_FMT_YUV420:
                return 4;
#endif  // CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_ENCODER
            case V4L2_PIX_FMT_GREY:
                return 5;
            case V4L2_PIX_FMT_YUV422P:
                return 6;
            default:
                return 1 << 29;  // unsupported
        }
    };
#endif
    while (ioctl(video_fd_, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        ESP_LOGI(TAG, "VIDIOC_ENUM_FMT: pixelformat=0x%08lx, description=%s", fmtdesc.pixelformat, fmtdesc.description);
        CAM_PRINT_FOURCC(fmtdesc.pixelformat);
        int rank = get_rank(fmtdesc.pixelformat);
        if (rank < best_rank) {
            best_rank = rank;
            best_fmt = fmtdesc.pixelformat;
        }
        fmtdesc.index++;
    }
    if (best_rank < (1 << 29)) {
        setformat.fmt.pix.pixelformat = best_fmt;
        sensor_format_ = best_fmt;
    }

    if (!setformat.fmt.pix.pixelformat) {
        ESP_LOGE(TAG, "no supported pixel format found");
        close(video_fd_);
        video_fd_ = -1;
        sensor_format_ = 0;
        return;
    }

    ESP_LOGI(TAG, "selected pixel format: 0x%08lx", setformat.fmt.pix.pixelformat);

    const uint32_t selected_pixfmt = setformat.fmt.pix.pixelformat;
    const std::pair<uint16_t, uint16_t> resolution_chain[] = {
        {static_cast<uint16_t>(width ? width : format.fmt.pix.width),
         static_cast<uint16_t>(height ? height : format.fmt.pix.height)},
        {320, 240},
        {160, 120},
        {176, 144},
        {static_cast<uint16_t>(format.fmt.pix.width), static_cast<uint16_t>(format.fmt.pix.height)},
    };

    bool format_set = false;
    for (const auto& resolution : resolution_chain) {
        const uint16_t try_w = resolution.first;
        const uint16_t try_h = resolution.second;
        setformat = {};
        setformat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setformat.fmt.pix.pixelformat = selected_pixfmt;
        setformat.fmt.pix.width = try_w;
        setformat.fmt.pix.height = try_h;
        if (ioctl(video_fd_, VIDIOC_S_FMT, &setformat) == 0) {
            format_set = true;
            ESP_LOGI(TAG, "VIDIOC_S_FMT ok: %lux%lu fmt=0x%08lx",
                     setformat.fmt.pix.width, setformat.fmt.pix.height, setformat.fmt.pix.pixelformat);
            break;
        }
        ESP_LOGW(TAG, "VIDIOC_S_FMT failed for %ux%u fmt=0x%08lx, errno=%d(%s)",
                 try_w, try_h, selected_pixfmt, errno, strerror(errno));
    }

    if (!format_set) {
        ESP_LOGE(TAG, "VIDIOC_S_FMT failed for all fallback resolutions");
        close(video_fd_);
        video_fd_ = -1;
        sensor_format_ = 0;
        return;
    }
    sensor_format_ = setformat.fmt.pix.pixelformat;

    sensor_width_ = setformat.fmt.pix.width;
    sensor_height_ = setformat.fmt.pix.height;

#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
    if (width > 0 && height > 0) {
        frame_.width = height;
        frame_.height = width;
    } else {
        frame_.width = sensor_height_;
        frame_.height = sensor_width_;
    }
#else
    if (width > 0 && height > 0) {
        frame_.width = width;
        frame_.height = height;
    } else {
        frame_.width = sensor_width_;
        frame_.height = sensor_height_;
    }
#endif


    // ç”³è¯·ç¼“å†²å¹¶mmap
    struct v4l2_requestbuffers req = {};
    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd_, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "VIDIOC_REQBUFS failed");
        close(video_fd_);
        video_fd_ = -1;
        sensor_format_ = 0;
        return;
    }
    mmap_buffers_.resize(req.count);
    for (uint32_t i = 0; i < req.count; i++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(video_fd_, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QUERYBUF failed");
            close(video_fd_);
            video_fd_ = -1;
            sensor_format_ = 0;
            return;
        }
        void* start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd_, buf.m.offset);
        if (start == MAP_FAILED) {
            ESP_LOGE(TAG, "mmap failed");
            close(video_fd_);
            video_fd_ = -1;
            sensor_format_ = 0;
            return;
        }
        mmap_buffers_[i].start = start;
        mmap_buffers_[i].length = buf.length;

        if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QBUF failed");
            close(video_fd_);
            video_fd_ = -1;
            sensor_format_ = 0;
            return;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd_, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "VIDIOC_STREAMON failed");
        close(video_fd_);
        video_fd_ = -1;
        sensor_format_ = 0;
        return;
    }

#ifdef CONFIG_ESP_VIDEO_ENABLE_ISP_VIDEO_DEVICE
    // å½“å¯ç”¨ ISP æ—¶ï¼ŒISP éœ€è¦ä¸€äº›ç…§ç‰‡æ¥åˆå§‹åŒ–å‚æ•°ï¼Œå› æ­¤å¼€å¯åŽåŽå°æ‹æ‘„5sç…§ç‰‡å¹¶ä¸¢å¼ƒ
    xTaskCreate(
        [](void* arg) {
            Esp32Camera* self = static_cast<Esp32Camera*>(arg);
            uint16_t capture_count = 0;
            TickType_t start = xTaskGetTickCount();
            TickType_t duration = 5000 / portTICK_PERIOD_MS;  // 5s
            while ((xTaskGetTickCount() - start) < duration) {
                struct v4l2_buffer buf = {};
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                if (ioctl(self->video_fd_, VIDIOC_DQBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "VIDIOC_DQBUF failed during init");
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    continue;
                }
                if (ioctl(self->video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "VIDIOC_QBUF failed during init");
                }
                capture_count++;
            }
            ESP_LOGI(TAG, "Camera init success, captured %d frames in %dms", capture_count,
                     (xTaskGetTickCount() - start) * portTICK_PERIOD_MS);
            self->streaming_on_ = true;
            vTaskDelete(NULL);
        },
        "CameraInitTask", 4096, this, 5, nullptr);
#else
    ESP_LOGI(TAG, "Camera init success");
    streaming_on_ = true;
    capture_sem_ = xSemaphoreCreateBinary();
    encode_sem_ = xSemaphoreCreateBinary();
#endif  // CONFIG_ESP_VIDEO_ENABLE_ISP_VIDEO_DEVICE
}

Esp32Camera::~Esp32Camera() {
    if (streaming_on_ && video_fd_ >= 0) {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(video_fd_, VIDIOC_STREAMOFF, &type);
    }
    for (auto& b : mmap_buffers_) {
        if (b.start && b.length) {
            munmap(b.start, b.length);
        }
    }
    if (video_fd_ >= 0) {
        close(video_fd_);
        video_fd_ = -1;
    }
    sensor_format_ = 0;
    esp_video_deinit();
}

void Esp32Camera::SetExplainUrl(const std::string& url, const std::string& token) {
    explain_url_ = url;
    explain_token_ = token;
}

bool Esp32Camera::Capture() {
    if (encoder_thread_.joinable()) {
        encoder_thread_.join();
    }

    if (!streaming_on_ || video_fd_ < 0) {
        return false;
    }

    for (int i = 0; i < 1; i++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(video_fd_, VIDIOC_DQBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_DQBUF failed");
            return false;
        }
        if (i == 0) {
            // ä¿å­˜å¸§å‰¯æœ¬åˆ°PSRAM
            size_t bytes_per_pixel = 2;
            if (sensor_format_ == V4L2_PIX_FMT_GREY) {
                bytes_per_pixel = 1;
            } else if (sensor_format_ == V4L2_PIX_FMT_RGB24) {
                bytes_per_pixel = 3;
            }
            size_t needed_len = (sensor_format_ == V4L2_PIX_FMT_JPEG || sensor_format_ == V4L2_PIX_FMT_MJPEG)
                                ? mmap_buffers_[buf.index].length
                                : (size_t)frame_.width * (size_t)frame_.height * bytes_per_pixel;
            if (!frame_.data || frame_.len != needed_len) {
                if (frame_.data) {
                    heap_caps_free(frame_.data);
                }
                frame_.len = needed_len;
                frame_.data = (uint8_t*)heap_caps_malloc(frame_.len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            }

            if (!frame_.data) {
                ESP_LOGE(TAG, "alloc frame copy failed");
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }

#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
            ESP_LOGD(TAG, "mmap_buffers_[buf.index].length = %d, sensor_width = %d, sensor_height = %d",
                     (int)mmap_buffers_[buf.index].length, sensor_width_, sensor_height_);
#else
            ESP_LOGD(TAG, "mmap_buffers_[buf.index].length = %d, frame.width = %d, frame.height = %d",
                     (int)mmap_buffers_[buf.index].length, frame_.width, frame_.height);
#endif  // CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
            ESP_LOG_BUFFER_HEXDUMP(TAG, mmap_buffers_[buf.index].start, MIN(mmap_buffers_[buf.index].length, 256),
                                   ESP_LOG_DEBUG);

            bool needs_downscale = (sensor_width_ != frame_.width || sensor_height_ != frame_.height);

            switch (sensor_format_) {
                case V4L2_PIX_FMT_JPEG:
                case V4L2_PIX_FMT_MJPEG:
                    frame_.len = buf.bytesused;
                    memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    frame_.format = sensor_format_;
                    break;
                case V4L2_PIX_FMT_RGB565:
                    if (needs_downscale) {
                        downscale_rgb565((const uint8_t*)mmap_buffers_[buf.index].start, sensor_width_, sensor_height_,
                                         frame_.data, frame_.width, frame_.height);
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP
                    {
                        auto dst16 = (uint16_t*)frame_.data;
                        size_t count = frame_.len / 2;
                        for (size_t i = 0; i < count; i++) {
                            dst16[i] = __builtin_bswap16(dst16[i]);
                        }
                    }
#endif
                    frame_.format = sensor_format_;
                    break;

                case V4L2_PIX_FMT_RGB24:
                    if (needs_downscale) {
                        ESP_LOGW(TAG, "Downscaling not supported for RGB24, copying directly");
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, MIN(mmap_buffers_[buf.index].length, frame_.len));
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
                    frame_.format = sensor_format_;
                    break;

                case V4L2_PIX_FMT_YUYV:
                    if (needs_downscale) {
                        downscale_yuyv((const uint8_t*)mmap_buffers_[buf.index].start, sensor_width_, sensor_height_,
                                       frame_.data, frame_.width, frame_.height);
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP
                    {
                        auto dst16 = (uint16_t*)frame_.data;
                        size_t count = frame_.len / 2;
                        for (size_t i = 0; i < count; i++) {
                            dst16[i] = __builtin_bswap16(dst16[i]);
                        }
                    }
#endif
                    frame_.format = sensor_format_;
                    break;

                case V4L2_PIX_FMT_YUV420:
                    if (needs_downscale) {
                        ESP_LOGW(TAG, "Downscaling not supported for YUV420, copying directly");
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, MIN(mmap_buffers_[buf.index].length, frame_.len));
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
                    frame_.format = sensor_format_;
                    break;

                case V4L2_PIX_FMT_GREY:
                    if (needs_downscale) {
                        downscale_grey((const uint8_t*)mmap_buffers_[buf.index].start, sensor_width_, sensor_height_,
                                       frame_.data, frame_.width, frame_.height);
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
                    frame_.format = sensor_format_;
                    break;

                case V4L2_PIX_FMT_YUV422P:
                    if (needs_downscale) {
                        downscale_yuv422p((const uint8_t*)mmap_buffers_[buf.index].start, sensor_width_, sensor_height_,
                                       frame_.data, frame_.width, frame_.height);
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP
                    {
                        auto dst16 = (uint16_t*)frame_.data;
                        size_t count = frame_.len / 2;
                        for (size_t i = 0; i < count; i++) {
                            dst16[i] = __builtin_bswap16(dst16[i]);
                        }
                    }
#endif
                    frame_.format = V4L2_PIX_FMT_YUV422P;
                    break;

                case V4L2_PIX_FMT_RGB565X:
                    if (needs_downscale) {
                        downscale_rgb565((const uint8_t*)mmap_buffers_[buf.index].start, sensor_width_, sensor_height_,
                                         frame_.data, frame_.width, frame_.height);
                    } else {
                        memcpy(frame_.data, mmap_buffers_[buf.index].start, frame_.len);
                    }
                    {
                        auto dst16 = (uint16_t*)frame_.data;
                        size_t count = frame_.len / 2;
                        for (size_t i = 0; i < count; i++) {
                            dst16[i] = __builtin_bswap16(dst16[i]);
                        }
                    }
                    frame_.format = V4L2_PIX_FMT_RGB565;
                    break;

                default:
                    ESP_LOGE(TAG, "unsupported sensor format: 0x%08x", sensor_format_);
                    if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                        ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                    }
                    return false;
            }

#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
#ifndef CONFIG_SOC_PPA_SUPPORTED
            uint8_t* rotate_dst =
                (uint8_t*)heap_caps_aligned_alloc(64, frame_.len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (rotate_dst == nullptr) {
                ESP_LOGE(TAG, "Failed to allocate memory for rotate image");
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }
            uint8_t* rotate_src = (uint8_t*)frame_.data;

            esp_imgfx_rotate_cfg_t rotate_cfg = {
                .in_res =
                    {
                        .width = static_cast<int16_t>(sensor_width_),
                        .height = static_cast<int16_t>(sensor_height_),
                    },
                .degree = IMAGE_ROTATION_ANGLE,
            };
            switch (frame_.format) {
                case V4L2_PIX_FMT_RGB565:
                    rotate_cfg.in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
                    break;
                case V4L2_PIX_FMT_YUYV:
                    rotate_cfg.in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
                    break;
                case V4L2_PIX_FMT_GREY:
                    rotate_cfg.in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_Y;
                    break;
                case V4L2_PIX_FMT_RGB24:
                    rotate_cfg.in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888;
                    break;
                default:
                    ESP_LOGE(TAG, "unsupported sensor format: 0x%08x", sensor_format_);
                    if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                        ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                    }
                    return false;
            }
            esp_imgfx_rotate_handle_t rotate_handle = nullptr;
            esp_imgfx_err_t imgfx_err = esp_imgfx_rotate_open(&rotate_cfg, &rotate_handle);
            if (imgfx_err != ESP_IMGFX_ERR_OK || rotate_handle == nullptr) {
                ESP_LOGE(TAG, "esp_imgfx_rotate_create failed");
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }

            esp_imgfx_data_t rotate_input_data = {
                .data = rotate_src,
                .data_len = frame_.len,
            };
            esp_imgfx_data_t rotate_output_data = {
                .data = rotate_dst,
                .data_len = frame_.len,
            };

            imgfx_err = esp_imgfx_rotate_process(rotate_handle, &rotate_input_data, &rotate_output_data);
            if (imgfx_err != ESP_IMGFX_ERR_OK) {
                ESP_LOGE(TAG, "esp_imgfx_rotate_process failed");
                heap_caps_free(rotate_dst);
                rotate_dst = nullptr;
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                esp_imgfx_rotate_close(rotate_handle);
                rotate_handle = nullptr;
                return false;
            }

            frame_.data = rotate_dst;

            heap_caps_free(rotate_src);
            rotate_src = nullptr;

            esp_imgfx_rotate_close(rotate_handle);
            rotate_handle = nullptr;
#else   // CONFIG_SOC_PPA_SUPPORTED
            uint8_t* rotate_src = nullptr;

            ppa_srm_color_mode_t ppa_color_mode;
            switch (frame_.format) {
                case V4L2_PIX_FMT_RGB565:
                    rotate_src = (uint8_t*)frame_.data;
                    ppa_color_mode = PPA_SRM_COLOR_MODE_RGB565;
                    break;
                case V4L2_PIX_FMT_RGB24:
                    rotate_src = (uint8_t*)frame_.data;
                    ppa_color_mode = PPA_SRM_COLOR_MODE_RGB888;
                    break;
                case V4L2_PIX_FMT_YUYV: {
                    ESP_LOGW(TAG, "YUYV format is not supported for PPA rotation, using software conversion to RGB888");
                    rotate_src = (uint8_t*)heap_caps_malloc(frame_.width * frame_.height * 3,
                                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (rotate_src == nullptr) {
                        ESP_LOGE(TAG, "Failed to allocate memory for rotate image");
                        if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                            ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                        }
                        return false;
                    }
                    esp_imgfx_color_convert_cfg_t convert_cfg = {
                        .in_res = {.width = static_cast<int16_t>(frame_.width),
                                   .height = static_cast<int16_t>(frame_.height)},
                        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_YUYV,
                        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
                    };
                    esp_imgfx_color_convert_handle_t convert_handle = nullptr;
                    esp_imgfx_err_t err = esp_imgfx_color_convert_open(&convert_cfg, &convert_handle);
                    if (err != ESP_IMGFX_ERR_OK || convert_handle == nullptr) {
                        ESP_LOGE(TAG, "esp_imgfx_color_convert_open failed");
                        heap_caps_free(rotate_src);
                        rotate_src = nullptr;
                        if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                            ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                        }
                        return false;
                    }
                    esp_imgfx_data_t convert_input_data = {
                        .data = frame_.data,
                        .data_len = frame_.len,
                    };
                    esp_imgfx_data_t convert_output_data = {
                        .data = rotate_src,
                        .data_len = static_cast<uint32_t>(frame_.width * frame_.height * 3),
                    };
                    err = esp_imgfx_color_convert_process(convert_handle, &convert_input_data, &convert_output_data);
                    if (err != ESP_IMGFX_ERR_OK) {
                        ESP_LOGE(TAG, "esp_imgfx_color_convert_process failed");
                        heap_caps_free(rotate_src);
                        rotate_src = nullptr;
                        esp_imgfx_color_convert_close(convert_handle);
                        convert_handle = nullptr;
                        if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                            ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                        }
                        return false;
                    }
                    esp_imgfx_color_convert_close(convert_handle);
                    convert_handle = nullptr;
                    ppa_color_mode = PPA_SRM_COLOR_MODE_RGB888;
                    heap_caps_free(frame_.data);
                    frame_.data = rotate_src;
                    frame_.len = frame_.width * frame_.height * 3;
                    break;
                }
                default:
                    ESP_LOGE(TAG, "unsupported sensor format for PPA rotation: 0x%08x", sensor_format_);
                    if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                        ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                    }
                    return false;
            }

            uint8_t* rotate_dst = (uint8_t*)heap_caps_malloc(
                frame_.width * frame_.height * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_CACHE_ALIGNED);
            if (rotate_dst == nullptr) {
                ESP_LOGE(TAG, "Failed to allocate memory for rotate image");
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }

            ppa_client_handle_t ppa_client = nullptr;
            ppa_client_config_t client_cfg = {
                .oper_type = PPA_OPERATION_SRM,
                .max_pending_trans_num = 1,
            };
            esp_err_t err = ppa_register_client(&client_cfg, &ppa_client);
            if (err != ESP_OK || ppa_client == nullptr) {
                ESP_LOGE(TAG, "ppa_register_client failed: %d", (int)err);
                heap_caps_free(rotate_dst);
                rotate_dst = nullptr;
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }

            ppa_srm_rotation_angle_t ppa_angle = IMAGE_ROTATION_ANGLE;

            ppa_srm_oper_config_t srm_cfg = {};
            srm_cfg.in.buffer = (void*)rotate_src;
            srm_cfg.in.pic_w = sensor_width_;
            srm_cfg.in.pic_h = sensor_height_;
            srm_cfg.in.block_w = sensor_width_;
            srm_cfg.in.block_h = sensor_height_;
            srm_cfg.in.block_offset_x = 0;
            srm_cfg.in.block_offset_y = 0;
            srm_cfg.in.srm_cm = ppa_color_mode;

            srm_cfg.out.buffer = (void*)rotate_dst;
            srm_cfg.out.buffer_size = frame_.len;
            srm_cfg.out.pic_w = frame_.width;
            srm_cfg.out.pic_h = frame_.height;
            srm_cfg.out.block_offset_x = 0;
            srm_cfg.out.block_offset_y = 0;
            srm_cfg.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

            // ç­‰æ¯”ä¾‹ç¼©æ”¾ 1.0
            srm_cfg.scale_x = 1.0f;
            srm_cfg.scale_y = 1.0f;
            srm_cfg.rotation_angle = ppa_angle;
            srm_cfg.mode = PPA_TRANS_MODE_BLOCKING;
            srm_cfg.user_data = nullptr;

            err = ppa_do_scale_rotate_mirror(ppa_client, &srm_cfg);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "ppa_do_scale_rotate_mirror failed: %d", (int)err);
                heap_caps_free(rotate_dst);
                rotate_dst = nullptr;
                (void)ppa_unregister_client(ppa_client);
                if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
                    ESP_LOGE(TAG, "Cleanup: VIDIOC_QBUF failed");
                }
                return false;
            }

            (void)ppa_unregister_client(ppa_client);

            frame_.data = rotate_dst;
            frame_.len = frame_.width * frame_.height * 2;
            frame_.format = V4L2_PIX_FMT_RGB565;
            heap_caps_free(rotate_src);
            rotate_src = nullptr;
#endif  // CONFIG_SOC_PPA_SUPPORTED
#endif  // CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
        }

        if (ioctl(video_fd_, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QBUF failed");
        }
    }

    // æ˜¾ç¤ºé¢„è§ˆå›¾ç‰‡
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (display != nullptr) {
        if (!frame_.data) {
            ESP_LOGE(TAG, "frame.data is null");
            return false;
        }
        uint16_t w = frame_.width;
        uint16_t h = frame_.height;
        size_t lvgl_image_size = frame_.len;
        size_t stride = ((w * 2) + 3) & ~3;  // 4å­—èŠ‚å¯¹é½
        lv_color_format_t color_format = LV_COLOR_FORMAT_RGB565;
        uint8_t* data = nullptr;

        switch (frame_.format) {
            // LVGL æ˜¾ç¤º YUV ç³»çš„å›¾åƒä¼¼ä¹Žéƒ½æœ‰é—®é¢˜ï¼Œæš‚æ—¶è½¬æ¢ä¸º RGB565 æ˜¾ç¤º
            case V4L2_PIX_FMT_YUYV:
            case V4L2_PIX_FMT_YUV420:
            case V4L2_PIX_FMT_RGB24: {
                color_format = LV_COLOR_FORMAT_RGB565;
                data = (uint8_t*)heap_caps_malloc(w * h * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (data == nullptr) {
                    ESP_LOGE(TAG, "Failed to allocate memory for preview image");
                    return false;
                }
                esp_imgfx_color_convert_cfg_t convert_cfg = {
                    .in_res = {.width = static_cast<int16_t>(frame_.width),
                               .height = static_cast<int16_t>(frame_.height)},
                    .in_pixel_fmt = static_cast<esp_imgfx_pixel_fmt_t>(frame_.format),
                    .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
                    .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601,
                };
                esp_imgfx_color_convert_handle_t convert_handle = nullptr;
                esp_imgfx_err_t err = esp_imgfx_color_convert_open(&convert_cfg, &convert_handle);
                if (err != ESP_IMGFX_ERR_OK || convert_handle == nullptr) {
                    ESP_LOGE(TAG, "esp_imgfx_color_convert_open failed");
                    heap_caps_free(data);
                    data = nullptr;
                    return false;
                }
                esp_imgfx_data_t convert_input_data = {
                    .data = frame_.data,
                    .data_len = frame_.len,
                };
                esp_imgfx_data_t convert_output_data = {
                    .data = data,
                    .data_len = static_cast<uint32_t>(w * h * 2),
                };
                err = esp_imgfx_color_convert_process(convert_handle, &convert_input_data, &convert_output_data);
                if (err != ESP_IMGFX_ERR_OK) {
                    ESP_LOGE(TAG, "esp_imgfx_color_convert_process failed");
                    heap_caps_free(data);
                    data = nullptr;
                    esp_imgfx_color_convert_close(convert_handle);
                    convert_handle = nullptr;
                    return false;
                }
                esp_imgfx_color_convert_close(convert_handle);
                convert_handle = nullptr;
                lvgl_image_size = w * h * 2;
                break;
            }

            case V4L2_PIX_FMT_RGB565:
                data = (uint8_t*)heap_caps_malloc(w * h * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (data == nullptr) {
                    ESP_LOGE(TAG, "Failed to allocate memory for preview image");
                    return false;
                }
                memcpy(data, frame_.data, frame_.len);
                lvgl_image_size = frame_.len;  // fallthrough æ—¶å…¼é¡¾ YUYV ä¸Ž RGB565
                break;

            default:
                ESP_LOGE(TAG, "unsupported frame format: 0x%08lx", frame_.format);
                return false;
        }

        auto image = std::make_unique<LvglAllocatedImage>(data, lvgl_image_size, w, h, stride, color_format);
        display->SetPreviewImage(std::move(image));
    }
    return true;
}

bool Esp32Camera::SetHMirror(bool enabled) {
    if (video_fd_ < 0)
        return false;
    struct v4l2_ext_controls ctrls = {};
    struct v4l2_ext_control ctrl = {};
    ctrl.id = V4L2_CID_HFLIP;
    ctrl.value = enabled ? 1 : 0;
    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.count = 1;
    ctrls.controls = &ctrl;
    if (ioctl(video_fd_, VIDIOC_S_EXT_CTRLS, &ctrls) != 0) {
        ESP_LOGE(TAG, "set HFLIP failed");
        return false;
    }
    return true;
}

bool Esp32Camera::SetVFlip(bool enabled) {
    if (video_fd_ < 0)
        return false;
    struct v4l2_ext_controls ctrls = {};
    struct v4l2_ext_control ctrl = {};
    ctrl.id = V4L2_CID_VFLIP;
    ctrl.value = enabled ? 1 : 0;
    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.count = 1;
    ctrls.controls = &ctrl;
    if (ioctl(video_fd_, VIDIOC_S_EXT_CTRLS, &ctrls) != 0) {
        ESP_LOGE(TAG, "set VFLIP failed");
        return false;
    }
    return true;
}

/**
 * @brief å°†æ‘„åƒå¤´æ•èŽ·çš„å›¾åƒå‘é€åˆ°è¿œç¨‹æœåŠ¡å™¨è¿›è¡ŒAIåˆ†æžå’Œè§£é‡Š
 *
 * è¯¥å‡½æ•°å°†å½“å‰æ‘„åƒå¤´ç¼“å†²åŒºä¸­çš„å›¾åƒç¼–ç ä¸ºJPEGæ ¼å¼ï¼Œå¹¶é€šè¿‡HTTP POSTè¯·æ±‚
 * ä»¥multipart/form-dataçš„å½¢å¼å‘é€åˆ°æŒ‡å®šçš„è§£é‡ŠæœåŠ¡å™¨ã€‚æœåŠ¡å™¨å°†æ ¹æ®æä¾›çš„
 * é—®é¢˜å¯¹å›¾åƒè¿›è¡ŒAIåˆ†æžå¹¶è¿”å›žç»“æžœã€‚
 *
 * å®žçŽ°ç‰¹ç‚¹ï¼š
 * - ä½¿ç”¨ç‹¬ç«‹çº¿ç¨‹ç¼–ç JPEGï¼Œä¸Žä¸»çº¿ç¨‹åˆ†ç¦»
 * - é‡‡ç”¨åˆ†å—ä¼ è¾“ç¼–ç (chunked transfer encoding)ä¼˜åŒ–å†…å­˜ä½¿ç”¨
 * - é€šè¿‡é˜Ÿåˆ—æœºåˆ¶å®žçŽ°ç¼–ç çº¿ç¨‹å’Œå‘é€çº¿ç¨‹çš„æ•°æ®åŒæ­¥
 * - æ”¯æŒè®¾å¤‡IDã€å®¢æˆ·ç«¯IDå’Œè®¤è¯ä»¤ç‰Œçš„HTTPå¤´éƒ¨é…ç½®
 *
 * @param question è¦å‘AIæå‡ºçš„å…³äºŽå›¾åƒçš„é—®é¢˜ï¼Œå°†ä½œä¸ºè¡¨å•å­—æ®µå‘é€
 * @return std::string æœåŠ¡å™¨è¿”å›žçš„JSONæ ¼å¼å“åº”å­—ç¬¦ä¸²
 *         æˆåŠŸæ—¶åŒ…å«AIåˆ†æžç»“æžœï¼Œå¤±è´¥æ—¶åŒ…å«é”™è¯¯ä¿¡æ¯
 *         æ ¼å¼ç¤ºä¾‹ï¼š{"success": true, "result": "åˆ†æžç»“æžœ"}
 *                  {"success": false, "message": "é”™è¯¯ä¿¡æ¯"}
 *
 * @note è°ƒç”¨æ­¤å‡½æ•°å‰å¿…é¡»å…ˆè°ƒç”¨SetExplainUrl()è®¾ç½®æœåŠ¡å™¨URL
 * @note å‡½æ•°ä¼šç­‰å¾…ä¹‹å‰çš„ç¼–ç çº¿ç¨‹å®ŒæˆåŽå†å¼€å§‹æ–°çš„å¤„ç†
 * @warning å¦‚æžœæ‘„åƒå¤´ç¼“å†²åŒºä¸ºç©ºæˆ–ç½‘ç»œè¿žæŽ¥å¤±è´¥ï¼Œå°†è¿”å›žé”™è¯¯ä¿¡æ¯
 */
std::string Esp32Camera::Explain(const std::string& question) {
    if (explain_url_.empty()) {
        throw std::runtime_error("Image explain URL or token is not set");
    }

    // åˆ›å»ºå±€éƒ¨çš„ JPEG é˜Ÿåˆ—, 40 entries is about to store 512 * 40 = 20480 bytes of JPEG data
    QueueHandle_t jpeg_queue = xQueueCreate(40, sizeof(JpegChunk));
    if (jpeg_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create JPEG queue");
        throw std::runtime_error("Failed to create JPEG queue");
    }

    // We spawn a thread to encode the image to JPEG using optimized encoder (cost about 500ms and 8KB SRAM)
    encoder_thread_ = std::thread([this, jpeg_queue]() {
        uint16_t w = frame_.width ? frame_.width : 320;
        uint16_t h = frame_.height ? frame_.height : 240;
        v4l2_pix_fmt_t enc_fmt = frame_.format;
        image_to_jpeg_cb(
            frame_.data, frame_.len, w, h, enc_fmt, 80,
            [](void* arg, size_t index, const void* data, size_t len) -> size_t {
                auto jpeg_queue = (QueueHandle_t)arg;
                JpegChunk chunk = {.data = (uint8_t*)heap_caps_aligned_alloc(16, len, MALLOC_CAP_SPIRAM), .len = len};
                memcpy(chunk.data, data, len);
                xQueueSend(jpeg_queue, &chunk, portMAX_DELAY);
                return len;
            },
            jpeg_queue);
    });

    auto network = Board::GetInstance().GetNetwork();
    auto http = network->CreateHttp(3);
    // æž„é€ multipart/form-dataè¯·æ±‚ä½“
    std::string boundary = "----ESP32_CAMERA_BOUNDARY";

    // é…ç½®HTTPå®¢æˆ·ç«¯ï¼Œä½¿ç”¨åˆ†å—ä¼ è¾“ç¼–ç 
    http->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    http->SetHeader("Client-Id", Board::GetInstance().GetUuid().c_str());
    if (!explain_token_.empty()) {
        http->SetHeader("Authorization", "Bearer " + explain_token_);
    }
    http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http->SetHeader("Transfer-Encoding", "chunked");
    if (!http->Open("POST", explain_url_)) {
        ESP_LOGE(TAG, "Failed to connect to explain URL");
        // Clear the queue
        encoder_thread_.join();
        JpegChunk chunk;
        while (xQueueReceive(jpeg_queue, &chunk, portMAX_DELAY) == pdPASS) {
            if (chunk.data != nullptr) {
                heap_caps_free(chunk.data);
            } else {
                break;
            }
        }
        vQueueDelete(jpeg_queue);
        throw std::runtime_error("Failed to connect to explain URL");
    }

    {
        // ç¬¬ä¸€å—ï¼šquestionå­—æ®µ
        std::string question_field;
        question_field += "--" + boundary + "\r\n";
        question_field += "Content-Disposition: form-data; name=\"question\"\r\n";
        question_field += "\r\n";
        question_field += question + "\r\n";
        http->Write(question_field.c_str(), question_field.size());
    }
    {
        // ç¬¬äºŒå—ï¼šæ–‡ä»¶å­—æ®µå¤´éƒ¨
        std::string file_header;
        file_header += "--" + boundary + "\r\n";
        file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"camera.jpg\"\r\n";
        file_header += "Content-Type: image/jpeg\r\n";
        file_header += "\r\n";
        http->Write(file_header.c_str(), file_header.size());
    }

    // ç¬¬ä¸‰å—ï¼šJPEGæ•°æ®
    size_t total_sent = 0;
    while (true) {
        JpegChunk chunk;
        if (xQueueReceive(jpeg_queue, &chunk, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to receive JPEG chunk");
            break;
        }
        if (chunk.data == nullptr) {
            break;  // The last chunk
        }
        http->Write((const char*)chunk.data, chunk.len);
        total_sent += chunk.len;
        heap_caps_free(chunk.data);
    }
    // Wait for the encoder thread to finish
    encoder_thread_.join();
    // æ¸…ç†é˜Ÿåˆ—
    vQueueDelete(jpeg_queue);

    {
        // ç¬¬å››å—ï¼šmultipartå°¾éƒ¨
        std::string multipart_footer;
        multipart_footer += "\r\n--" + boundary + "--\r\n";
        http->Write(multipart_footer.c_str(), multipart_footer.size());
    }
    // ç»“æŸå—
    http->Write("", 0);

    if (http->GetStatusCode() != 200) {
        ESP_LOGE(TAG, "Failed to upload photo, status code: %d", http->GetStatusCode());
        throw std::runtime_error("Failed to upload photo");
    }

    std::string result = http->ReadAll();
    http->Close();

    // Get remain task stack size
    size_t remain_stack_size = uxTaskGetStackHighWaterMark(nullptr);
    ESP_LOGI(TAG, "Explain image size=%d bytes, compressed size=%d, remain stack size=%d, question=%s\n%s",
             (int)frame_.len, (int)total_sent, (int)remain_stack_size, question.c_str(), result.c_str());
    return result;
}

bool Esp32Camera::GetFrame(uint8_t** data, size_t* len, uint16_t* width, uint16_t* height, uint32_t* format) {
    if (!frame_.data || frame_.len == 0) {
        return false;
    }
    *data = frame_.data;
    *len = frame_.len;
    *width = frame_.width;
    *height = frame_.height;
    *format = (uint32_t)frame_.format;
    return true;
}

// ============================================================================
// Streaming pipeline methods (double-buffer)
// ============================================================================

bool Esp32Camera::CaptureInto(FrameBuffer& buf, bool show_preview) {
    if (!streaming_on_ || video_fd_ < 0) {
        return false;
    }

    struct v4l2_buffer buf_info = {};
    buf_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_info.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd_, VIDIOC_DQBUF, &buf_info) != 0) {
        ESP_LOGE(TAG, "CaptureAsync: VIDIOC_DQBUF failed");
        return false;
    }

    // Keep stream size locked unless explicitly changed by the server.
#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
    uint16_t out_w = buf.width ? buf.height : sensor_height_;
    uint16_t out_h = buf.height ? buf.width : sensor_width_;
#else
    uint16_t out_w = buf.width ? buf.width : sensor_width_;
    uint16_t out_h = buf.height ? buf.height : sensor_height_;
#endif

    // Allocate buffer if needed
    size_t bytes_per_pixel = 2;
    if (sensor_format_ == V4L2_PIX_FMT_GREY) {
        bytes_per_pixel = 1;
    } else if (sensor_format_ == V4L2_PIX_FMT_RGB24) {
        bytes_per_pixel = 3;
    }
    size_t needed_len = (sensor_format_ == V4L2_PIX_FMT_JPEG || sensor_format_ == V4L2_PIX_FMT_MJPEG)
                            ? mmap_buffers_[buf_info.index].length
                            : (size_t)out_w * (size_t)out_h * bytes_per_pixel;
    if (!buf.data || buf.len != needed_len) {
        if (buf.data) {
            heap_caps_free(buf.data);
        }
        buf.len = needed_len;
        buf.data = (uint8_t*)heap_caps_malloc(buf.len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (!buf.data) {
        ESP_LOGE(TAG, "CaptureAsync: alloc frame copy failed");
        if (ioctl(video_fd_, VIDIOC_QBUF, &buf_info) != 0) {
            ESP_LOGE(TAG, "CaptureAsync: VIDIOC_QBUF cleanup failed");
        }
        return false;
    }

    buf.width = out_w;
    buf.height = out_h;

    ESP_LOGD(TAG, "CaptureAsync: mmap len=%d, out=%dx%d",
             mmap_buffers_[buf_info.index].length, out_w, out_h);

    bool needs_downscale = (sensor_width_ != out_w || sensor_height_ != out_h);

    switch (sensor_format_) {
        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
            buf.len = buf_info.bytesused;
            memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_RGB565:
            if (needs_downscale) {
                downscale_rgb565((const uint8_t*)mmap_buffers_[buf_info.index].start, sensor_width_, sensor_height_,
                                 buf.data, out_w, out_h);
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP
            { auto dst16 = (uint16_t*)buf.data; size_t count = buf.len / 2; for (size_t i = 0; i < count; i++) dst16[i] = __builtin_bswap16(dst16[i]); }
#endif
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_RGB24:
            if (needs_downscale) {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, MIN(mmap_buffers_[buf_info.index].length, buf.len));
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_YUYV:
            if (needs_downscale) {
                downscale_yuyv((const uint8_t*)mmap_buffers_[buf_info.index].start, sensor_width_, sensor_height_,
                               buf.data, out_w, out_h);
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
#ifdef CONFIG_XIAOZHI_ENABLE_CAMERA_ENDIANNESS_SWAP
            { auto dst16 = (uint16_t*)buf.data; size_t count = buf.len / 2; for (size_t i = 0; i < count; i++) dst16[i] = __builtin_bswap16(dst16[i]); }
#endif
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_YUV420:
            if (needs_downscale) {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, MIN(mmap_buffers_[buf_info.index].length, buf.len));
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_GREY:
            if (needs_downscale) {
                downscale_grey((const uint8_t*)mmap_buffers_[buf_info.index].start, sensor_width_, sensor_height_,
                               buf.data, out_w, out_h);
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
            buf.format = sensor_format_;
            break;
        case V4L2_PIX_FMT_YUV422P:
            if (needs_downscale) {
                downscale_yuv422p((const uint8_t*)mmap_buffers_[buf_info.index].start, sensor_width_, sensor_height_,
                                 buf.data, out_w, out_h);
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
            buf.format = V4L2_PIX_FMT_YUV422P;
            break;
        case V4L2_PIX_FMT_RGB565X:
            if (needs_downscale) {
                downscale_rgb565((const uint8_t*)mmap_buffers_[buf_info.index].start, sensor_width_, sensor_height_,
                                 buf.data, out_w, out_h);
            } else {
                memcpy(buf.data, mmap_buffers_[buf_info.index].start, buf.len);
            }
            { auto dst16 = (uint16_t*)buf.data; size_t count = buf.len / 2; for (size_t i = 0; i < count; i++) dst16[i] = __builtin_bswap16(dst16[i]); }
            buf.format = V4L2_PIX_FMT_RGB565;
            break;
        default:
            ESP_LOGE(TAG, "CaptureAsync: unsupported sensor format: 0x%08x", sensor_format_);
            if (ioctl(video_fd_, VIDIOC_QBUF, &buf_info) != 0) {
                ESP_LOGE(TAG, "CaptureAsync: VIDIOC_QBUF cleanup failed");
            }
            return false;
    }

    // Return buffer to driver
    if (ioctl(video_fd_, VIDIOC_QBUF, &buf_info) != 0) {
        ESP_LOGE(TAG, "CaptureAsync: VIDIOC_QBUF failed");
    }

    // Show preview on display if requested (for backward-compatible Capture)
    if (show_preview) {
        auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
        if (display != nullptr && buf.data != nullptr) {
            uint16_t w = buf.width;
            uint16_t h = buf.height;
            size_t lvgl_image_size = buf.len;
            size_t stride = ((w * 2) + 3) & ~3;
            lv_color_format_t color_format = LV_COLOR_FORMAT_RGB565;
            uint8_t* data = nullptr;

            switch (buf.format) {
                case V4L2_PIX_FMT_YUYV:
                case V4L2_PIX_FMT_YUV420:
                case V4L2_PIX_FMT_RGB24: {
                    color_format = LV_COLOR_FORMAT_RGB565;
                    data = (uint8_t*)heap_caps_malloc(w * h * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (data == nullptr) {
                        ESP_LOGE(TAG, "Failed to allocate memory for preview image");
                        return true;  // Frame captured, just preview failed
                    }
                    esp_imgfx_color_convert_cfg_t convert_cfg = {
                        .in_res = {.width = static_cast<int16_t>(w), .height = static_cast<int16_t>(h)},
                        .in_pixel_fmt = static_cast<esp_imgfx_pixel_fmt_t>(buf.format),
                        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
                        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601,
                    };
                    esp_imgfx_color_convert_handle_t convert_handle = nullptr;
                    esp_imgfx_err_t err = esp_imgfx_color_convert_open(&convert_cfg, &convert_handle);
                    if (err != ESP_IMGFX_ERR_OK || convert_handle == nullptr) {
                        heap_caps_free(data);
                        return true;
                    }
                    esp_imgfx_data_t convert_input_data = {.data = buf.data, .data_len = buf.len};
                    esp_imgfx_data_t convert_output_data = {.data = data, .data_len = static_cast<uint32_t>(w * h * 2)};
                    err = esp_imgfx_color_convert_process(convert_handle, &convert_input_data, &convert_output_data);
                    esp_imgfx_color_convert_close(convert_handle);
                    if (err != ESP_IMGFX_ERR_OK) {
                        heap_caps_free(data);
                        return true;
                    }
                    lvgl_image_size = w * h * 2;
                    break;
                }
                case V4L2_PIX_FMT_RGB565:
                    data = (uint8_t*)heap_caps_malloc(w * h * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (data == nullptr) {
                        return true;
                    }
                    memcpy(data, buf.data, buf.len);
                    lvgl_image_size = buf.len;
                    break;
                default:
                    return true;
            }
            auto image = std::make_unique<LvglAllocatedImage>(data, lvgl_image_size, w, h, stride, color_format);
            display->SetPreviewImage(std::move(image));
        }
    }
    return true;
}

bool Esp32Camera::CaptureAsync(bool show_preview) {
    if (!streaming_on_ || video_fd_ < 0) {
        return false;
    }

    int cap_idx = capture_index_.load();
    FrameBuffer& buf = stream_buffers_[cap_idx];

    // Initialize width/height if first time
    if (buf.width == 0 || buf.height == 0) {
#ifdef CONFIG_XIAOZHI_ENABLE_ROTATE_CAMERA_IMAGE
        if (frame_.width > 0 && frame_.height > 0) {
            buf.width = frame_.width;
            buf.height = frame_.height;
        } else {
            buf.width = sensor_height_;
            buf.height = sensor_width_;
        }
#else
        if (frame_.width > 0 && frame_.height > 0) {
            buf.width = frame_.width;
            buf.height = frame_.height;
        } else {
            buf.width = sensor_width_;
            buf.height = sensor_height_;
        }
#endif
    }

    bool ok = CaptureInto(buf, show_preview);
    if (!ok) {
        return false;
    }

    // Swap: capture_index_ becomes ready, the old ready becomes next capture target
    int old_ready = ready_index_.load();
    capture_index_.store(old_ready);
    ready_index_.store(cap_idx);

    // Signal that a new frame is ready for encoding
    if (capture_sem_) {
        xSemaphoreGive(capture_sem_);
    }

    return true;
}

void Esp32Camera::SetStreamFrameSize(uint16_t width, uint16_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    // Reconfigure only between frames; preserve current capture buffers until the next capture cycle.
    for (auto& buf : stream_buffers_) {
        if (buf.width == width && buf.height == height) {
            continue;
        }
        if (buf.data) {
            heap_caps_free(buf.data);
            buf.data = nullptr;
        }
        buf.len = 0;
        buf.width = width;
        buf.height = height;
        buf.format = 0;
    }
    ESP_LOGI(TAG, "Stream frame size set to %ux%u", width, height);
}

bool Esp32Camera::GetReadyFrame(uint8_t** data, size_t* len, uint16_t* width, uint16_t* height, uint32_t* format) {
    int idx = ready_index_.load();
    FrameBuffer& buf = stream_buffers_[idx];
    if (!buf.data || buf.len == 0) {
        return false;
    }
    *data = buf.data;
    *len = buf.len;
    *width = buf.width;
    *height = buf.height;
    *format = (uint32_t)buf.format;
    return true;
}


