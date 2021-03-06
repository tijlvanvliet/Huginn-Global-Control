// ifconfig to show ip
// netstat -tulnap to show used ports and pid
// kill -KILL <pid>
// send color image, works with StreamClient03

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// CV includes
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

using namespace cv;
using namespace std;

// config
#include "../common.h"
#include "../config.h"
#include "line_detection.h"

long long debut_mesure;
long long fin_mesure;

IplImage*	img_shared;

int main(int argc, char** argv)
{
	char key = 0;
#ifdef SHOW_WINDOW
	cvNamedWindow("Example3", CV_WINDOW_AUTOSIZE);
#endif
	
#if PLATFORM == PLATFORM_x86
	CvCapture* capture = cvCreateFileCapture(argv[1]);
	printf("Input type = video\n\r");
#else
	CvCapture* capture = cvCreateCameraCapture(-1);
	printf("Input type = webcam\n\r");
#endif
	if (!capture) {
		quit("cvCapture failed", 1);
	}

#ifdef	WEBCAM_RESIZE
	IplImage* img_interm = cvQueryFrame(capture);
	IplImage* img_in = cvCreateImage(cvSize( img_interm->width / 2, img_interm->height / 2 ), img_interm->depth, img_interm->nChannels );
	printf("Webcam is resized to %d x %d\n\r",(int)(img_interm->width/2),(int)(img_interm->height/2));
#else
	IplImage* img_in = cvQueryFrame(capture);
#endif
	IplImage* img_pgm = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 1); //gray
	img_shared = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 3); //color
	CvSize size = cvGetSize(img_in);
	
	cvZero(img_pgm);
	cvZero(img_shared);

	// for text:
#if (DEBUG_LEVEL <= 1) && (PLATFORM == PLATFORM_x86)
	char text[50];
	int fontFace = FONT_HERSHEY_PLAIN;
	double fontScale = 1;
	int thickness = 1;
