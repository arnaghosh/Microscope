#include <stdio.h>
#include "SimpleGPIO.h"
#include "SimpleGPIO.cpp"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <cv.h>
#include <highgui.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "BMA180Accelerometer.h"
#include "EasyDriver.h"
#include "EasyDriver.cpp"
#include <fstream>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

// a buffer (i.e. temporary array for data) that is used
struct buffer {
	void *start;
	size_t length;
};


static void xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = v4l2_ioctl(fh, request, arg);
	} while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

	if (r == -1) {
		fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


static int              fd = -1;
struct buffer          *buffers;
static int 		image_width = 1080;
static int 		image_height = 720;

// This function is called to initialize the camera with Video4Linux, so that we can use the camera to take photos
static void init_dev(void)
{
	struct v4l2_format fmt;
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = image_width;
	fmt.fmt.pix.height = image_height;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	//fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	xioctl(fd, VIDIOC_S_FMT, &fmt);

	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	xioctl(fd, VIDIOC_REQBUFS, &req);

	buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));
	struct v4l2_buffer buf;
	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;

	xioctl(fd, VIDIOC_QUERYBUF, &buf);

	buffers->length = buf.length;
	buffers->start = v4l2_mmap(NULL, buf.length,PROT_READ | PROT_WRITE, MAP_SHARED,fd, buf.m.offset);

	if (MAP_FAILED == buffers[0].start) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
}

// this uninitializes the device, whenever we have no need of it yet
static void uninit_device(void)
{
	unsigned int i;

	if (-1 == munmap(buffers->start, buffers->length))
		exit(EXIT_FAILURE);

	free(buffers);
	return;
}

// allows us to record using the camera. Uses Video4Linux
static void start_capturing(void)
{
	enum v4l2_buf_type type;
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	xioctl(fd, VIDIOC_QBUF, &buf);

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(fd, VIDIOC_STREAMON, &type);
}

// stops recording using the camera when no more images are needed. Uses Video4Linux
static void stop_capturing(void)
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(fd, VIDIOC_STREAMOFF, &type);
	return;
}

// takes an image using the camera using Video4Linux
IplImage* image_capture(void)
{
	int r;
	do
	{
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	// Timeout
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);
	} while ((r == -1 && (errno = EINTR)));

	if (r == -1)
	{
		perror("select");
		//return errno;
	}

	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	xioctl(fd, VIDIOC_DQBUF, &buf);
	CvMat cvmat = cvMat(image_height, image_width, CV_8UC3, (void*)buffers->start);
	IplImage* bgr_frame = cvDecodeImage(&cvmat, 1); 

	xioctl(fd, VIDIOC_QBUF, &buf); 
	return (bgr_frame);
}

// calculates how sharp the image is. The greater the number returned, the more sharp the image is.
// In the for loop, the program essentially calculates how different the pixels and its neighbours are
long sharpness(IplImage* bgr_frame)
{
	int x, y;
	unsigned char *tmp, *ptr0, *ptr1, *ptr2;
	long integral=0;
	//printf("%d", bgr_frame->width);
	IplImage *gray_frame=cvCreateImage(cvSize(bgr_frame->width, bgr_frame->height), IPL_DEPTH_8U, 1);
	cvCvtColor(bgr_frame, gray_frame, CV_RGB2GRAY);
	IplImage *grad=cvCreateImage(cvSize((bgr_frame)->width, (bgr_frame)->height), IPL_DEPTH_8U, 1);
	ptr1=(unsigned char*)gray_frame->imageData;
	ptr2=(unsigned char*)gray_frame->imageData+gray_frame->widthStep;

	for(y=1;y<gray_frame->height-1; y++)
	{
		ptr0 = ptr1;
		ptr1 = ptr2;
		ptr2 = ptr2 + gray_frame->widthStep;
		for(x=1; x<gray_frame->width-1; x++)
		{
			int Gx =(int)(1 *  *(ptr2+x+1) + 2 * *(ptr1+x+1) + 1 * *(ptr0+x+1) - 1 * *(ptr2+x-1) - 2 * *(ptr1+x-1) - 1 * *(ptr0+x-1));
			int Gy =(int)(1 *  *(ptr2+x-1) + 2 * *(ptr2+x) + 1 * *(ptr2+x+1) - 1 * *(ptr0+x-1) - 2 * *(ptr0+x) - 1 * *(ptr0+x+1)); 
			//tmp = (unsigned char*)grad->imageData+ y*grad->widthStep+x;
			//*tmp = (unsigned char) (0.5*(abs(Gx)+abs(Gy)));
			integral += (long)(abs(Gx)+abs(Gy));
		}
	}	
	//cvNamedWindow("gradient",CV_WINDOW_AUTOSIZE);
	//cvShowImage("gradient", grad);
	//cvNamedWindow("image",CV_WINDOW_AUTOSIZE);
	//cvShowImage("image", bgr_frame);
	//cvWaitKey(0);

	cvReleaseImage(&grad);
	cvReleaseImage(&gray_frame);
	return integral;
}


