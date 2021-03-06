/*
 * global_controller.h
 *
 *  Created on: 30 mei 2013
 *      Author: Floris
 */

#ifndef GLOBAL_CONTROLLER_H_
#define GLOBAL_CONTROLLER_H_

#define FOLLOW_ANGLE_THRESHOLD 0.1 // x deg to rad
#define FOLLOW_X_THRESHOLD	1 //pixels

#define ROTATE_X_THRESHOLD	1 //pixels
#define ROTATE_Y_THRESHOLD	1 //pixels

#define HOVER_X_THRESHOLD	1 //pixels
#define HOVER_Y_THRESHOLD	1 //pixels
#define HOVER_ANGLE_THRESHOLD 0.1f // x deg to rad

#define ROLL_NEUTRAL		1490
#define ROLL_RIGHT			ROLL_NEUTRAL + 20
#define ROLL_LEFT			ROLL_NEUTRAL - 20

#define PITCH_NEUTRAL		1495
#define PITCH_FORWARD		PITCH_NEUTRAL + 20
#define PITCH_BACKWARD		PITCH_NEUTRAL - 20

#define YAW_NEUTRAL			1500
#define YAW_RIGHT			YAW_NEUTRAL + 70
#define YAW_LEFT			YAW_NEUTRAL - 70

int WaitForNewFrame(int fdfifo, ld_information *info);

#endif /* GLOBAL_CONTROLLER_H_ */