#endif

	/* create the FIFO (named pipe) */
	//unlink(LINE_DETECT_FIFO);
	mkfifo(LINE_DETECT_FIFO, 0666);

	// open fifo
	int fd_ldfifo = open(LINE_DETECT_FIFO, O_WRONLY/* | O_NONBLOCK*/);
	if (fd_ldfifo < 0)
	{
		printf("ERROR opening fifo %s, error nr: %d\n", LINE_DETECT_FIFO, fd_ldfifo);
		return -1;
	}

	// for line detection
	Mat dst, cdst;

	while(1) {
#ifdef SHOW_WINDOW
		if(key == 27)
		{
#endif
#if DEBUG_LEVEL <= 1
			printf("\n\n");
#endif
			/* get a frame from camera */
#ifdef WEBCAM_RESIZE
			img_interm = cvQueryFrame(capture);
			cvResize((CvArr*)img_interm, (CvArr*)img_in, INTER_LINEAR);
#else
			img_in = cvQueryFrame(capture);
#endif
			
			//gray scale image for edge detection
			cvCvtColor(img_in, img_pgm, CV_BGR2GRAY);
			Mat src = img_pgm;
			// make edges smoother
			//medianBlur(src,src, 3); //uneven and larger than 1
			medianBlur(src,src, 3);
			// create b&w 
			Mat bw = src > 200;
			// edge detection
			Canny(bw, dst, 50, 100, 3); //GRAY output format
			cdst = img_in;
			//cvtColor(dst, cdst, COLOR_GRAY2BGR); //convert to color to be able to draw color lines

			vector<Vec4i> vectors;
			LineStruct lines[2];

			HoughLinesP(dst, vectors, 1, CV_PI/180, 50, 10, 10);
			VectorsToLines(vectors, &lines[0], &lines[1]);
			// line with lowest angle is main line
			if(lines[0].n > 0 && lines[1].n > 0)
			{
				LineStruct tmp;
				if(abs(lines[0].angle) > abs(lines[1].angle))
				{
					tmp = lines[0];
					lines[0] = lines[1];
					lines[1] = tmp;
				}
				// else nothing to do
			}
			
			int sq_size = 24;
			int close_displacement = 25;
			int far_displacement = 100;
			int mean_threshold = 100;
			
			Point crossing;
			LineDetectedPoint line0detected;
			LineDetectedPoint line1detected;
			
			LineElement line_element = le_unknown;
			LineStruct *main_line;
			ld_information ld_info;
			
			//DetectLinePresence(const Mat &bw, const LineStruct* line, const Point crossing, int dist_close, int dist_far, int threshold, int sq_size)
			if(lines[0].n > 0 && lines[1].n > 0)
			{
				crossing = LinesCrossing(&lines[0], &lines[1]);
				// angle line 0 check on the side of the crossing for line 1
				line1detected = DetectLinePresence(bw, lines[0].angle, crossing, close_displacement, far_displacement, mean_threshold, sq_size);
				line0detected = DetectLinePresence(bw, lines[1].angle, crossing, close_displacement, far_displacement, mean_threshold, sq_size);
				
				if ((line0detected == ld_close && line1detected == ld_close)
				|| (line0detected == ld_all && line1detected == ld_close)
				|| (line0detected == ld_close && line1detected == ld_all)
				|| (line0detected == ld_all && line1detected == ld_all))
				{
					main_line = &lines[0];
					ld_info.element = (int)le_cross;
					
					ld_info.mode = MODE_LINE_HOVER;
					ld_info.angle_sp = ANGLE_OFFSET;
					ld_info.angle_cv = lines[0].angle;
					ld_info.x_sp = X_OFFSET;
					ld_info.x_cv = crossing.x - size.width/2;
					ld_info.y_sp = Y_OFFSET;
					ld_info.y_cv = crossing.y - size.height/2;
				}
				// detect cross on line
				else if (line0detected == ld_all && line1detected == ld_close)
				{
					main_line = &lines[0];
					ld_info.element = (int)le_cross;
					
					ld_info.mode = MODE_LINE_HOVER;
					ld_info.angle_sp = ANGLE_OFFSET;
					ld_info.angle_cv = lines[0].angle;
					ld_info.x_sp = X_OFFSET;
					ld_info.x_cv = crossing.x - size.width/2;
					ld_info.y_sp = Y_OFFSET;
					ld_info.y_cv = crossing.y - size.height/2;
				}
				else if (line0detected == ld_close && line1detected == ld_all)
				{
					// in rotate
					main_line = &lines[1];
					ld_info.element = (int)le_cross;
					
					ld_info.mode = MODE_LINE_HOVER;
					ld_info.angle_sp = ANGLE_OFFSET;
					ld_info.angle_cv = lines[0].angle;
					ld_info.x_sp = X_OFFSET;
					ld_info.x_cv = crossing.x - size.width/2;
					ld_info.y_sp = Y_OFFSET;
					ld_info.y_cv = crossing.y - size.height/2;
				}
				// detect corner
				else if ((line0detected == ld_plus || line0detected == ld_minus)
				         && (line1detected == ld_plus || line1detected == ld_minus))
				{
					if (line0detected == ld_minus && line1detected == ld_plus)
					{
						ld_info.element = (int)le_corner_right;
					}
					else if (line0detected == ld_minus && line1detected == ld_minus)
					{
						ld_info.element = (int)le_corner_left;
					}
					else if (line0detected == ld_plus && line1detected == ld_plus)
					{
						ld_info.element = (int)le_corner_right_taken;
					}
					else if (line0detected == ld_plus && line1detected == ld_minus)
					{
						ld_info.element = (int)le_corner_left_taken;
					}
				}
				// detect T end
				else if (line0detected == ld_minus && line1detected == ld_close)
				{
					ld_info.element = (int)le_end;
				}
				// detect T begin
				else if (line0detected == ld_plus && line1detected == ld_close)
				{
					ld_info.element = (int)le_begin;
				}
			}
			else if(lines[0].n > 0)
			{
				// line follow
				ld_info.element = (int)le_none;
				
				ld_info.mode = MODE_LINE_FOLLOW;
				ld_info.angle_sp = ANGLE_OFFSET;
				ld_info.angle_cv = lines[0].angle;
				ld_info.x_sp = X_OFFSET;
				ld_info.x_cv = lines[0].location.x - size.width/2;
				ld_info.y_sp = Y_OFFSET;
				ld_info.y_cv = lines[0].location.y - size.height/2;
			}
			else
			{
				// no lines found
			}

			// send control info to controller
			int bytes_written = write(fd_ldfifo, &ld_info, sizeof(ld_information));
				
#if DEBUG_LEVEL <= 1
			char le_strings[][50] = {"le_cross","le_corner_left","le_corner_right","le_corner_left_taken","le_corner_right_taken","le_end","le_begin","le_none","le_unknown"};
			
			printf("line_element = %s\n", le_strings[ld_info.element-1]);
			printf("value line0detected mean = %x\n", line0detected);
			printf("value line1detected mean = %x\n", line1detected);
		
			// printf("(x,y)_0 = (%d,%d), (dx,dy)_0 = (%d,%d)\n", lines[0].location.x, lines[0].location.y, lines[0].vector.x, lines[0].vector.y);
			// printf("(x,y)_1 = (%d,%d), (dx,dy)_1 = (%d,%d)\n", lines[1].location.x, lines[1].location.y, lines[1].vector.x, lines[1].vector.y);
			// printf("a0=%f, b0=%f, a1=%f, b1=%f\n", a0, b0, a1, b1);
			// printf("cross (x,y) = (%d,%d)\n", cross.x, cross.y);
			
#if PLATFORM == PLATFORM_x86
			//rectangle(bw, cvRect(50, 50, 10, 100), Scalar(255,255,255));
			circle(cdst, crossing, 4, Scalar(0,0,255), -1, 8, 0 );

			line(cdst, lines[0].location, lines[0].location + lines[0].vector ,Scalar(255,0,0), 3, CV_AA);
			line(cdst, lines[1].location, lines[1].location + lines[1].vector ,Scalar(0,255,0), 3, CV_AA);
			img_shared->imageData = (char *)src.data;
			//cvFlip(img_shared, img_shared, 1);
			//cvShowImage("Example3", img_shared);
			imshow("Example3", bw);
#endif
			printf("%s \nangle=%f\nn=%d\n", "line0", lines[0].angle, lines[0].n);
			printf("%s \nangle=%f\nn=%d\n", "line1", lines[1].angle, lines[1].n);
#endif
			printf("fps : %.2f\n", calculeFrameRate());

#ifdef SHOW_WINDOW
		}
		key = cvWaitKey(33);
#endif
	}