/*long sharpness(IplImage* bgr_frame){
  int x, y;
  unsigned char *tmp, *ptr0, *ptr1, *ptr2;
  long integral=0;
  //printf("%d", bgr_frame->width);
  IplImage *gray_frame=cvCreateImage(cvSize(bgr_frame->width, bgr_frame->height), IPL_DEPTH_8U, 1);
  	cvCvtColor(bgr_frame, gray_frame, CV_RGB2GRAY);
  IplImage *grad=cvCreateImage(cvSize((bgr_frame)->width, (bgr_frame)->height), IPL_DEPTH_8U, 1); 
  ptr1=(unsigned char*)gray_frame->imageData + 178 * gray_frame->widthStep;
  ptr2=(unsigned char*)gray_frame->imageData + 179 * gray_frame->widthStep;
  for(y = 1;y < 359; y++){
    ptr0 = ptr1;
    ptr1 = ptr2;
    ptr2 = ptr2 + gray_frame->widthStep;
    for(x = 269; x < 809; x++){
      int Gx =(int)(1 *  *(ptr2+x+1) + 2 * *(ptr1+x+1) + 1 * *(ptr0+x+1) - 1 * *(ptr2+x-1) - 2 * *(ptr1+x-1) - 1 * *(ptr0+x-1));
      int Gy =(int)(1 *  *(ptr2+x-1) + 2 * *(ptr2+x) + 1 * *(ptr2+x+1) - 1 * *(ptr0+x-1) - 2 * *(ptr0+x) - 1 * *(ptr0+x+1)); 
      //tmp = (unsigned char*)grad->imageData+ y*grad->widthStep+x;
      //*tmp = (unsigned char) (0.5*(abs(Gx)+abs(Gy)));
      integral += (long)(abs(Gx)+abs(Gy));
    }
  }
  cvNamedWindow("gradient",CV_WINDOW_AUTOSIZE);
  //cvShowImage("gradient", grad);
  //cvNamedWindow("image",CV_WINDOW_AUTOSIZE);
  //cvShowImage("image", bgr_frame);
  //cvWaitKey(0);

  cvReleaseImage(&grad);
  cvReleaseImage(&gray_frame);
  return integral;
} */


bool is_empty(std::ifstream& pFile)
{
	return pFile.peek() == std::ifstream::traits_type::eof();
}

// takes the pointer of the IplImage object. Saves the image to a file in the current folder.
int save_image(IplImage* image,int x,int y)
{
	char out_name[256];
	sprintf(out_name, "image%03da%03d.jpg", x,y);
	cvSaveImage(out_name, image, 0);

	// checks whether the image is empty and gives an error if it is empty.
	if ( image == NULL)
		printf( "You are trying to save image%03da%03d.jpg, which is currently empty, because the image pointer is NULL.", x,y);


	std::ifstream file(out_name);
	while (is_empty(file))
	{
		cvSaveImage(out_name, image, 0); // save the file again if and only if the file is empty. this error has occured several times
	}

	return 0;
}

