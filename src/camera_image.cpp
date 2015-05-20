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

cv::Mat rgb2gray(cv::Mat img){
	cv::Mat res(img.rows,img.cols,CV_8UC1,cvScalarAll(0));
	for(int i=0;i<img.rows;i++){
		for(int j=0;j<img.cols;j++){
			res.at<uchar>(i,j)=(4*img.at<cv::Vec3b>(i,j)[0]+4*img.at<cv::Vec3b>(i,j)[1]+img.at<cv::Vec3b>(i,j)[2])/9;
		}
	}
	return res;
}

int absol(int x){
	if(x<0)return -x;
	return x;
}

long sobel_sharpness(cv::Mat color){
	long integral=0;
	cv::Mat gray=rgb2gray(color);
	cv::Mat sobel=gray.clone();
	int max_abs=0;
	for(int i=1;i<gray.rows-1;i++){
		for(int j=1;j<gray.cols-1;j++){
			int gx=(int)(gray.at<uchar>(i+1,j+1)+2*gray.at<uchar>(i,j+1)+gray.at<uchar>(i-1,j+1) - gray.at<uchar>(i-1,j-1)-2*gray.at<uchar>(i,j-1)-gray.at<uchar>(i+1,j-1));
			int gy=(int)(gray.at<uchar>(i+1,j+1)+2*gray.at<uchar>(i+1,j)+gray.at<uchar>(i+1,j-1) - gray.at<uchar>(i-1,j-1)-2*gray.at<uchar>(i-1,j)-gray.at<uchar>(i-1,j+1));
			integral+=(long)(absol(gx)+absol(gy))/2;
			//int abs=sqrt(gx*gx+gy*gy);
			int abs=(int)(absol(gx)+absol(gy))/2;
			if(abs>max_abs)max_abs=abs;
			sobel.at<uchar>(i,j)=abs;
		}
	}
	for(int i=1;i<sobel.rows-1;i++){
		for(int j=1;j<sobel.cols-1;j++){
			sobel.at<uchar>(i,j)=sobel.at<uchar>(i,j)*255/max_abs;
		}
	}
	return integral;
}


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
	int r=atoi(argv[1]);
	//cv::VideoCapture cap(s.append(argv[2])); 
	cv::VideoCapture cap(1);
	long long int t=0;
	int selection=0;
	long long int sharpest=0;
	cv::Mat sharpestIm;
	while(1){
		cv::Mat frame;
		cap>>frame;
		cv::imshow("video",frame);
		frame=sharpenImage(frame);
		cv::imshow("sharp",frame);
		long long sharpness=sobel_sharpness(frame);
		//cout<<t<<":"<<selection<<" "<<sharpness<<" "<<sharpest<<endl;
		if(sharpness>sharpest){
			sharpest=sharpness;
			frame.copyTo(sharpestIm);
		}
		if(selection>5){
			std::ostringstream oss;
			oss<<"im"<<(t-10)<<".jpg";
			if(r==1 && t>10)cv::imwrite(oss.str(),sharpestIm);
			sharpest=0;
			selection=0;
		}
		if(cv::waitKey(33)==27)break;
		t++;
		selection++;
	}
	std::cout<<t<<std::endl;
	return 0;
}
