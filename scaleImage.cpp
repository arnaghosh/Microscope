#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

/// Global variables

char* window_name = "Pyramids Demo";


/**
 * @function main
 */
int main( int argc, char** argv )
{
	cv::Mat src, dst, tmp;
	src = cv::imread( argv[1] );
	if( !src.data )
		{ printf(" No data! -- Exiting the program \n");
		return -1; }

	tmp = src;
	dst = tmp;

  /// Create window
  cv::namedWindow( window_name, CV_WINDOW_AUTOSIZE );
  cv::imshow( window_name, dst );
  cv::pyrUp( tmp, dst, Size( tmp.cols*2, tmp.rows*2 ) );
      
  cv::pyrDown( tmp, dst, Size( tmp.cols/2, tmp.rows/2 ) );
     

  cv::imshow( window_name, dst );
    tmp = dst;
  
  return 0;
}
