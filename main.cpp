#include "stabilization.cpp"

// This keeps the webcam/video from locking up when you interrupt a frame capture
volatile int quit_signal = 0;
#ifdef __unix__
#include <signal.h>

extern "C" void quit_signal_handler(int signum) 
{
	if ( quit_signal != 0 )
	{
		exit(0); // just exit already
	} 
	quit_signal = 1;
	printf("Will quit at next frame\n");
}
#endif

int main(int c, char ** argv)
{
#ifdef __unix__
	// listen for ctrl-C
	signal(SIGINT, quit_signal_handler); 
#endif

	VideoCapture videoBuffer;
	std::string videoPath = "samples/calme.mp4";
	videoBuffer = VideoCapture(videoPath);

	// Handling errors
	if ( !videoBuffer.isOpened() )
	{
		cerr << "[ERROR]: Could not open the video named \"" << videoPath << "\"" << endl;
		return -1;
	}
	else
	{
		cout << "Video opened with success" << endl;
	}

	// Video infos
	int size;
	int videoHeight;
	int videoWidth;
	int videoFramerate;
	int videoFrameTotalCount;

	size = MIN(videoBuffer.get(CV_CAP_PROP_FRAME_WIDTH), videoBuffer.get(CV_CAP_PROP_FRAME_HEIGHT)); // TODO : useless ?
	videoHeight = videoBuffer.get(CV_CAP_PROP_FRAME_HEIGHT);
	videoWidth = videoBuffer.get(CV_CAP_PROP_FRAME_WIDTH);
	videoFramerate = videoBuffer.get(CV_CAP_PROP_FPS);
	videoFrameTotalCount = videoBuffer.get(CV_CAP_PROP_FRAME_COUNT);

	cout << "Video infos:" << endl;
	cout << "Resolution: " << videoWidth << "x" << videoHeight << endl;
	cout << "Framerate: " << videoFramerate << "fps" << endl;
	cout << "Total Frame Count: " << videoFrameTotalCount << endl;

	// The exported video file, with the borders
	Size frameSize(static_cast<int>(videoWidth + BORDER_WIDTH), static_cast<int>(videoHeight + BORDER_HEIGHT));
	VideoWriter exportVideoWriter("stabilized_video.avi", CV_FOURCC('P', 'I', 'M', '1'), videoFramerate, frameSize, true);
	// Initialisation : First frame
	Mat previousFrame;
	videoBuffer >> previousFrame;

	// Then, loop on frames
	for (int frameNumber = 2 ; frameNumber <= videoFrameTotalCount; frameNumber++)
	{
		//imshow("previousFrame", reduceImgCouleur(previousFrame, 4));
		cout << "frame " << frameNumber << "/" << videoFrameTotalCount + 1 << " ";
		Mat currentFrame;
		videoBuffer >> currentFrame;

		// Handling end of video
		if ( currentFrame.empty() )
		{
			break;
		} 
		// Handling keyboard events
		if ( quit_signal )
		{
			cout << "QUIT SIGNAL REACHED" << endl;
			exit(0); // exit cleanly on interrupt
		}

		double elapsedTime = (double)cvGetTickCount();
		Mat stabilizedFrame = stabilize(previousFrame, currentFrame);
		elapsedTime = (double)cvGetTickCount() - elapsedTime;
		printf( "detection time = %g ms\n", elapsedTime / ((double)cvGetTickFrequency() * 1000.) );

		Mat singularitiesMask = getMaskOfIrrelevantAreasForSingularities(stabilizedFrame);
		imshow("singularity final mask", scaleGrayFrame(singularitiesMask, 3));

		// Blend frames together to generate some kind of "panorama" construction
		Mat displayFrame = previousFrame.clone();
		//imshow("before", display_frame);
		if ( waitKey(300) >= 0 ) 
		{
			break;
		}

		for( int y = 0 ; y < stabilizedFrame.rows ; y++ )
		{
			for( int x = 0 ; x < stabilizedFrame.cols ; x++ )
			{
				Vec3b pixel = displayFrame.at<Vec3b>(y,x);
				Vec3b pixel2 = stabilizedFrame.at<Vec3b>(y,x);
				pixel[0] = 0.5* pixel[0] + 0.5 * pixel2[0];
				pixel[1] = 0.5* pixel[1] + 0.5 * pixel2[1];
				pixel[2] = 0.5* pixel[2] + 0.5 * pixel2[2];
				displayFrame.at<Vec3b>(y,x) = pixel;
			}
		}

		//imshow("after", display_frame);
		if ( waitKey(300) >= 0 ) 
		{
			break;
		}

		// spacebar control to move to the next frame
		while ( waitKey(100) == -1 ) 
		{}

		previousFrame = currentFrame;
	}

	cout << "Stabilization ended, closing program." << endl;
	return 1;
}