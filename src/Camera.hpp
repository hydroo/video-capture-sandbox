#ifndef CAMERA_HPP
#define CAMERA_HPP


#include "Prereqs.hpp"

#include <ctime>
#include <deque>
#include <linux/videodev2.h>
#include <list>
#include <mutex>
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

    Camera();
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


    void init(unsigned int bufferCount = 2);
    void finish();

    void printDeviceInfo() const;
    void printControls() const;
    void printFormats() const;
    void printTimerInformation() const;

    struct Buffer
    {
        timespec time;
        /** if 0 -> writeable, readable; if > 0 -> readable */
        unsigned int readerCount;
        unsigned char *buffer;
    };

    /** n <= bufferCount-1 */
    std::deque<const Buffer*> lockFirstNBuffers(unsigned int n);
    void unlock(const std::deque<const Buffer*>& buffers);
    /** @returns number of newer buffers.
        when actually locking the buffer this number might defer.
        It can be larger, or it can be n-1, when previously n ~.~ */
    unsigned int newerBuffersAvailable(const timespec& newerThan);

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


    unsigned int m_bufferSize;
    std::list<Buffer> m_buffers;
    std::deque<Buffer*> m_timelySortedBuffers;
    std::mutex m_timelySortedBuffersMutex;

    clockid_t m_timerClockId;
    struct timespec m_timerResolution;
    struct timespec m_timerStart;
    struct timeval m_realStartTime;

    std::thread *m_captureThread;
    bool m_captureThreadCancellationFlag;
};


#endif /* CAMERA_HPP */

