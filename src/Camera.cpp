#include "Camera.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>


using namespace std;


static void determineCapturePeriodThread(Camera*, pair<double,double>&);
static void captureThread(Camera* camera);


/** helper, which calls ioctl until an undisturbed call has been done */
static int xioctl(int fileDescriptor, int request, void *arg);


Camera::Camera(unsigned int ringBufferCount) :
        m_fileName(),
        m_fileDescriptor(-1),
        m_height(0),
        m_width(0),
        m_pixelFormat(V4L2_PIX_FMT_JPEG),
        m_fieldFormat(V4L2_FIELD_NONE),
        m_readTimeOut(2),
        m_ringBuffer(0),
        m_ringBufferCount(ringBufferCount),
        m_ringBufferSize(0),
        m_timerClockId(CLOCK_REALTIME)
{
    // cerr << __PRETTY_FUNCTION__ << endl;
}


Camera::~Camera()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    assert(m_fileDescriptor == -1);
    assert(m_ringBuffer == 0);
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
    return m_ringBufferSize;
}
void Camera::setReadTimeOut(unsigned int seconds)
{
    m_readTimeOut = seconds;
}
unsigned int Camera::readTimeOut() const
{
    return m_readTimeOut;
}
void Camera::setClockId(clockid_t id)
{
    m_timerClockId = id;
}
clockid_t Camera::clockId() const
{
    return m_timerClockId;
}


void Camera::init()
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    
    assert(m_fileDescriptor == -1);
    assert(m_ringBuffer == 0);
    assert(m_fileName != string());


    /* *** initialize timer *** */
    clock_gettime(m_timerClockId, &m_timerStart);
    clock_getres(m_timerClockId, &m_timerResolution);
    gettimeofday(&m_realStartTime, 0);


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

    m_ringBufferSize = fmt.fmt.pix.sizeimage;

    /* *** allocate buffers *** */
    m_ringBuffer = (unsigned char**) malloc(sizeof(unsigned char*)*m_ringBufferCount);
    assert(m_ringBuffer != 0);

    for (unsigned int a=0; a < m_ringBufferCount; ++a) {
        m_ringBuffer[a] = (unsigned char*) malloc(sizeof(unsigned char)*m_ringBufferSize);
        assert(m_ringBuffer[a] != 0);
    }
}


void Camera::finish()
{
    if (m_fileDescriptor != -1) {
        if (close(m_fileDescriptor) == -1) {
            cerr << __PRETTY_FUNCTION__ << " " << errno << " " << strerror(errno) << endl;
        }
        m_fileDescriptor = -1;
    }


    if (m_ringBuffer != 0) {
        for(unsigned int a=0; a < m_ringBufferCount; ++a) {
            assert(m_ringBuffer[a] != 0);
            free (m_ringBuffer[a]); m_ringBuffer[a] = 0;
        }
        free(m_ringBuffer); m_ringBuffer = 0;
    }
    m_ringBufferSize = 0;
}


void Camera::printDeviceInfo() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    assert(m_fileDescriptor != -1);

	struct v4l2_capability cap;
	/* check capabilities */
	if (xioctl(m_fileDescriptor, VIDIOC_QUERYCAP, &cap) == -1) {
        if (EINVAL == errno) {
            cerr << "Device is no V4L2 device." << endl;
            assert(0);
        } else {
            cerr << __PRETTY_FUNCTION__ << " VIDIOC_QUERYCAP " << errno << strerror(errno) << endl;
        }
	}

    cout << "Device info:" << endl
            << "  driver: " << cap.driver << endl
            << "  card: " << cap.card << endl
            << "  bus info: " << cap.bus_info << endl
            << "  version: " << cap.version << endl;

    cout << "  supports: ";

	if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        cout << "capture, ";
    }

	if (cap.capabilities & V4L2_CAP_STREAMING) {
        cout << "streaming";
    }
    cout << endl;
}


void Camera::printControls() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    assert(m_fileDescriptor != -1);

    struct v4l2_queryctrl ctrl;
    memset (&ctrl, 0, sizeof(v4l2_queryctrl ));

    cout << "Available Controls:" << endl;
    for (__u32 id = V4L2_CID_BASE; id < V4L2_CID_LASTP1; id++) {
        queryControl(id);
    }

    cout << "Available Private Controls:" << endl;
    for (__u32 id = V4L2_CID_PRIVATE_BASE;; ++id) {
        /* an invalid id means we are beyond fence */
        if (queryControl(id) == true) break;
    }
}