// global variables that store the GPIO number of the pins that will be used to control when the DAC loads data, which output is addressed, when to reset, which pins are connected to the parallel interface, etc.
// the datasheet of the DAC explains what each pin's purpose is
unsigned int DB[12] = {30, 60, 31, 50, 48, 51, 5, 4, 3, 2, 49, 15}; //P9_11 to p9_18 && P9_21 to P9_24
unsigned int W = 14; //p9_26 
unsigned int A1 = 20; //P9_27
unsigned int A0 = 115; //P9_41A
unsigned int CS = 7; //P9_42A
unsigned int LDAC = 61; //P8_26
unsigned int RESET = 22; //P8_19

// sets up the necessary GPIO Pins in the linux operating system for the DAC. 
// see https://www.youtube.com/watch?v=wui_wU1AeQc for detailed information on what must be done to use the GPIO pins of the Beaglebone Black
void setGPIOPins()
{
	int i;
	for(i=0; i<12; i++)
	{
		gpio_export(DB[i]);
		gpio_set_dir(DB[i], OUTPUT_PIN);
	}

	gpio_export(W);
	gpio_export(A1);
	gpio_export(A0);
	gpio_export(CS);
	gpio_export(LDAC);
	gpio_export(RESET);
	gpio_set_dir(W, OUTPUT_PIN);
	gpio_set_dir(A1, OUTPUT_PIN);
	gpio_set_dir(A0, OUTPUT_PIN);
	gpio_set_dir(CS, OUTPUT_PIN);
	gpio_set_dir(LDAC, OUTPUT_PIN);
	gpio_set_dir(RESET, OUTPUT_PIN);
}

// takes a number from -2048 to 2047. 2048 is added to this number, converted to binary, then sent through a parallel interface to the DAC. To see how this the DAC handles the bits through the digital interface refer to its datasheet.
// Some important values for our setup:
// -2048 => -2V
// -1024 => -1V
// 0 => 0V
// 1024 => 1V
// 2047 => 2V
int Set_Value(int value, char Channel)
{
	int i;

	int S = value + 2048;
	/* Convesion into binary */
	for(i=0; i<12; i++)
	{
		if(S%2 == 1)
			gpio_set_value(DB[i], HIGH);
		else 
			gpio_set_value(DB[i], LOW);
		S = S/2; 
	}
	switch(Channel)
	{
		case 'A' :
			gpio_set_value(A0, LOW);
			gpio_set_value(A1, LOW);
			break;
		case 'B':
			gpio_set_value(A0, HIGH);
			gpio_set_value(A1, LOW);
			break;
		case 'C':
			gpio_set_value(A0, LOW);
			gpio_set_value(A1, HIGH);
			break;
		case 'D':
			gpio_set_value(A0, HIGH);
			gpio_set_value(A1, HIGH);
			break;
	}

	gpio_set_value(RESET, HIGH);
	gpio_set_value(W, LOW);
	gpio_set_value(LDAC, LOW);
	gpio_set_value(CS, LOW);
	//gpio_set_value(LDAC, HIGH);
	return S;
}

// sets the output of the DAC to 100000000000, which is 0V in our setup. This function is not used.
static void RESET_DAC(void)
{
	gpio_set_value(RESET, LOW);
	return;
}

