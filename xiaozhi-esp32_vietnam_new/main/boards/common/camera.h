#ifndef CAMERA_H
#define CAMERA_H

#include <string>

class Camera {
public:
    virtual void SetExplainUrl(const std::string& url, const std::string& token) = 0;
    virtual bool Capture() = 0;
    virtual bool SetHMirror(bool enabled) = 0;
    virtual bool SetVFlip(bool enabled) = 0;
    virtual std::string Explain(const std::string& question) = 0;
    virtual bool GetFrame(uint8_t** data, size_t* len, uint16_t* width, uint16_t* height, uint32_t* format) {
        return false;
    }
};

#endif // CAMERA_H