void Camera::printFormats() const
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    assert(m_fileDescriptor != -1);

    struct FormatRecord
    {
        unsigned int id;
        const char *name;
    };

    /* copied from the videodev2.h header */
    FormatRecord pixelFormats[] = {
        { V4L2_PIX_FMT_RGB332,   "V4L2_PIX_FMT_RGB332" },
        { V4L2_PIX_FMT_RGB555,   "V4L2_PIX_FMT_RGB555" },
        { V4L2_PIX_FMT_RGB565,   "V4L2_PIX_FMT_RGB565" },
        { V4L2_PIX_FMT_RGB555X,  "V4L2_PIX_FMT_RGB555X" },
        { V4L2_PIX_FMT_RGB565X,  "V4L2_PIX_FMT_RGB565X" },
        { V4L2_PIX_FMT_BGR24,    "V4L2_PIX_FMT_BGR24" },
        { V4L2_PIX_FMT_RGB24,    "V4L2_PIX_FMT_RGB24" },
        { V4L2_PIX_FMT_BGR32,    "V4L2_PIX_FMT_BGR32" },
        { V4L2_PIX_FMT_RGB32,    "V4L2_PIX_FMT_RGB32" },
        { V4L2_PIX_FMT_GREY,     "V4L2_PIX_FMT_GREY" },
        { V4L2_PIX_FMT_YVU410,   "V4L2_PIX_FMT_YVU410" },
        { V4L2_PIX_FMT_YVU420,   "V4L2_PIX_FMT_YVU420" },
        { V4L2_PIX_FMT_YUYV,     "V4L2_PIX_FMT_YUYV" },
        { V4L2_PIX_FMT_UYVY,     "V4L2_PIX_FMT_UYVY" },
        { V4L2_PIX_FMT_YUV422P,  "V4L2_PIX_FMT_YUV422P" },
        { V4L2_PIX_FMT_YUV411P,  "V4L2_PIX_FMT_YUV411P" },
        { V4L2_PIX_FMT_Y41P,     "V4L2_PIX_FMT_Y41P" },
        { V4L2_PIX_FMT_NV12,     "V4L2_PIX_FMT_NV12" },
        { V4L2_PIX_FMT_NV21,     "V4L2_PIX_FMT_NV21" },
        { V4L2_PIX_FMT_YUV410,   "V4L2_PIX_FMT_YUV410" },
        { V4L2_PIX_FMT_YUV420,   "V4L2_PIX_FMT_YUV420" },
        { V4L2_PIX_FMT_YYUV,     "V4L2_PIX_FMT_YYUV" },
        { V4L2_PIX_FMT_HI240,    "V4L2_PIX_FMT_HI240" },
        { V4L2_PIX_FMT_HM12,     "V4L2_PIX_FMT_HM12" },
        { V4L2_PIX_FMT_RGB444,   "V4L2_PIX_FMT_RGB444" },
        { V4L2_PIX_FMT_SBGGR8,   "V4L2_PIX_FMT_SBGGR8" },
        { V4L2_PIX_FMT_MJPEG,    "V4L2_PIX_FMT_MJPEG" },
        { V4L2_PIX_FMT_JPEG,     "V4L2_PIX_FMT_JPEG" },
        { V4L2_PIX_FMT_DV,       "V4L2_PIX_FMT_DV" },
        { V4L2_PIX_FMT_MPEG,     "V4L2_PIX_FMT_MPEG" },
        { V4L2_PIX_FMT_WNVA,     "V4L2_PIX_FMT_WNVA" },
        { V4L2_PIX_FMT_SN9C10X,  "V4L2_PIX_FMT_SN9C10X" },
        { V4L2_PIX_FMT_PWC1,     "V4L2_PIX_FMT_PWC1" },
        { V4L2_PIX_FMT_PWC2,     "V4L2_PIX_FMT_PWC2" },
        { V4L2_PIX_FMT_ET61X251, "V4L2_PIX_FMT_ET61X251" }
    };

    int formatCount = sizeof(pixelFormats) / sizeof(FormatRecord);
    

    cout << "Supported Formats: " << endl;

	/* ask for a pixel format enumeration */
	int ioctlError = 0;
	int formatIndex = 0;

	while (ioctlError == 0) {

        struct v4l2_fmtdesc format;
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.index = formatIndex;

		ioctlError = ioctl (m_fileDescriptor, VIDIOC_ENUM_FMT, &format);

		if (ioctlError == 0) {
			for (int a = 0; a < formatCount; ++a) {
				if (format.pixelformat == pixelFormats[a].id) {
                    cout << "  " << format.description
                            << ((format.flags & V4L2_FMT_FLAG_COMPRESSED) ? " compressed" : " raw")
                            << " \"" << pixelFormats[a].name << "\"" << endl;
					break;
				}
			}
		}

        ++formatIndex;
	}
}