#ifdef SHOW_WINDOW
	destroyAllWindows();
#endif

	/* free memory */
	close(fd_ldfifo);
	if (capture) cvReleaseCapture(&capture);
	if (img_in) cvReleaseImage(&img_in);
	if (img_pgm) cvReleaseImage(&img_pgm);
	quit(NULL, 0);
}

int VectorsToLines(const vector<Vec4i>& vectors, LineStruct* line0, LineStruct* line1)
{
	line0->vector = Point(0,0);
	line0->location = Point(0,0);
	line0->n = 0;
	line0->angle = 0;
	line1->vector = Point(0,0);
	line1->location = Point(0,0);
	line1->n = 0;
	line1->angle = 0;

	// loop through all lines found in image
	for(size_t i = 0; i < vectors.size(); i++)
	{
		Vec4i l = vectors[i];
		LineStruct line_temp;

		line_temp.vector.x = l[2]-l[0]; // delta x
		line_temp.vector.y = l[3]-l[1]; // delta y
		// for center
		line_temp.location.x = l[2] + l[0];
		line_temp.location.y = l[3] + l[1];

		if(line_temp.vector.y < 0)
		{
			line_temp.vector_lh.x = -line_temp.vector.x;
			line_temp.vector_lh.y = -line_temp.vector.y;
		}
		else
		{
			line_temp.vector_lh.x = line_temp.vector.x;
			line_temp.vector_lh.y = line_temp.vector.y;
		}

		float angle = calcAngle(line_temp.vector);

		LineStruct *dest_line;

		// if line1 is still empty add it to line1
		if(line0->n == 0)
			dest_line = line0;
		else if(belongs_to_line(angle, line0->angle, ANGLE_THRESHOLD))
			dest_line = line0;
		// if line2 is empty and it doesn't belong to line1 add it to line 2
		else if(line1->n == 0)
			dest_line = line1;
		else if(belongs_to_line(angle, line1->angle, ANGLE_THRESHOLD))
			dest_line = line1;

		// if y difference == 0 then processed by calcAngle()
		// if x difference and y difference == 0 then reject line
		if((line_temp.vector.x != 0) && (line_temp.vector.y != 0))
		{
			dest_line->vector.x += line_temp.vector.x;
			dest_line->vector.y += line_temp.vector.y;
			dest_line->location.x += line_temp.location.x;
			dest_line->location.y += line_temp.location.y;
			dest_line->vector_lh.x += line_temp.vector_lh.x;
			dest_line->vector_lh.y += line_temp.vector_lh.y;

			dest_line->angle = calcAngle(dest_line->vector_lh);
			dest_line->n++;

#if DEBUG_LEVEL <= 1 && 0
			int current_line = 1;
			if(dest_line == &detected_line2)
				current_line = 2;

			printf("vector %d angle = %f\n", i, angle);
			printf("vector %d pos = s(%d,%d) e(%d,%d)\n", i, l[0], l[1], l[2], l[3]);
			printf("angle line %d = %f\n", current_line, dest_line->angle);
#if PLATFORM == PLATFORM_x86	
			sprintf(text, "%d", i);
			putText(cdst, text, Point(l[0], l[1]), fontFace, fontScale,Scalar(0,0,0), thickness, 8);

			if(current_line == 2)
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,255), 3, CV_AA);
			else
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
#endif
#endif
		}

	}

	// clear screen
