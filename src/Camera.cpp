#include "Camera.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using std::cerr;
using std::endl;
using std::make_pair;
using std::pair;
using std::string;


Camera::Camera() :
        m_fileName(),
        m_fileDescriptor(-1),
        m_height(0),
        m_width(0),
        m_pixelFormat(V4L2_PIX_FMT_JPEG),
        m_fieldFormat(V4L2_FIELD_NONE),
        m_readTimeOut(2),
        m_bufferOne(0),
        m_bufferTwo(0)
{
    // cerr << __PRETTY_FUNCTION__ << endl;
}


Camera::~Camera()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    assert(m_fileDescriptor == -1);
    assert(m_bufferTwo == 0);
    assert(m_bufferOne== 0);
}


void Camera::setFileName(const string& fileName)
{
    m_fileName = fileName;
}
const string& Camera::fileName() const
{
    return m_fileName;
}
void Camera::setCaptureSize(unsigned int width, unsigned int height)
{
    m_width = width;
    m_height = height;
}
pair<unsigned int, unsigned int> Camera::captureSize() const
{
    return make_pair(m_width, m_height);
}
__u32 Camera::pixelFormat() const
{
    return m_pixelFormat;
}
string Camera::pixelFormatString() const
{
    string ret;
    ret.push_back((char) ((m_pixelFormat & 0x000000ff) >> 0));
    ret.push_back((char) ((m_pixelFormat & 0x0000ff00) >> 8));
    ret.push_back((char) ((m_pixelFormat & 0x00ff0000) >> 16));
    ret.push_back((char) ((m_pixelFormat & 0xff000000) >> 24));
    return  ret;
}
enum v4l2_field Camera::fieldFormat() const
{
    return m_fieldFormat;
}
unsigned int Camera::bufferSize() const
{
    return m_bufferSize;
}
void Camera::setReadTimeOut(unsigned int seconds)
{
    m_readTimeOut = seconds;
}
unsigned int Camera::readTimeOut() const
{
    return m_readTimeOut;
}


void Camera::init()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    
    assert(m_fileDescriptor == -1);
    assert(m_bufferTwo == 0);
    assert(m_bufferOne== 0);
    assert(m_fileName != string());

    /* *** open the device file *** */
    struct stat st;

    if (stat(m_fileName.c_str(), &st) == -1) {
        cerr << __PRETTY_FUNCTION__ << "Cannot identify file. " << errno << strerror(errno) << endl;
    }

    if (!S_ISCHR (st.st_mode)) {
        cerr << "File is no device." << endl;
    }

    m_fileDescriptor = open(m_fileName.c_str(), O_RDWR|O_NONBLOCK, 0);

    if (m_fileDescriptor == -1) {
        cerr << "Cannot open file. " << errno << strerror (errno) << endl;
    }

   
    /* *** initialize capturing *** */
    struct v4l2_capability cap;

    if (xioctl(m_fileDescriptor, VIDIOC_QUERYCAP, &cap) == -1) {
        if (EINVAL == errno) {
            cerr << "File is no V4L2 device." << endl;
            finish(); return;
        } else {
            cerr << __PRETTY_FUNCTION__ << " VIDIOC_QUERYCAP " << errno << strerror(errno) << endl;
            finish(); return;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        cerr << "File is no video capture device." << endl;
        finish(); return;
    }

    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
        cerr << "File does not support read i/o." << endl;
        finish(); return;
    }


    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    memset(&cropcap, 0, sizeof(v4l2_cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(m_fileDescriptor, VIDIOC_CROPCAP, &cropcap);

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;
    xioctl(m_fileDescriptor, VIDIOC_S_CROP, &crop); /* ignore errors */


    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = m_width;
    fmt.fmt.pix.height = m_height;
    fmt.fmt.pix.pixelformat = m_pixelFormat;
    fmt.fmt.pix.field = m_fieldFormat;

    if (xioctl(m_fileDescriptor, VIDIOC_S_FMT, &fmt) == -1) {
        cerr << __PRETTY_FUNCTION__ << " VIDIOC_S_FMT " << errno << strerror(errno) << endl;
        finish(); return;
    }

    if (fmt.fmt.pix.width != m_width || fmt.fmt.pix.height != m_height ||
            fmt.fmt.pix.pixelformat != m_pixelFormat ||
            fmt.fmt.pix.field != m_fieldFormat) {

        cerr << "Your parameters were changed: "
                << m_width << "x" << m_height << " in "
                << pixelFormatString() << ", " << m_fieldFormat << " -> ";

        m_width = fmt.fmt.pix.width;
        m_height = fmt.fmt.pix.height;
        m_pixelFormat = fmt.fmt.pix.pixelformat;
        m_fieldFormat = fmt.fmt.pix.field;

        cerr << m_width << "x" << m_height << " in "
                << pixelFormatString() << ", " << m_fieldFormat << endl;
    }


    /* Buggy driver paranoia. */
    unsigned int min;
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    m_bufferSize = fmt.fmt.pix.sizeimage;

    /* *** allocate buffers *** */
    m_bufferOne = (unsigned char*) malloc(sizeof(unsigned char) * m_bufferSize);
    m_bufferTwo = (unsigned char*) malloc(sizeof(unsigned char) * m_bufferSize);
    assert(m_bufferOne != 0 && m_bufferTwo != 0);
}


void Camera::finish()
{
    assert(m_fileDescriptor != -1);
    if (close(m_fileDescriptor) == -1) {
        cerr << __PRETTY_FUNCTION__ << " " << errno << " " << strerror(errno) << endl;
    }
    m_fileDescriptor = -1;

    assert(m_bufferOne != 0);
    free(m_bufferOne);
    m_bufferOne = 0;

    assert(m_bufferTwo != 0);
    free(m_bufferTwo);
    m_bufferTwo = 0;
}


void Camera::enumerateControls() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


void Camera::enumerateFormats() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


void Camera::readNewImage()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


void Camera::lockBuffer()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


const unsigned char *Camera::buffer() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
    return 0;
}


void Camera::unlockBuffer()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


void Camera::waitForNewImage()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


int Camera::xioctl(int fileDescriptor, int request, void *arg)
{
    int r;

    do r = ioctl (fileDescriptor, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

