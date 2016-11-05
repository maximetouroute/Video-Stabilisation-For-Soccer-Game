/*
Maxime Touroute
28 July 2015
*/

#include "stabilization.hpp"

//*************************************************************************
//                              STABILIZATION                             *
//*************************************************************************

/*
Pre processing applied on frames before stabilization
The goal is to enhance camera stabilization precision 
by swipping away interference with the camera movement
@param frame : the frame to process
@return Mat: the frame processed
*/
Mat preProccessingStabilization(const Mat frame)
{
    Mat mask = getMaskOfIrrelevantAreasForCameraStabilization(frame);
    // Blur the areas of the mask
    Mat blurredMask, blurredFrame, resultFrame = frame.clone();
    blur(mask, blurredMask, Size(30, 30) );
    blur(frame, blurredFrame, Size(30, 30) );

    for ( int y = 0 ; y < resultFrame.rows ; y++ )
    {
        for ( int x = 0 ; x < resultFrame.cols ; x++ )
        {
            if ( blurredMask.at<uchar>(y, x) > 0 )
            {
               resultFrame.at<Vec3b>(y, x) = blurredFrame.at<Vec3b>(y, x);
            }
        }
    }
    return resultFrame;
}

/*
The main stabilization method
@param previousFrame : the previousFrame of the video
@param currentFrame : the currentFrame of the video
@return the stabilized image
*/
Mat stabilize(const Mat previousFrame, const Mat currentFrame)
{
    // Add a black border (seems to give better results with it)
    Mat currentFrameBordered = addBlackBorder(currentFrame, BORDER_WIDTH, BORDER_HEIGHT);
    Mat previousFrameBordered = addBlackBorder(previousFrame, BORDER_WIDTH, BORDER_HEIGHT);
    // Pre Process of frames
    Mat processedPreviousFrame = preProccessingStabilization(previousFrameBordered);
    Mat processedCurrentFrame = preProccessingStabilization(currentFrameBordered);
    //imshow("processedFrame", (processedPreviousFrame, 4) );
    //imshow("processedFrame", (processedCurrentFrame, 4) );
    // Movement detection between the two frames
    Mat homography = estimateRigidTransform(processedPreviousFrame, processedCurrentFrame, false);
    // Apply the detected movement
    Mat stabilizedFrame;
    warpAffine(currentFrame, stabilizedFrame, homography, Size(currentFrame.cols, currentFrame.rows), INTER_NEAREST | WARP_INVERSE_MAP);
    ////imshow("currentFrame", (currentFrame, 2) );
    ////imshow("stabilizedFrame", (stabilizedFrame, 2) );
    return stabilizedFrame;
}

/*
Detect... the grass. based on color.
Might not properly work if the players are green
param frame: the frame to compute
@return mask of the field
*/
Mat detectGrass(const Mat frame)
{
    Mat HSV;
    Mat threshold;
    cvtColor(frame, HSV, CV_BGR2HSV);
    inRange(HSV, Scalar(35, 50, 100), Scalar(70, 255, 200), threshold);
    ////imshow("thr", scaleGrayFrame(threshold, 2));
    return threshold;
}

/*
TODO : use a smart algorithm instead (panelDetector() method)
Manually creates the perfect match for France-Sweden match sample
*/
Mat detectScoreOverlayPanel(const Mat frame, int height_offset/* = 0*/, int width_offset/* = 0*/)
{
    Mat mask = cv::Mat::zeros(frame.rows, frame.cols, CV_8U);
    mask(Rect(80 + height_offset, 40 + width_offset, 290, 40)) = 255;
    ////imshow("info layer mask", mask);
    return mask;
}