void Camera::printTimerInformation() const
{
    struct tm *localTime = localtime(&m_realStartTime.tv_sec);

    cout << "Start Time: "
            << setw(2) << setfill('0') << localTime->tm_year-100
            << setw(2) << setfill('0') << localTime->tm_mon+1
            << setw(2) << setfill('0') << localTime->tm_mday
            << " "
            << setw(2) << setfill('0') << localTime->tm_hour
            << ":"
            << setw(2) << setfill('0') << localTime->tm_min << endl;

    cout << "Timer Resolution: " << (double) m_timerResolution.tv_sec << "s "
            << m_timerResolution.tv_nsec << "nsec" << endl;
}


unsigned char *Camera::lockBufferForWriting()
{
    // cerr << __PRETTY_FUNCTION__ << endl;

    return 0;
}


unsigned char *Camera::lockBufferForReading()
{
    // cerr << __PRETTY_FUNCTION__ << endl;

    return 0;
}


void Camera::unlockBuffer(unsigned char *buffer)
{
    // cerr << __PRETTY_FUNCTION__ << endl;
    (void) buffer;
}


pair<double, double> Camera::determineCapturePeriod()
{
    pair<double, double> ret;


    thread t(bind(determineCapturePeriodThread, this, ret));
    t.join();
    
    return ret;
}


void Camera::startCapturing()
{
    thread t(bind(captureThread, this));
}


bool Camera::queryControl(__u32 id) const
{
    bool ret = false;
    struct v4l2_queryctrl ctrl;
    memset (&ctrl, 0, sizeof(v4l2_queryctrl ));
    ctrl.id = id;

    if (xioctl(m_fileDescriptor, VIDIOC_QUERYCTRL, &ctrl) == 0) {

        cout << "  " << ctrl.id - V4L2_CID_BASE << " \"" << ctrl.name << "\""
                << ((ctrl.flags & V4L2_CTRL_FLAG_DISABLED) ? " " : " not ") << "disabled,"
                << ((ctrl.flags & V4L2_CTRL_FLAG_GRABBED) ? " " : " not  ") << "grabbed,"
                << ((ctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) ? " " : " not ") << "readonly,"
                << ((ctrl.flags & V4L2_CTRL_FLAG_UPDATE) ? " " : " not ") << "update,"
                << ((ctrl.flags & V4L2_CTRL_FLAG_INACTIVE) ? " " : " not ") << "inactive,"
                << ((ctrl.flags & V4L2_CTRL_FLAG_SLIDER) ? " " : " not ") << "slider,";

        switch (ctrl.type) {
        case V4L2_CTRL_TYPE_INTEGER:
            cout << " integer type";
            break;
        case V4L2_CTRL_TYPE_BOOLEAN:
            cout << " boolean type";
            break;
        case V4L2_CTRL_TYPE_MENU:
            cout << " menu type";
            break;
        case V4L2_CTRL_TYPE_BUTTON:
            cout << " button type";
            break;
        case V4L2_CTRL_TYPE_INTEGER64:
            cout << " integer 64 type";
            break;
        case V4L2_CTRL_TYPE_CTRL_CLASS:
            cout << " control class type";
            break;
        default:
            assert(0);
        }

        cout << endl;

        if (!(ctrl.flags & V4L2_CTRL_FLAG_DISABLED) && ctrl.type == V4L2_CTRL_TYPE_MENU) {

            struct v4l2_querymenu menu;
            memset (&menu, 0, sizeof (v4l2_querymenu));
            menu.id = ctrl.id;

            for (menu.index = ctrl.minimum; ((__s32) menu.index) <= ctrl.maximum; menu.index++) {

                if (xioctl(m_fileDescriptor, VIDIOC_QUERYMENU, &menu) == 0) {
                    cout << "  " << menu.name << endl;
                } else {
                    cerr << __PRETTY_FUNCTION__ << " VIDIOC_QUERYMENU " << errno << strerror(errno) << endl;
                }
            }

        }

    } else if (errno != EINVAL) {
        cerr << __PRETTY_FUNCTION__ << " VIDIOC_QUERYCTRL " << errno << strerror(errno) << endl;
    } else if (errno == EINVAL) {
        ret = true;
    }

    return ret;
}


/* *** static functions ***************************************************** */
static void determineCapturePeriodThread(Camera* camera,
        pair<double,double>& ret)
{
    (void) camera;
    (void) ret;
    cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


static void captureThread(Camera* camera)
{
    (void) camera;
    cerr << __PRETTY_FUNCTION__ << endl;
    // TODO
}


int xioctl(int fileDescriptor, int request, void *arg)
{
    int r;

    do r = ioctl (fileDescriptor, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

