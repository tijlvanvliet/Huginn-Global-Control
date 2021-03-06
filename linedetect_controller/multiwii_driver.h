/*
 * multiwii_driver.h
 *
 *  Created on: 31 mei 2013
 *      Author: Floris
 */

#ifndef MULTIWII_DRIVER_H_
#define MULTIWII_DRIVER_H_

#define MSP_IDENT                100   //out message         multitype + multiwii version + protocol version + capability variable
#define MSP_STATUS               101   //out message         cycletime & errors_count & sensor present & box activation & current setting number
#define MSP_RAW_IMU              102   //out message         9 DOF
#define MSP_SERVO                103   //out message         8 servos
#define MSP_MOTOR                104   //out message         8 motors
#define MSP_RC                   105   //out message         8 rc chan
#define MSP_RAW_GPS              106   //out message         fix, numsat, lat, lon, alt, speed, ground course
#define MSP_COMP_GPS             107   //out message         distance home, direction home
#define MSP_ATTITUDE             108   //out message         2 angles 1 heading (pitch roll yaw headfreemodehold)
#define MSP_ALTITUDE             109   //out message         altitude, variometer
#define MSP_BAT                  110   //out message         vbat, powermetersum
#define MSP_RC_TUNING            111   //out message         rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_PID                  112   //out message         up to 16 P I D (8 are used)
#define MSP_BOX                  113   //out message         up to 16 checkbox (11 are used)
#define MSP_MISC                 114   //out message         powermeter trig + 8 free for future use
#define MSP_MOTOR_PINS           115   //out message         which pins are in use for motors & servos, for GUI
#define MSP_BOXNAMES             116   //out message         the aux switch names
#define MSP_PIDNAMES             117   //out message         the PID names
#define MSP_WP                   118   //out message         get a WP, WP# is in the payload, returns (WP#, lat, lon, alt, flags) WP#0-home, WP#16-poshold

#define MSP_SET_RAW_RC           200   //in message          8 rc chan
#define MSP_SET_RAW_GPS          201   //in message          fix, numsat, lat, lon, alt, speed
#define MSP_SET_PID              202   //in message          up to 16 P I D (8 are used)
#define MSP_SET_BOX              203   //in message          up to 16 checkbox (11 are used)
#define MSP_SET_RC_TUNING        204   //in message          rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_ACC_CALIBRATION      205   //in message          no param
#define MSP_MAG_CALIBRATION      206   //in message          no param
#define MSP_SET_MISC             207   //in message          powermeter trig + 8 free for future use
#define MSP_RESET_CONF           208   //in message          no param
#define MSP_SET_WP               209   //in message          sets a given WP (WP#,lat, lon, alt, flags)
#define MSP_SELECT_SETTING       210   //in message          Select Setting Number (0-2)
#define MSP_BEAGLEBOARD			 211	// in message		 {roll pitch yaw}

#define MSP_SPEK_BIND            240   //in message          no param

#define MSP_EEPROM_WRITE         250   //in message          no param

#define MSP_DEBUGMSG             253   //out message         debug string buffer
#define MSP_DEBUG                254   //out message         debug1,debug2,debug3,debug4

typedef struct {
	int acc_x;
	int acc_y;
	int acc_z;
	int gyro_roll;
	int gyro_pitch;
	int gyro_yaw;
	int mag_roll;
	int mag_pitch;
	int mag_yaw;
} sensor_data;

typedef struct {
	int fl;
	int fr;
	int bl;
	int br;
} motor_data;

typedef struct
{
	int roll;
	int pitch;
	int yaw;
	int throttle;
	int aux1;
	int aux2;
	int aux3;
	int aux4;
} rc_values;

typedef struct
{
	int roll;
	int pitch;
	int yaw;
	int headfreemodehold;
} attitude_data;

typedef struct
{
	int d0;
	int d1;
	int d2;
	int d3;
} debug_data;

typedef struct
{
	int altitude;
	int vario;
} altitude_data;

int GetMotorMW(int fd, motor_data *md);
int GetAttitudeMW(int fd, attitude_data *attd);
int GetAltitudeMW(int fd, altitude_data *altd);
int GetSensorMW(int fd, sensor_data *sd);
int SetRcMW(int fd, rc_values *values);
int GetRcMW(int fd, rc_values *rc);
int GetDebug(int fd, debug_data *dd);
inline void B16ToBuf(unsigned char *buf, int in);
inline short BufToB16(unsigned char *buf);

inline short Deserialize32(unsigned char *buf);
inline short Deserialize16(unsigned char *buf);
inline void Serialize16(short data);
inline void Serialize8(unsigned char data);


#endif /* MULTIWII_DRIVER_H_ */
