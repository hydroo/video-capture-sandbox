#ifndef CAMERA_HPP
#define CAMERA_HPP


#include "Prereqs.hpp"

#include <linux/videodev2.h>
#include <string>
#include <utility>


class Camera
{
public:
    Camera();
    ~Camera();

    void setFileName(const std::string& fileName);
    const std::string& fileName() const;

    void setCaptureSize(unsigned int width, unsigned int height);
    std::pair<unsigned int, unsigned int> captureSize() const;

    __u32 pixelFormat() const;
    std::string pixelFormatString() const;
    enum v4l2_field fieldFormat() const;
    unsigned int bufferSize() const;

    void setReadTimeOut(unsigned int seconds);
    unsigned int readTimeOut() const;

    void init();
    void finish();

    /** dont print, but return variables with the content */
    void enumerateControls() const;
    /** dont print, but return variables with the content */
    void enumerateFormats() const;

    /**
     * reads one new image from the input stream
     * blocks as long as there is no new image data available
     * returns after reading the new image fully into the internal buffer
     */
    void readNewImage();

    void lockBuffer();
    const unsigned char *buffer() const;
    void unlockBuffer();

    /** blocks until there is a new picture to draw */
    void waitForNewImage();

private:

    static int xioctl(int fileDescriptor, int request, void *arg);

    std::string m_fileName;
    int m_fileDescriptor;
    unsigned int m_height;
    unsigned int m_width;
    __u32 m_pixelFormat;
    enum v4l2_field m_fieldFormat;
    unsigned int m_readTimeOut;

    unsigned char *m_bufferOne;
    unsigned char *m_bufferTwo;
    size_t m_bufferSize;
};


#endif /* CAMERA_HPP */