#if DEBUG_LEVEL <= 1 && 0
	printf("Line1 size = %d\n",line0->n);
	printf("Line2 size = %d\n",line1->n);
#endif

	if(line0->n > 0)
	{
		line0->vector.x = (int)(line0->vector.x/line0->n);
		line0->vector.y = (int)(line0->vector.y/line0->n);
		line0->location.x = (int)(line0->location.x/(2*line0->n));
		line0->location.y = (int)(line0->location.y/(2*line0->n));

#if DEBUG_LEVEL <= 1 && 0
		printf("Start = (%d,%d)\nEnd = (%d,%d)\n"
			"av_abs = (%d,%d)\nav_sum = (%d,%d)\nAngle = %.3f degree\n",
			line0->av_abs.x, line0->av_abs.y, Point(line0->av_abs + line0->av_sum).x, Point(line0->av_abs + line0->av_sum).y,
			line0->av_abs.x, line0->av_abs.y, line0->av_sum.x, line0->av_sum.y, line0->av_angle*180/CV_PI);

#if PLATFORM == PLATFORM_x86	
		// print text to the image
		sprintf(text, "Angle = %.3f", line0->av_angle*180/CV_PI);
		Point textOrg1(20,20);
		putText(cdst, text, textOrg1, fontFace, fontScale,Scalar(0,255,0), thickness, 8);
		//draw average vector
		line( cdst, line0->av_abs, line0->av_abs + line0->av_sum,Scalar(255,0,0), 3, CV_AA);
		//draw reference line
		line( cdst, Point(size.width/2,0), Point(size.width/2,size.height),Scalar(255,255,255), 2, CV_AA);
#endif
		// calculate deviation from centre
#endif
		// float distance_average = line0->av_abs.x;
		// float centre = size.width/2;
		// float deviation = centre - distance_average;
		// float deviation_normalized = (deviation/size.width) * 100;

#if DEBUG_LEVEL <= 1 && 0
		printf("Distance average = %.3f\nCentre = %.3f\nDeviation = %.3f = %.3f [percent]\n",
			 distance_average, centre, deviation, deviation_normalized);
#if PLATFORM == PLATFORM_x86	
		// print two dots to the image
		 Point circle_center;
		 // indicate center:
		 circle_center.x = size.width/2;
		 circle_center.y = size.height/2;
		 //make dot white for reference
		 circle( cdst, circle_center, 4, Scalar(255,255,255), -1, 8, 0 );
		 // indicate calculated line center
		 circle_center.x = distance_average;
		 circle_center.y = line0->av_abs.y;
		 //make dot green
		 circle( cdst, circle_center, 4, Scalar(0,255,0), -1, 8, 0 );
#endif
#endif
	}
	
	if(line1->n > 0)
	{
		line1->vector.x = (int)(line1->vector.x/line1->n);
		line1->vector.y = (int)(line1->vector.y/line1->n);
		line1->location.x = (int)(line1->location.x/(2*line1->n));
		line1->location.y = (int)(line1->location.y/(2*line1->n));
	}

	/* print the width and height of the frame, needed by the client */
#if DEBUG_LEVEL <= 1 && 0
	//pthread_mutex_lock(&mutex);
	img_shared->imageData = (char *)cdst.data;
	//is_data_ready = 1;
	/* also display the video here on server */
#ifdef SHOW_WINDOW
	cvShowImage("Example3", img_shared);
#endif
	int img_shared_size = img_shared->imageSize;
	//pthread_mutex_unlock(&mutex);

	fprintf(stdout, "width:  %d\nheight: %d\nsize: %d\n", img_in->width, img_in->height, img_shared_size);
#endif
}