/*
@return the mask of areas to not use for camera stabilization

The mask includes:
    * the players (TODO and a few static objects on the field that should be kept)
    * the informations layer (stuff that isn't part of the video like the timer)
    and excludes:
    * the public
*/
Mat getMaskOfIrrelevantAreasForCameraStabilization(const Mat frame)
{
    Mat maskGrass = detectGrass(frame);
    ////imshow( "maskGrass step 1/3", scaleGrayFrame(maskGrass , 2));

    // Dilate to remove any left artefacts on the field (field lines)
    dilateMask(maskGrass, FIELD_MASK_DILATION_SIZE);
    ////imshow( "maskGrass step 2/3", scaleGrayFrame(maskGrass , 2));

    // Erode to clearly separate elements that are not part of the field (mostly players)
    erodeMask(maskGrass, FIELD_MASK_EROSION_SIZE);
    ////imshow( "maskGrass step 3/3", scaleGrayFrame(maskGrass , 2));

    Mat maskPublic = detectGrass(frame);
    ////imshow("maskPublic step 1/4", scaleGrayFrame(maskPublic, 2));

    // Dilate to remove any left artefacts out the field (in the public)
    erodeMask(maskPublic, PUBLIC_MASK_EROSION_SIZE);
    ////imshow( "maskPublic step 2/4", scaleGrayFrame(maskPublic , 2));

    // Dilate to remove everything on the field
    dilateMask(maskPublic, PUBLIC_MASK_DILATION_SIZE);
    ////imshow( "maskPublic step 3/4", scaleGrayFrame(maskPublic , 2));

    // Inverting the mask
    maskPublic = 255 - maskPublic;
    ////imshow( "maskPublic step 4/4", scaleGrayFrame(maskPublic , 2));

    Mat maskScore = detectScoreOverlayPanel(frame, BORDER_HEIGHT/2, BORDER_WIDTH/2);
    
    // We use those three masks to create the final mask
    Mat finalMask = cv::Mat::zeros(frame.rows, frame.cols, CV_8U);
    for ( int y = 0 ; y < frame.rows ; y++ )
    {
        for ( int x = 0 ; x < frame.cols ; x++ )
        {
            // add everything that is not grass
            if ( maskGrass.at<uchar>(y, x) < 10 )
            {
                finalMask.at<uchar>(y, x) = 255;
            }

            // Remove the public 
            if ( maskPublic.at<uchar>(y, x) > 250 )
            {
                finalMask.at<uchar>(y, x) = 0;
            }

            // Add infosLayer mask
            if ( maskScore.at<uchar>(y, x) > 250 )
            {
                finalMask.at<uchar>(y, x) = 255;
            }
        }
    }
    //imshow("final mask camstab", scaleGrayFrame(finalMask,3));

    // Visualization    
    Mat displayFrame = frame.clone();
    for ( int y = 0 ; y < displayFrame.rows ; y++ )
    {
        for ( int x = 0 ; x < displayFrame.cols ; x++ )
        {
            if ( maskGrass.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[1] = pixel[1] * 0.3 + 255 * 0.7;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
            if ( maskPublic.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[0] = pixel[0] * 0.2 + 255 * 0.8;
                pixel[1] = pixel[1] * 0.2 + 255 * 0.8;
                pixel[2] = pixel[2] * 0.2 + 255 * 0.8;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
            if ( maskScore.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[2] = pixel[2] * 0.2 + 250 * 0.8;
                pixel[1] = pixel[1] * 0.6;
                pixel[0] = pixel[0] * 0.6;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
        }
    }
    //imshow("displayFrame", (displayFrame, 2));
    return finalMask;
}

/*
@return the mask of areas to not use for singularities
The mask includes:
    * the informations layer (stuff that isn't part of the video like the timer)
    * the public
    and excludes:
    * the players (with a big area arround them)
*/
Mat getMaskOfIrrelevantAreasForSingularities(const Mat frame)
{
    Mat maskPublic = detectGrass(frame);
    ////imshow("maskPublic step 1/4", scaleGrayFrame(maskPublic, 2));

    // Dilate to remove any left artefacts out the field (in the public)
    erodeMask(maskPublic, PUBLIC_MASK_EROSION_SIZE);
    ////imshow( "maskPublic step 2/4", scaleGrayFrame(maskPublic , 2));

    // Dilate to remove everything on the field
    dilateMask(maskPublic, PUBLIC_MASK_DILATION_SIZE);
    ////imshow( "maskPublic step 3/4", scaleGrayFrame(maskPublic , 2));

    // Inverting the mask
    maskPublic = 255 - maskPublic;
    //imshow( "maskPublic step 4/4", scaleGrayFrame(maskPublic , 2));

    Mat maskScore = detectScoreOverlayPanel(frame);
    Mat maskBorders = getBorderMask(frame.rows, frame.cols, SINGULARITY_MASK_BORDER);
    // We use those  masks to create the final mask
    Mat finalMask = cv::Mat::zeros(frame.rows, frame.cols, CV_8U);
    for ( int y = 0 ; y < frame.rows ; y++ )
    {
        for ( int x = 0 ; x < frame.cols ; x++ )
        {
            // Add the public 
            if ( maskPublic.at<uchar>(y, x) > 250 )
            {
                finalMask.at<uchar>(y, x) = 255;
            }
            // Add infosLayer mask
            if ( maskScore.at<uchar>(y, x) > 250 )
            {
                finalMask.at<uchar>(y, x) = 255;
            }
            // Add infosLayer mask
            if ( maskBorders.at<uchar>(y, x) > 250 )
            {
                finalMask.at<uchar>(y, x) = 255;
            }
        }
    }

    // TODO: add some kind of borders ?
    ////imshow("final mask", finalMask);

    // Visualisation    
    Mat displayFrame = frame.clone();
    for ( int y = 0 ; y < displayFrame.rows ; y++ )
    {
        for ( int x = 0 ; x < displayFrame.cols ; x++ )
        {
            if ( maskPublic.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[0] = pixel[0] * 0.2 + 255 * 0.8;
                pixel[1] = pixel[1] * 0.2 + 255 * 0.8;
                pixel[2] = pixel[2] * 0.2 + 255 * 0.8;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
            if ( maskScore.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[2] = pixel[2] * 0.2 + 250 * 0.8;
                pixel[1] = pixel[1] * 0.6;
                pixel[0] = pixel[0] * 0.6;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
            if ( maskBorders.at<uchar>(y, x) > 250 )
            {
                Vec3b pixel = displayFrame.at<Vec3b>(y, x);
                pixel[2] = pixel[2];
                pixel[1] = pixel[1];
                pixel[0] = pixel[0] * 0.5 + 255*0.5;
                displayFrame.at<Vec3b>(y, x) = pixel;
            }
        }
    }
    //imshow("mask singularities display", (displayFrame, 2));
    return finalMask;
}



/*
Status : WIP

delete all pixels that have moved between the two frames
IMPORTANT: the two frames must have the same dimension
EVEN MORE IMPORTANT: if the info layer is not perfectly opaque, might give terrible results
@param previousFrame : the previousFrame of the video (COLOR)
@param currentFrame : the currentFrame of the video (COLOR)

@return the processed image
*/
/*
Mat panelDetector(Mat previousFrame, Mat currentFrame)
{
    // The sensibility of the thing. Between 0-255
    // Even the panel pixels changes a bit so better not put a 0 here.
    const int SENSIBILITY = 20;

    // Black and white matrices
    Mat bw_previousFrame, bw_currentFrame;
    Mat processedFrame = currentFrame.clone();

    cvtColor(previousFrame, bw_previousFrame, CV_BGR2GRAY);
    cvtColor(currentFrame, bw_currentFrame, CV_BGR2GRAY);


    // Black & White method: 30ms on my laptop, might take a few seconds to identify the panel area
    /*for (int y = 0 ; y < bw_currentFrame.rows ; y++)
    {
        for (int x = 0 ; x < bw_currentFrame.cols ; x++)
        {
            // TODO: a mask ? so we don't go back twice on the same *bip*

           // if( bw_currentFrame.at<uchar>(y, x) != 0 )
            {
                // If the pixel value has changed A LOT, it's moving !
                // TODO: maybe KEEP the PREVIOUS value or return the PREVIOUS frame just to be sure it's not changing ?
                if ( (bw_currentFrame.at<uchar>(y, x) < bw_previousFrame.at<uchar>(y, x)-SENSIBILITY)
                    ||  bw_currentFrame.at<uchar>(y, x) > bw_previousFrame.at<uchar>(y, x)+SENSIBILITY )
                {
                    // We delete it. On the colour frame
                    currentFrame.at<Vec3b>(y,x) = Vec3b(0,0,0);
                }
            }
        }
    }

    // Color method
    for ( int y = 0 ; y < currentFrame.rows ; y++ )
    {
        for ( int x = 0 ; x < currentFrame.cols ; x++ )
        {
            // TODO: a mask ? so we don't go back twice on the same *bip*
            // if(bw_currentFrame.at<uchar>(y, x) != 0)
            {
                // If the pixel value has changed
                if ( (currentFrame.at<Vec3b>(y, x)[0] < previousFrame.at<Vec3b>(y, x)[0] - SENSIBILITY)
                        ||  currentFrame.at<Vec3b>(y, x)[0] > previousFrame.at<Vec3b>(y, x)[0] + SENSIBILITY )
                {
                    // We delete it. On the colour frame
                    currentFrame.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
                }

                // If the pixel value has changed
                else if ( (currentFrame.at<Vec3b>(y, x)[1] < previousFrame.at<Vec3b>(y, x)[1] - SENSIBILITY)
                          ||  currentFrame.at<Vec3b>(y, x)[1] > previousFrame.at<Vec3b>(y, x)[1] + SENSIBILITY )
                {
                    // We delete it. On the colour frame
                    currentFrame.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
                }

                // If the pixel value has changed
                else if ( (currentFrame.at<Vec3b>(y, x)[2] < previousFrame.at<Vec3b>(y, x)[2] - SENSIBILITY)
                          ||  currentFrame.at<Vec3b>(y, x)[2] > previousFrame.at<Vec3b>(y, x)[2] + SENSIBILITY )
                {
                    // We delete it. On the colour frame
                    currentFrame.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
                }
            }
        }
    }
    return currentFrame;
}*/


//*************************************************************************
//                                  UTILS                                 *
//*************************************************************************

/*
@param mask : the mask to erode
@param erosionSize : the size of the erosion
*/
void erodeMask(Mat& mask, int erosionSize)
{
    int erosion_type = MORPH_RECT;
    Mat erode_element = getStructuringElement( erosion_type, Size( 2 * erosionSize + 1, 2 * erosionSize + 1 ),
                        Point( erosionSize, erosionSize ) );
    erode( mask, mask, erode_element );
}

/*
@param mask: the mask to dilate
@param dilationSize: the size of the dilation
*/
void dilateMask(Mat& mask, int dilationSize)
{
    int dilation_type = MORPH_RECT;
    Mat dilate_element = getStructuringElement( dilation_type, Size( 2 * dilationSize + 1, 2 * dilationSize + 1 ),
                         Point( dilationSize, dilationSize ) );
    dilate( mask, mask, dilate_element );
}

/*
Generate a bordered mask
@param rows: row size of returned mask
@param cols: col size of returned mask
@param borderSize: size of 
@return mask with white border
*/
Mat getBorderMask(const int rows, const int cols, const int borderSize)
{
    // We use those three masks to create the final mask
    Mat finalMask = cv::Mat::zeros(rows, cols, CV_8U);
    Mat mini_mask = cv::Mat::zeros(rows - 2 * borderSize, cols - 2 * borderSize, CV_8U);
    finalMask = 255;
    mini_mask.copyTo(finalMask(Rect(borderSize, borderSize, cols - 2 * borderSize, rows - 2 * borderSize)));
    //imshow("getBorderMask result", scaleGrayFrame(finalMask,2));
    return finalMask;
}

/*
wrap the frame in a bigger frame with margins
@param frame: frame to add border to
@return the bordered frame
*/
Mat addBlackBorder(Mat frame, int borderWidth, int borderHeight)
{
    Mat borderFrame;
    Size frameSize(static_cast<int>(frame.cols + borderWidth), static_cast<int>(frame.rows + borderHeight));
    borderFrame = Mat::zeros(frameSize, frame.type());
    frame.copyTo(borderFrame(Rect(borderWidth / 2, borderWidth / 2, frame.cols, frame.rows)));
    return borderFrame;
}

/*
Add axis to the frame
@param frame: the frame to draw an
*/
void drawAxis(Mat& frame)
{
    line(frame, Point(frame.cols / 2, 0), Point(frame.cols / 2, frame.rows), CV_RGB(255, 50, 50), 2, 8);
    line(frame, Point(0, frame.rows / 2), Point(frame.cols, frame.rows / 2), CV_RGB(255, 50, 50), 2, 8);
}

/*
Reduce the image size (black & white only)
@param frame: the frame to resize
@param scale: an integer reducing scale to apply
@return the scaled frame
*/
Mat scaleGrayFrame(Mat frame, int scale)
{
    Mat res(frame.rows / scale, frame.cols / scale, CV_8UC1, Scalar::all(0));
    // 4dim factor
    Scalar s;
    Mat tmp;
    // resample image
    for ( int i = 0; i < res.rows; i++ )
    {
        for ( int j = 0; j < res.cols; j++ )
        {
            // Range: used to specify a row or a column span in a matrix
            s = sum(frame(Range(i * scale, (i + 1) * scale), Range(j * scale, (j + 1) * scale)));
            res.at<uchar>(i, j) = s[0] / (scale * scale);
        }
    }
    return res;
}

/*
Reduce the image size (color only)
@param frame: the frame to resize
@param scale: an integer reducing scale to apply
@return the scaled frame
*/
Mat saleColorFrame(Mat frame, int scale)
{
    Mat res(frame.rows / scale, frame.cols / scale, CV_8UC3, Scalar::all(0));
    Scalar s;
    Mat tmp;
    for ( int i = 0; i < res.rows; i++ ) 
    {
        for ( int j = 0; j < res.cols; j++ ) 
        {
            s = sum(frame(Range(i * scale, (i + 1) * scale), Range(j * scale, (j + 1) * scale)));
            res.at<Vec3b>(i, j) = Vec3b(s[0] / (scale * scale), s[1] / (scale * scale), s[2] / (scale * scale));
        }
    }
    return res;
}