#include <stdio.h>
#include <string>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

int main(int argc, char** argv )
{
    int iLowB = 0;
    int iHighB = 80;
    int iLowG = 0; 
    int iHighG = 80;
    int iLowR = 140;
    int iHighR = 255;
    int iBri = 70;
    int iCon = 65;
    int iSat = 50;

    if (argc > 1) {
        string arg = argv[1];
        if (arg == "-c") {

            namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"

            //Create trackbars in "Control" window
            cvCreateTrackbar("LowB", "Control", &iLowB, 255); //Hue (0 - 179)  //blue
            cvCreateTrackbar("HighB", "Control", &iHighB, 255);

            cvCreateTrackbar("LowG", "Control", &iLowG, 255); //Saturation (0 - 255)  green
            cvCreateTrackbar("HighG", "Control", &iHighG, 255);

            cvCreateTrackbar("LowR", "Control", &iLowR, 255); //Value (0 - 255)  red
            cvCreateTrackbar("HighR", "Control", &iHighR, 255);

            cvCreateTrackbar("Brightness", "Control", &iBri, 255);
            cvCreateTrackbar("Contrast", "Control", &iCon, 255);
            cvCreateTrackbar("Saturation", "Control", &iSat, 255);
        }
    }

    Mat imgOriginal;
    raspicam::RaspiCam_Cv Camera;
    Camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    if (!Camera.open()) { 
        std::cerr  <<  "Error opening the camera"  <<  std::endl;
        return -1;
    }

    namedWindow("Display Image", CV_WINDOW_AUTOSIZE );
    namedWindow("Thresholded", CV_WINDOW_AUTOSIZE );

    while(true) {

        // Get a frame from the camera.
        Camera.set(CV_CAP_PROP_BRIGHTNESS, iBri);
        Camera.set(CV_CAP_PROP_CONTRAST, iCon);
        Camera.set(CV_CAP_PROP_SATURATION, iSat);
        Camera.grab();
        Camera.retrieve(imgOriginal);

        Mat imgHSV;
        cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
        Mat imgThresholded;
        inRange(imgOriginal, Scalar(iLowB, iLowG, iLowR), Scalar(iHighB, iHighG, iHighR), imgThresholded); //Threshold the image

        // Dilate the thresholded image
        dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(10, 10)));

        imshow("Thresholded", imgThresholded);

        // Find contours
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(imgThresholded, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

        /// Approximate contours to polygons + get bounding rects and circles
        vector<vector<Point> > contours_poly(contours.size());
        vector<Rect> boundRect(contours.size());
        vector<Point2f>center(contours.size());
        vector<float>radius(contours.size());

        // Build polygons, bounding rects and circles
        int largest = -1;
        int max = 0;
        for(int i = 0; i < contours.size(); i++) {
            approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
            boundRect[i] = boundingRect( Mat(contours_poly[i]) );
            minEnclosingCircle((Mat)contours_poly[i], center[i], radius[i]);
            if (radius[i] > max) {
                max = radius[i];
                largest = i;
            }
        }

        // Draw bounding circle of largest object detected.
        if (largest > -1) {
            Scalar color = Scalar(0, 255, 0); // green
            Point2f pt = center[largest];
            string txt = std::to_string(pt.x);
            circle(imgOriginal, center[largest], (int)radius[largest], color, 2, 8, 0);
            putText(imgOriginal, txt, pt, FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 1);
        }

        // Show the original image with circle(s) overlaid.
        imshow("Display Image", imgOriginal);

        // Wait for a keypress
        int c = cvWaitKey(10);
        if(c!=-1)
        {
            // If pressed, break out of the loop
            break;
        }

    }
    Camera.release();
    return 0;
}
