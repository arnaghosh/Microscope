#include <iostream>
#include <cmath>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

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
	cv::imshow("sobel",sobel);
	cv::waitKey(0);
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
	cv::imshow("orig",img);
	cv::imshow("blur",blurred);
	cv::waitKey(0);
	cv::addWeighted(img,1.5,blurred,-0.5,0,img);
	cv::imshow("sharper",img);
	cv::waitKey(0);
	return img;
}

int main(int argc,char** argv){
	cv::Mat original = cv::imread(argv[1],1);
	long img_sharpness=sobel_sharpness(original);
	std::cout<<"image sharpness="<<img_sharpness<<std::endl;
	img_sharpness=sobel_sharpness(sharpenImage(original));
	std::cout<<"sharpened image sharpness="<<img_sharpness<<std::endl;
	/*original = cv::imread(argv[2],1);
	img_sharpness=sobel_sharpness(original);
	std::cout<<"image sharpness="<<img_sharpness<<std::endl;
	img_sharpness=sobel_sharpness(sharpenImage(original));
	std::cout<<"sharpened image sharpness="<<img_sharpness<<std::endl;*/
	
}
