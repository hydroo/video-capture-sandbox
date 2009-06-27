#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>


#include <jpegsimdlib.h>


#include <QtGlobal>


#include <iostream>
using std::cerr;
using std::cout;
using std::endl;


const __u32 CAPTURE_SIZE_X = 320;
const __u32 CAPTURE_SIZE_Y = 240;
const __u32 PIXEL_FORMAT = V4L2_PIX_FMT_JPEG;
const enum v4l2_field FIELD_FORMAT = V4L2_FIELD_NONE;

static void enumerateControls(int deviceFileDescriptor, bool enumEverything = false);

static void errno_exit(const char *description);
static int xioctl(int fd, int request, void *arg);


/**
 * @param len number of read bytes
 */
void processImage(unsigned char *buffer, ssize_t len, __u32 maxX, __u32 maxY)
{
    Q_UNUSED(buffer);
    Q_UNUSED(len);
    Q_UNUSED(maxX);
    Q_UNUSED(maxY);

    static char a = 'a' - 1;
    char filename[128] = "";

    if (a == 'z') exit(0);

    ++a;

    sprintf(filename,"%c.jpg", a);

    FILE *fp = fopen(filename, "wb");
    if (!fp) exit(1);
    fwrite(buffer, len, sizeof(unsigned char), fp);
    fclose(fp);
}


void mainLoop(int fileDescriptor, unsigned char *buffer,
        unsigned int bufferSize, int captureSizeX, int captureSizeY)
{
    fd_set filedescriptorset;
    struct timeval tv;
    int sel;
    ssize_t readlen;

    for (;;) {

        FD_ZERO(&filedescriptorset);
        FD_SET(fileDescriptor, &filedescriptorset);
        tv.tv_sec = 2;
        tv.tv_usec = 0;


        /* watch the file handle for new readable data */
        sel = select(fileDescriptor + 1, &filedescriptorset, NULL, NULL, &tv);

        if (sel == -1 && errno != EINTR) {
            errno_exit("select");
        } else if (sel == 0) {
            cerr << "select timeout" << endl;
            assert(0);
        }

        
        /* read from the device */
        readlen = read(fileDescriptor, buffer, bufferSize);

        if (readlen == -1) {
            cerr << "\"" << "read error " << errno << " (" << strerror (errno) << ")" << endl;
            assert(0);
        }

        processImage(buffer, readlen, captureSizeX, captureSizeY);
    }
}


void stopCapturing()
{
}


void startCapturing()
{
}


int openDevice(const char *deviceName)
{
    struct stat st; 
    int filedescriptor;

    if (stat(deviceName, &st) == -1) {
        cerr << "Cannot identify \"" << deviceName << "\": " << errno << "(" << strerror(errno) << ")" << endl;
        filedescriptor = -1;
    }

    if (!S_ISCHR (st.st_mode)) {
        cerr << deviceName << " is no device." << endl;
        filedescriptor = -1;
    }

    filedescriptor = open(deviceName, O_RDWR|O_NONBLOCK, 0);

    if (filedescriptor == -1) {
        cerr << "Cannot open \"" << deviceName << "\": " << errno << "(" << strerror (errno) << ")." << endl;
        filedescriptor = -1;
    }

    return filedescriptor;
}


/**
 * @param captureSizeX might be changed by the device
 * @param captureSizeY might be changed by the device
 * @returns needed buffer size
 */