Point LinesCrossing(const LineStruct* line0, const LineStruct* line1)
{
	float a0 = (float)line0->vector.y/(float)line0->vector.x;
	float a1 = (float)line1->vector.y/(float)line1->vector.x;
	Point cross;
	
	float b0 = (float)line0->location.y - a0*(float)line0->location.x;
	float b1 = (float)line1->location.y - a1*(float)line1->location.x;
	float x_temp = (b1 - b0)/(float)(a0-a1);
	cross.x = x_temp;
	cross.y = (int)(a0 * x_temp + b0);
	
	return cross;
}

LineDetectedPoint DetectLinePresence(Mat &bw, const float angle, const Point crossing, int dist_close, int dist_far, int threshold, int sq_size)
{
	double mean_roi;
	Point p;
	Mat rect_roi;
	LineDetectedPoint result = ld_none;
	LineDetectedPoint bit_to_set = ld_none;
	for (int i = 0; i < 4; i++)
	{
		switch(i)
		{
			// bit 0 = plus_close
			case 0:
				p = Point(crossing.x + (double)dist_close*cos(angle), crossing.y + (double)dist_close*sin(angle));
				bit_to_set = ld_plus_close;
				break;
			case 1:
				p = Point(crossing.x - (double)dist_close*cos(angle), crossing.y - (double)dist_close*sin(angle));
				bit_to_set = ld_minus_close;
				break;
			case 2:
				p = Point(crossing.x + (double)dist_far*cos(angle), crossing.y + (double)dist_far*sin(angle));
				bit_to_set = ld_plus_far;
				break;
			case 3:
			default:
				p = Point(crossing.x - (double)dist_far*cos(angle), crossing.y - (double)dist_far*sin(angle));
				bit_to_set = ld_minus_far;
				break;
		}
				
		if(p.x-sq_size/2 < 0 || p.x > bw.cols-sq_size/2
			|| p.y-sq_size/2 < 0 || p.y > bw.rows-sq_size/2)
		{
			//printf("crossing {x,y} = {%d,%d}\n",crossing.x,crossing.y);
			//printf("ext pres {x,y,i} = {%d,%d,%d}\n",p.x,p.y,i);
			continue;
		}
#if PLATFORM == PLATFORM_x86	
		rectangle(bw, cvRect(p.x-sq_size/2, p.y-sq_size/2, sq_size, sq_size), Scalar(255,255,255));
#endif
		rect_roi = bw(cvRect(p.x-sq_size/2, p.y-sq_size/2, sq_size, sq_size));
		mean_roi = mean(rect_roi)[0];
		if(mean_roi > threshold)
			result = (LineDetectedPoint)(result | bit_to_set);
	}
	
	return result;
}

bool belongs_to_line(float vector_angle, float line_angle, float margin)
{
	bool ret = false;
	// if region of line1 crosses -90 deg
	if(line_angle - margin < -CV_PI/2)
	{
		if(vector_angle < line_angle + margin ||
				vector_angle > line_angle - margin + CV_PI)
			ret = true;
	}
	// if region of line1 crosses +90 deg
	else if(line_angle + margin > CV_PI/2)
	{
		if(vector_angle > line_angle - margin ||
				vector_angle < line_angle + margin - CV_PI)
			ret = true;
	}
	else
	{
		if(vector_angle > line_angle - margin &&
				vector_angle < line_angle + margin)
			ret = true;
	}

	return ret;
}

void draw_detected_line(Mat dst, LineStruct line)
{

}

/**
 * this function provides a way to exit nicely from the system
 */
void quit(char* msg, int retval)
{
	if (retval == 0) {
		fprintf(stdout, (msg == NULL ? "" : msg));
		fprintf(stdout, "\n");
	} else {
		fprintf(stderr, (msg == NULL ? "" : msg));
		fprintf(stderr, "\n");
	}

	//if (capture) cvReleaseCapture(&capture);
	if (img_shared) cvReleaseImage(&img_shared);
	//if (img_in) cvReleaseImage(&img_in);
	//if (img_pgm) cvReleaseImage(&img_pgm);
		
	/* remove the FIFO */
	unlink(LINE_DETECT_FIFO);

	exit(retval);
}

float calcAngle(Point delta)
{
	float angle;

	if(delta.y == 0) angle = CV_PI/2;
	else if(delta.x == 0) angle = 0;
	else angle = (float)atan(((double)delta.x/-(double)delta.y));

	return angle;
}

long long getTimeMillis()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	long long l = tv.tv_usec / 1000;
	l += tv.tv_sec * 1000;

	return l;
}

double calculeFrameRate()
{
    fin_mesure = getTimeMillis();

    double fps = 1000.0 / (fin_mesure - debut_mesure);

    debut_mesure = fin_mesure;

    return fps;
}


