/* videocapture is a tool with no special purpose
 *
 * Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef CAPTURE_DEVICE_HPP
#define CAPTURE_DEVICE_HPP

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
};


/**
 * @note
 *    changes to most of the settings will take effect when newly initializing the
 *    capture device
 */
class CaptureDevice
{
public:

    CaptureDevice();
    /** @todo */
    CaptureDevice(const CaptureDevice&) = delete;
    /** @todo */
    CaptureDevice(CaptureDevice&&) = delete;
    /** @todo */
    CaptureDevice &operator=(const CaptureDevice&) = delete;
    /** @todo */
    CaptureDevice &operator=(CaptureDevice&&) = delete;
    ~CaptureDevice();

    /** set programatically (approx width*height*byteperpixel) */
    unsigned int bufferSize() const;

    /** @note possibly changed during initialization by the device */
    void setCaptureSize(unsigned int width, unsigned int height);
    std::pair<unsigned int, unsigned int> captureSize() const;

    void setFileName(const std::string&);
    const std::string &fileName() const;

    /** number of buffers in the ring used for storing images. Default: 2 */
    void setBufferCount(unsigned int);
    unsigned int bufferCount() const;

    /**
     * @pre captureSize() has to be set
     * @pre fileName() has to be set
     * @returns true on success, false on failure
     *
     * @note on failure, finish() is called implicitly
     */
    bool init();

    void finish();


    void printDeviceInfo();
    void printControls();
    void printFormats();
    void printTimerInformation() const;

    
    struct Buffer
    {
        timespec time;
        /** if 0 -> writeable, readable; if > 0 -> readable */
        int readerCount;
        unsigned char *buffer;
    };

    /** n has to be less than  'buffersCount' */
    std::deque<const Buffer*> lockFirstNBuffers(unsigned int n);
    void unlock(const std::deque<const Buffer*> &buffers);
    /** @returns number of newer buffers
        @note
        When actually locking the buffer this number might differ due to threading.
        It can be larger, or it can be n-1, when previously n */
    unsigned int newerBuffersAvailable(const timespec &newerThan);

    /** @returns average period for capturing an image and the standard deviation
        @note blocks for several seconds */
    std::pair<double, double> determineCapturePeriod(double secondsToIterate = 5.0);

    void startCapturing();
    void stopCapturing();
    bool isCapturing() const;

    void pauseCapturing(bool pause);
    bool isCapturingPaused() const;

    /** @returns all controls and control menu items, which the capture device provides
        @see http://www.linuxtv.org/downloads/video4linux/API/V4L2_API/spec-single/v4l2.html#V4L2-QUERYCTRL
        @see http://www.linuxtv.org/downloads/video4linux/API/V4L2_API/spec-single/v4l2.html#V4L2-QUERYMENU */
    std::pair<std::list<struct v4l2_queryctrl>, std::list<struct v4l2_querymenu> > controls();

    /** @returns true if the query succeeded - more sophisticated error checking to come
        @see http://www.linuxtv.org/downloads/video4linux/API/V4L2_API/spec-single/v4l2.html#V4L2-CONTROL */
    bool control(struct v4l2_control&);
    /** @returns true if the call succeeded - more sophisticated error checking to come */
    bool setControl(const struct v4l2_control&);

private:

    bool queryControl(struct v4l2_queryctrl&);
    std::list<struct v4l2_querymenu> menus(const struct v4l2_queryctrl&);

    static void captureThread(CaptureDevice *camera);
    static void determineCapturePeriodThread(double, CaptureDevice*,
            std::pair<double,double>*);

    int xv4l2_ioctl(int fileDescriptor, int request, void *arg);

    static std::string pixelFormatString(__u32 pixelFormat);


    unsigned int m_captureHeight;
    unsigned int m_captureWidth;
    std::string m_fileName;
    unsigned int m_bufferCount;

    int m_fileDescriptor;
    unsigned int m_bufferSize;
    std::list<Buffer> m_buffers;
    std::deque<Buffer*> m_timelySortedBuffers;
    std::mutex m_timelySortedBuffersMutex;

    struct timespec m_timerResolution;
    struct timespec m_timerStart;
    struct timeval m_realStartTime;

    std::thread *m_captureThread;
    bool m_captureThreadCancellationFlag;

    std::mutex m_fileAccessMutex;
    std::mutex m_pauseCapturingMutex;
    bool m_capturingPaused;
};


#endif /* CAPTURE_DEVICE_HPP */