// returns an address to an IplImage that contains the information of the sharpest image
IplImage* AutoFocus(int x, int y)
{
	int i, opt_point, S; // opt_point is the S value which has produced the sharpest image
	int gapping = 1023; // how far away in bits we will move from the reference level
	int set_value = 0;  // the inference point (i.e. the middle level)
	long sharpest;	    // an integar that will the value of the sharpnest image, so that we can compare the the sharpness image with any other images that are taken
	long integral;      // an integar that will store the value of the sharpness function for the current image
	char out_name[256]; // temporary stores the name of the image
	IplImage* frame = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 3);
	while(gapping > 0){
	//printf("%d\n",gapping);
		for(i=0; i<5 ;++i)
		{
			S = set_value + (i-2)* gapping;
			printf("%d", S);
			Set_Value(S, 'B');

			if ( gapping > 200)		// if the gapping is very large, allow the lens to come to a rest before taking a picture
			{
				sleep(1);
			}

			start_capturing();
			frame = image_capture();
			stop_capturing();

			sprintf(out_name, "image%04da%09d.ppm", S, integral);  //comment here not to show every image
			cvNamedWindow(out_name,CV_WINDOW_AUTOSIZE);            //comment here not to show every image
			cvShowImage(out_name,frame);                           //comment here not to show every image
			cvWaitKey(0);                                          //comment here not to show every image
			while ( frame == NULL ) // sometimes the image_capture function returns a null pointer, so we need to call it again to retrieve a valid pointer.
			{ 
				printf("\n\nimage%03da%03d.jpg is currently a null pointer. If this message repeats indefinitely, this message and the program are in an infinite loop.\n\n", x,y);
				start_capturing();
				frame = image_capture();
				stop_capturing();
			}



			integral= sharpness(frame);
			cvReleaseImage(&frame);

			if(integral > sharpest)
			{
				opt_point = i;
				sharpest = integral;
			}    
		}
		set_value = set_value + (opt_point-2) * gapping;
		gapping = (3 * gapping / 8);
		sharpest = 0;
	}  

	Set_Value(set_value, 'B');
	start_capturing();
	frame = image_capture();
	stop_capturing();
	return (frame);
}


int main()
{ 
	// initialize the GPIO (General Purpose Input-Output) pins that will be used with the DAC
	// for detailed information, see https://www.youtube.com/watch?v=wui_wU1AeQc 
	setGPIOPins();
	// creates a empty IplImage object that will serve as a blank template for images taken. The IplImage class is a OpenCV class that manages all the data of an image and its metadata
	// for detailed information, read O'Reilly's Learning OpenCV book, which is located on the Windows computer.
	IplImage* frame = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 3);
	
	// Obtain the file descriptor of the camera and check if it failed. The file descriptor is what is used to retrieve data from a device of file; in this case, we retrieve data from the camera.
	// in Linux, this is called a file descriptor, but in Windows it is called a handle. There is plenty of information online about how to use file descriptors
	char dev_name[] = "/dev/video0";
	fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) {
		perror("Cannot open device");
		exit(EXIT_FAILURE);
	}

	//initializes camera to be used with Video4Linux with our program
  	init_dev();


	// Stepper ONE Initailization. See the EasyDriver.h and EasyDriver.cpp files which are included at the top of this source code for a deatiled explanation
	// EasyDriver(int gpio_MS1, int gpio_MS2, int gpio_STEP, int gpio_SLP, int gpio_DIR, int speedRPM, int stepsPerRevolution)
	EasyDriver motor1(66, 67, 69, 68, 45, 240, 200);
	// sets the sleep pin to high, so instructions will be read. If the sleep pin is low, no instructions are read. The function is located in the EasyDriver.h file
	motor1.wake();
	// sets the default step. In this case, the default step is a full step
	motor1.setStepMode(STEP_FULL);

	// Stepper TWO Initialization
	EasyDriver motor2(44, 23, 26, 47, 46, 240, 200);
	motor2.wake();
	motor2.setStepMode(STEP_FULL);
	motor2.reverseDirection();


	/* Scan a small area*/
	int i,j;
	bool dir = 0;
	j = 0;
	i =-1;
	while(j<17){
		i++;		
		frame = AutoFocus(j, i);


		//cvNamedWindow("image",CV_WINDOW_NORMAL);
		//cvShowImage("image",frame);
		//cvWaitKey(0);
		//cvDestroyWindow("image");
		if(dir == 0){
			save_image(frame,j,i);
		}
		else{
			save_image(frame, j, (13 - i));
		}
		cvReleaseImage(&frame);


		if(i > 12){
			if ( j == 0)
				motor2.step(100); 


			j++;
			motor2.step(75);  //calculate exactly how many steps it has to move
			motor1.reverseDirection();
			motor1.step(100);
			i = -1;
			dir = ~dir ;
		}
		else{
			motor1.step(75); //calculate exactly how many steps it has to move
		}
	}

	//double end = clock();
	//printf("Time taken %lf", (end- start)/CLOCKS_PER_SEC);
	//save_image(frame, gapping, set_value); //Later gapping and set_value need to be replaced by x and y coordinate of the picture 
	return 0;
}