unsigned int initDevice(int fileDescriptor, unsigned int *captureSizeX, unsigned int *captureSizeY, const char *deviceName)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;


    if (-1 == xioctl(fileDescriptor, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            cerr << "\"" << deviceName << "\" is no V4L2 device." << endl;
            assert(0);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }


    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        cerr << "\"" << deviceName << "\" is no video capture device." << endl;
        assert(0);
    }

    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
        cerr << "\"" << deviceName << "\" does not support read i/o." << endl;
        assert(0);
    }


    /* *** Select video input, video standard and tune here. *** */

    memset(&cropcap, 0, sizeof(v4l2_cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(fileDescriptor, VIDIOC_CROPCAP, &cropcap);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    xioctl(fileDescriptor, VIDIOC_S_CROP, &crop); /* ignore errors */


    memset(&fmt, 0, sizeof(v4l2_format));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = *captureSizeX; 
    fmt.fmt.pix.height = *captureSizeY;
    fmt.fmt.pix.pixelformat = PIXEL_FORMAT;
    fmt.fmt.pix.field = FIELD_FORMAT;

    if (-1 == xioctl (fileDescriptor, VIDIOC_S_FMT, &fmt)) {
        errno_exit ("VIDIOC_S_FMT");
    }

    *captureSizeX = fmt.fmt.pix.width;
    *captureSizeY = fmt.fmt.pix.height;

    cout << *captureSizeX << "x" << *captureSizeY << endl
            << "format: "
            << (char) ((fmt.fmt.pix.pixelformat & 0x000000ff) >> 0)
            << (char) ((fmt.fmt.pix.pixelformat & 0x0000ff00) >> 8)
            << (char) ((fmt.fmt.pix.pixelformat & 0x00ff0000) >> 16)
            << (char) ((fmt.fmt.pix.pixelformat & 0xff000000) >> 24)
            << endl
            << "field: " << fmt.fmt.pix.field << endl << endl;

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;


    return fmt.fmt.pix.sizeimage;
}


void closeDevice(int fileDescriptor)
{
    if (-1 == close (fileDescriptor)) {
        errno_exit("close");
    }
}


int main(int argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const char *devicename = "/dev/video0";
    unsigned int capturesizex = CAPTURE_SIZE_X;
    unsigned int capturesizey = CAPTURE_SIZE_Y;
    int filedescriptor;
    unsigned int neededbuffersize;
    unsigned char *buffer;
    
    filedescriptor = openDevice(devicename);
    assert(filedescriptor != -1);

    neededbuffersize = initDevice(filedescriptor, &capturesizex, &capturesizey, devicename);

    enumerateControls(filedescriptor);

    
    buffer = (unsigned char*) malloc(sizeof(unsigned char) * neededbuffersize);
    assert(buffer != 0);


    startCapturing();

    mainLoop(filedescriptor, buffer, neededbuffersize, capturesizex, capturesizey);

    stopCapturing();


    free(buffer);


    closeDevice(filedescriptor);

    return 0;
}

/* *** enum controls ******************************************************* */
static void enumerateMenu(int fileDescriptor, const v4l2_queryctrl *ctrl)
{
    cout << "  Menu items [" << endl;

    struct v4l2_querymenu menu;
    memset (&menu, 0, sizeof (v4l2_querymenu));
    menu.id = ctrl->id;


    for (menu.index = ctrl->minimum; ((__s32) menu.index) <= ctrl->maximum; menu.index++) {

        if (0 == ioctl(fileDescriptor, VIDIOC_QUERYMENU, &menu)) {
                cout << "  " << menu.name;
        } else {
                perror("VIDIOC_QUERYMENU");
                assert(0);
        }
    }

    cout << "  ]" << endl;
}


void enumerateControls(int deviceFileDescriptor, bool all)
{
    struct v4l2_queryctrl ctrl;
    memset (&ctrl, 0, sizeof(v4l2_queryctrl ));

    cerr << "Available Controls:" << endl;

    for (ctrl.id = V4L2_CID_BASE; ctrl.id < V4L2_CID_LASTP1; ctrl.id++) {

        if (0 == ioctl(deviceFileDescriptor, VIDIOC_QUERYCTRL, &ctrl)) {

            if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                if (all) cout << "    " << ctrl.id - V4L2_CID_BASE << " \"" << ctrl.name << "\"" << endl;
            } else {
                cout << "  x " << ctrl.id - V4L2_CID_BASE << " \"" << ctrl.name << "\"" << endl;
                if (ctrl.type == V4L2_CTRL_TYPE_MENU) {
                    enumerateMenu(deviceFileDescriptor, &ctrl);
                }
            }

        } else {

            if (all) cout << "    " << ctrl.id - V4L2_CID_BASE << " Error" << endl;

            if (errno == EINVAL) continue;

            perror ("VIDIOC_QUERYCTRL");
            assert(0);
        }
    }

    cerr << "Available Private Controls:" << endl;

    for (ctrl.id = V4L2_CID_PRIVATE_BASE;; ++ctrl.id) {

        if (0 == ioctl(deviceFileDescriptor, VIDIOC_QUERYCTRL, &ctrl)) {

            if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                if (all) cout << "    " << ctrl.id - V4L2_CID_PRIVATE_BASE << " \"" << ctrl.name << "\"" << endl;
            } else {
                cout << "  x " << ctrl.id - V4L2_CID_PRIVATE_BASE << " \"" << ctrl.name << "\"" << endl;

                if (ctrl.type == V4L2_CTRL_TYPE_MENU) {
                    enumerateMenu(deviceFileDescriptor, &ctrl);
                }
            }

        } else {

            if (all) cout << "    " << ctrl.id - V4L2_CID_PRIVATE_BASE << " Error" << endl;

            if (errno == EINVAL) break; /* end of loop only here */

            perror ("VIDIOC_QUERYCTRL");
            assert(0);
        }
    }
}


/* *** misc **************************************************************** */
void errno_exit(const char *description)
{
    cerr << "\"" <<  description << "\" error " << errno << " (" << strerror (errno) << ")" << endl;
    assert(0);
}


int xioctl(int fileDescriptor, int request, void *arg)
{
    int r;

    do r = ioctl (fileDescriptor, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

