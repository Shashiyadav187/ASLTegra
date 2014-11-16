//opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//C
#include <stdio.h>
//C++
#include <iostream>
#include <sstream>

using namespace cv;
using namespace std;

enum { MAX_WORDS = 26 };

//global variables
Rect cropArea;
Mat cropFrame;
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
int keyboard;
RNG rng(12345);
int thresh = 200;
Mat letters[MAX_WORDS];
int frames = 0;

//function declarations
void processVideo();

int main(int argc, char* argv[])
{

    //create GUI windows
    namedWindow("Crop Frame");
    namedWindow("FG Mask MOG");

    //create Background Subtractor objects
   //NOTE HERE!!!!
    pMOG2 = new BackgroundSubtractorMOG2(); //MOG2 approach

	// Preload letter images
	for (int i = 0; i < MAX_WORDS; i++) {
		char buf[13 * sizeof(char)];
		sprintf(buf, "images/%c.png", (char)('a' + i));
		Mat im = imread(buf, 1);
		if (im.data) {
			letters[i] = im.clone();
		}
	}

    processVideo();

    //destroy GUI windows
    destroyAllWindows();
    return EXIT_SUCCESS;
}

void processVideo() {
    //create the capture object
    VideoCapture capture = VideoCapture(0);
    if(!capture.isOpened()){
        //error in opening the video input
        cerr << "Cannot Open Webcam... " << endl;
        exit(EXIT_FAILURE);
    }
    //read input data. ESC or 'q' for quitting
    while( (char)keyboard != 27 ){
        //read the current frame
        if(!capture.read(frame)) {
            cerr << "Unable to read next frame." << endl;
            cerr << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }

	// Crop Frame to smaller region
	cv::Rect myROI(50, 50, 200, 200);
	cropFrame = frame(myROI);

        //update the background model
         //AND HERE!!!
        pMOG2->operator()(cropFrame, fgMaskMOG2);

	// Generate Convex Hull

	   Mat threshold_output;
	   vector<vector<Point> > contours;
	   vector<Vec4i> hierarchy;

	   /// Detect edges using Threshold
	   threshold( fgMaskMOG2, threshold_output, thresh, 255, THRESH_BINARY );

	   /// Find contours
	   findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	   /// Find largest contour
	   Mat drawing = Mat::zeros( cropFrame.size(), CV_8UC3 );
	double largest_area=0;
	int maxIndex =0;
	double a;
	   for( int j = 0; j< contours.size(); j++ )
	      {
		a=contourArea( contours[j],false);  //  Find the area of contour
		    if(a>largest_area){
			largest_area=a;
			maxIndex=j;      //Store the index of largest contour
		    }
	      }

	// Draw Largest Contours
	Scalar color = Scalar( 0, 0, 255);
	drawContours( drawing, contours, maxIndex, Scalar(255, 255, 255), CV_FILLED); // fill white

	// Compare to reference imagesaaa
	if (frames++ >= 60) {
		frames = 0;
		long lowestDiff = 255*150*150*4 + 1; // larger than largest possible difference
		char best = 0; 
		for (int i = 0; i < MAX_WORDS; i++) {
			//cout << letters[i].rows << endl;
			if (letters[i].rows == 0) continue;
			Mat im;
			bitwise_xor(drawing, letters[i], im);
			Scalar diffV = sum(im);
			long diff = diffV[0] + diffV[1] + diffV[2] + diffV[3]; 
			if (diff < lowestDiff) {
				lowestDiff = diff;
				best = i + 'a';
			}
			//cout << (char)('a' + i) << ": " << diff << endl;
			
		}
		cout << best << ",  Diff: " << lowestDiff << endl; 
	}

        //show the current frame and the fg masks
	imshow("Crop Frame", cropFrame);
        imshow("FG Mask MOG", fgMaskMOG2);
        imshow("Contour", drawing);
        //get the input from the keyboard
        keyboard = waitKey( 1 );

	// Save image as keyboard input
	if (keyboard >= 'a' && keyboard <= 'z') {
		letters[keyboard - 'a'] = drawing;
		char buf[13 * sizeof(char)];
		sprintf(buf, "images/%c.png", (char)keyboard);
		imwrite(buf, drawing);
	}
    }
    //delete capture object
    capture.release();
}