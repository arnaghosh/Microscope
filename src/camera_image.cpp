#include <stdio.h>
#include <iostream>
#include <string> 
#include <sstream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
using namespace std;

cv::Mat sharpenImage(cv::Mat img){
	cv::Mat blurred=img;
	//cv::GaussianBlur(img,blurred,cv::Size(5,5),5);
	cv::pyrDown(img,blurred,cv::Size(img.cols/2,img.rows/2));
	cv::pyrDown(blurred,blurred,cv::Size(blurred.cols/2,blurred.rows/2));
	cv::pyrDown(blurred,blurred,cv::Size(blurred.cols/2,blurred.rows/2));
	cv::pyrUp(blurred,blurred,cv::Size(blurred.cols*2,blurred.rows*2));
	cv::pyrUp(blurred,blurred,cv::Size(blurred.cols*2,blurred.rows*2));
	cv::pyrUp(blurred,blurred,cv::Size(img.cols,img.rows));
	//cv::imshow("orig",img);
	//cv::imshow("blur",blurred);
	//cv::waitKey(0);
	cv::addWeighted(img,1.5,blurred,-0.5,0,img);
	//cv::imshow("sharper",img);
	//cv::waitKey(0);
	return img;
}

int main(int argc,char** argv){
	cv::namedWindow("video",1);
	cv::namedWindow("sharp",1);
	string s="/home/arna/Desktop/IITKGP/projects/Microscope_working/";
	//cv::VideoCapture cap(s.append(argv[1])); 
	cv::VideoCapture cap(0);
	long long int t=0;
	while(1){
		cv::Mat frame;
		cap>>frame;
		cv::imshow("video",frame);
		frame=sharpenImage(frame);
		cv::imshow("sharp",frame);
		std::ostringstream oss;
		oss<<"im"<<(t-20)<<".jpg";
		//if(t>10 && t%10==0)cv::imwrite(oss.str(),frame);
		if(cv::waitKey(33)==27)break;
		t++;
	}
	std::cout<<t<<std::endl;
	return 0;
}
