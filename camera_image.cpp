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

int main(){
	cv::VideoCapture cap("/home/arna/Desktop/IITKGP/projects/Microscope/vdo1.wmv"); 
	long long int t=0;
	while(1){
		cv::Mat frame;
		cap>>frame;
		cv::imshow("video",frame);
		std::ostringstream oss;
		oss<<"im"<<(t-20)<<".jpg";
		if(t>20 && t%4==0)cv::imwrite(oss.str(),frame);
		if(cv::waitKey(33)==27)break;
		t++;
	}
	std::cout<<t<<std::endl;
	return 0;
}
