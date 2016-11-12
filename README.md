# Stabilisation-For-Soccer-Game

This is a code snippet from a project aiming to highlight events in soccer game videos. This snippet aims separate camera movements and subject movements based on the common characteristics of every soccer game video.

The main algorithm based on an optical flow analysis can then be more accurate by analyzing players movements without the noise infered by camera movements

Developped with OpenCV 2.

Note: event highlighting algorithm and video samples are not included in this repo.



## How to use it

You must have openCV installed. Then, execute those commands at the root project folder :
```shell
cmake .
make Main
./Main
```


## Some results

This is the kind of video frame I've been working with. Low quality images, with compression artifacts :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/1.png)

With a simple hue-based color detection, and a bit of mathematical morphology, I can isolate the grass :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/2.png)

I've also been exploring background substraction algorithms to isolate the score panels :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/4.png)

With a bit of deduction and work on my masks, I can isolate multiple areas : the grass, the players, the public, and the score panel :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/3.png)

Then, I can ommit the players from my camera movement detection algorithm, as they are a source of noise. Here is a constructed panorama based on multiple stabilized frames blended together :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/5.png)

This panorama highlights the limits of my method. The low quality of the video and some ommisions such as public movement and video-ads movements causes an decreasing quality in the stabilisation when proceeded on a high number of frames. This isn't an issue here, as we only aim to remove camera movement between two or three frames.

The event highlight algorithm was giving those results before video stabilisation (every square represent a detected event) :
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/6.png)


Multiple false detections can be seen on the field lines, the public, and the score panel. Moreover, correct detections are crushed by the camera movements, usually moving in the same direction as the players.


Here is the results of the event highlight algorithm after video stabilisation : 

![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/7.png)
![](https://github.com/maximetouroute/Video-Stabilisation-For-Soccer-Game/blob/master/img/8.png)

False detections are less proeminent, and game events now jump out of the frame ! :soccer:
