#ifndef CAMERA_HPP
#define CAMERA_HPP


#include "Prereqs.hpp"

#include <ctime>
#include <linux/videodev2.h>
#include <string>
#include <sys/time.h>
#include <utility>

namespace std
{
    class thread;
}


/**
 * @note
 *  * changes in the most settings take effect when newly initializing the
 *    instance
 */
class Camera
{
public:

    Camera(unsigned int ringBufferCount = 2);
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
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

    void setClockId(clockid_t);
    clockid_t clockId() const;


    void init();
    void finish();

    void printDeviceInfo() const;
    void printControls() const;
    void printFormats() const;
    void printTimerInformation() const;

    unsigned char *lockBufferForWriting();
    unsigned char *lockBufferForReading();
    void unlockBuffer(unsigned char *buffer);

    /**
     * blocks for several seconds
     *
     * @returns average period and standard deviation 
     */
    std::pair<double, double> determineCapturePeriod(double secondsToIterate = 5.0);

    /** creates the capture thread */
    void startCapturing();
    /** blocks until the capture thread is joined */
    void stopCapturing();

private:

    /** @returns wether the id was a valid one */
    bool queryControl(__u32 id) const;

    static void captureThread(Camera* camera);
    static void determineCapturePeriodThread(double, Camera*,
            std::pair<double,double>*);

    std::string m_fileName;
    int m_fileDescriptor;
    unsigned int m_captureHeight;
    unsigned int m_captureWidth;
    __u32 m_pixelFormat;
    enum v4l2_field m_fieldFormat;
    unsigned int m_readTimeOut;

    unsigned char **m_buffers;
    unsigned int m_buffersCount;
    unsigned int m_bufferSize;

    clockid_t m_timerClockId;
    struct timespec m_timerResolution;
    struct timespec m_timerStart;
    struct timeval m_realStartTime;

    std::thread *m_captureThread;
    bool m_captureThreadCancellationFlag;
};


#endif /* CAMERA_HPP */

