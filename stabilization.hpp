/*
Maxime Touroute
28 July 2015
*/

// OpenCV libs
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/ml/ml.hpp>
// C libs
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <sstream>


// Black borders constants
#define BORDER_WIDTH 60
#define BORDER_HEIGHT 60

// Parameters of the stabilization mask
#define FIELD_MASK_DILATION_SIZE 1
#define FIELD_MASK_EROSION_SIZE 5
#define PUBLIC_MASK_DILATION_SIZE 20
#define PUBLIC_MASK_EROSION_SIZE 5

// Parameters of the singularity mask
#define SINGULARITY_MASK_BORDER 80


using namespace cv;
using namespace std;


void drawAxis(Mat& frame);
Mat scaleGrayFrame(Mat frame, int scale);
Mat scaleColorFrame(Mat frame, int scale);

Mat getBorderMask(const int rows, const int cols, const int borderSize);
Mat addBlackBorder(Mat frame, int borderWidth, int borderHeight);
Mat preProccessingStabilization(const Mat frame);
Mat stabilize(const Mat previousFrame, const Mat currentFrame);

void erodeMask(Mat& mask, int erosionSize);
void dilateMask(Mat& mask, int dilationSize);
Mat detectGrass(const Mat frame);

Mat detectScoreOverlayPanel(const Mat frame, int height_offset = 0, int width_offset = 0);
Mat getMaskOfIrrelevantAreasForCameraStabilization(const Mat frame);
Mat getMaskOfIrrelevantAreasForSingularities(const Mat frame);

// Mat panelDetector(Mat previousFrame, Mat currentFrame); WIP