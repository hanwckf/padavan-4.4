/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __HWMSENSOR_H__
#define __HWMSENSOR_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_ORIENTATION         3
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PRESSURE            6
#define SENSOR_TYPE_TEMPERATURE         7
#define SENSOR_TYPE_PROXIMITY           8
#define SENSOR_TYPE_GRAVITY             9
#define SENSOR_TYPE_LINEAR_ACCELERATION 10
#define SENSOR_TYPE_ROTATION_VECTOR     11
#define	SENSOR_TYPE_HUMIDITY		12
#define SENSOR_TYPE_AMBIENT_TEMPERATURE			13 /*Add kernel driver*/
#define SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED		14 /*Add kernel driver*/
#define SENSOR_TYPE_GAME_ROTATION_VECTOR 15
#define SENSOR_TYPE_GYROSCOPE_UNCALIBRATED		16 /*Add kernel driver*/
#define SENSOR_TYPE_SIGNIFICANT_MOTION  17
#define SENSOR_TYPE_STEP_DETECTOR       18
#define SENSOR_TYPE_STEP_COUNTER        19

#define SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR     20

#define SENSOR_TYPE_HEART_RATE          21
#define SENSOR_TYPE_TILT_DETECTOR       22
#define SENSOR_TYPE_WAKE_GESTURE        23
#define SENSOR_TYPE_GLANCE_GESTURE      24
#define SENSOR_TYPE_PICK_UP_GESTURE     25
#define SENSOR_TYPE_WRIST_TILT_GESTURE  26

#define SENSOR_TYPE_PEDOMETER           (35)
#define SENSOR_STRING_TYPE_PEDOMETER                 "android.sensor.pedometer"
#define SENSOR_TYPE_IN_POCKET           (36)
#define SENSOR_STRING_TYPE_IN_POCKET                 "android.sensor.in_pocket"
#define SENSOR_TYPE_ACTIVITY            (37)
#define SENSOR_STRING_TYPE_ACTIVITY                  "android.sensor.activity"
#define SENSOR_TYPE_PDR					38 /*Add kernel driver*/
#define SENSOR_STRING_TYPE_PDR                  "android.sensor.pdr"
#define SENSOR_TYPE_FREEFALL				39
#define SENSOR_STRING_TYPE_FREEFALL                "android.sensor.freefall"
#define SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED		40 /*Add kernel driver*/
#define SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED    "android.sensor.accelerometer_uncalibrated"

#define SENSOR_TYPE_FACE_DOWN           (41)
#define SENSOR_STRING_TYPE_FACE_DOWN                 "android.sensor.face_down"
#define SENSOR_TYPE_SHAKE               (42)
#define SENSOR_STRING_TYPE_SHAKE                     "android.sensor.shake"
#define SENSOR_TYPE_BRINGTOSEE          (43)
#define SENSOR_STRING_TYPE_BRINGTOSEE               "android.sensor.bring_to_see"
#define SENSOR_TYPE_ANSWER_CALL          (44)
#define SENSOR_STRING_TYPE_ANSWERCALL               "android.sensor.answer_call"
#define SENSOR_TYPE_STATIONARY          (45)
#define SENSOR_STRING_TYPE_STATIONARY               "android.sensor.stationary"
#define SENSOR_TYPE_RGBW								(46)
#define SENSOR_STRING_TYPE_RGBW                     "android.sensor.rgbw"

/*---------------------------------------------------------------------------*/
#define ID_BASE							0
#define ID_ACCELEROMETER				(ID_BASE+SENSOR_TYPE_ACCELEROMETER-1)
#define ID_MAGNETIC						(ID_BASE+SENSOR_TYPE_MAGNETIC_FIELD-1)
#define ID_ORIENTATION					(ID_BASE+SENSOR_TYPE_ORIENTATION-1)
#define ID_GYROSCOPE					(ID_BASE+SENSOR_TYPE_GYROSCOPE-1)
#define ID_LIGHT						(ID_BASE+SENSOR_TYPE_LIGHT-1)
#define ID_PRESSURE						(ID_BASE+SENSOR_TYPE_PRESSURE-1)
#define ID_TEMPRERATURE					(ID_BASE+SENSOR_TYPE_TEMPERATURE-1)
#define ID_HUMIDITY						(ID_BASE+SENSOR_TYPE_HUMIDITY-1)
#define ID_PROXIMITY					(ID_BASE+SENSOR_TYPE_PROXIMITY-1)
#define ID_GRAVITY						(ID_BASE+SENSOR_TYPE_GRAVITY-1)
#define ID_LINEAR_ACCELERATION			(ID_BASE+SENSOR_TYPE_LINEAR_ACCELERATION-1)
#define ID_ROTATION_VECTOR				(ID_BASE+SENSOR_TYPE_ROTATION_VECTOR-1)
#define ID_RELATIVE_HUMIDITY			(ID_BASE+SENSOR_TYPE_HUMIDITY-1)
#define ID_AMBIENT_TEMPERATURE			(ID_BASE+SENSOR_TYPE_AMBIENT_TEMPERATURE-1)
#define ID_MAGNETIC_UNCALIBRATED		(ID_BASE+SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED-1)
#define ID_GAME_ROTATION_VECTOR			(ID_BASE+SENSOR_TYPE_GAME_ROTATION_VECTOR-1)
#define ID_GYROSCOPE_UNCALIBRATED		(ID_BASE+SENSOR_TYPE_GYROSCOPE_UNCALIBRATED-1)
#define ID_SIGNIFICANT_MOTION			(ID_BASE+SENSOR_TYPE_SIGNIFICANT_MOTION-1)
#define ID_STEP_DETECTOR				(ID_BASE+SENSOR_TYPE_STEP_DETECTOR-1)
#define ID_STEP_COUNTER					(ID_BASE+SENSOR_TYPE_STEP_COUNTER-1)
#define ID_GEOMAGNETIC_ROTATION_VECTOR	(ID_BASE+SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR-1)
#define ID_HEART_RATE					(ID_BASE+SENSOR_TYPE_HEART_RATE-1)
#define ID_TILT_DETECTOR				(ID_BASE+SENSOR_TYPE_TILT_DETECTOR-1)
#define ID_WAKE_GESTURE					(ID_BASE+SENSOR_TYPE_WAKE_GESTURE-1)
#define ID_GLANCE_GESTURE				(ID_BASE+SENSOR_TYPE_GLANCE_GESTURE-1)
#define ID_PICK_UP_GESTURE				(ID_BASE+SENSOR_TYPE_PICK_UP_GESTURE-1)
#define ID_WRIST_TITL_GESTURE      (ID_BASE+SENSOR_TYPE_WRIST_TILT_GESTURE-1)

#define ID_PEDOMETER                                    (ID_BASE+SENSOR_TYPE_PEDOMETER-1)
#define ID_IN_POCKET                    (ID_BASE+SENSOR_TYPE_IN_POCKET-1)
#define ID_ACTIVITY                     (ID_BASE+SENSOR_TYPE_ACTIVITY-1)
#define ID_PDR							(ID_BASE+SENSOR_TYPE_PDR-1)
#define ID_FREEFALL						(ID_BASE+SENSOR_TYPE_FREEFALL-1)
#define ID_ACCELEROMETER_UNCALIBRATED	(ID_BASE+SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED-1)

#define ID_FACE_DOWN                                    (ID_BASE+SENSOR_TYPE_FACE_DOWN-1)
#define ID_SHAKE                                        (ID_BASE+SENSOR_TYPE_SHAKE-1)
#define ID_BRINGTOSEE                                   (ID_BASE+SENSOR_TYPE_BRINGTOSEE-1)
#define ID_ANSWER_CALL                                   (ID_BASE+SENSOR_TYPE_ANSWER_CALL-1)
#define ID_STATIONARY                                   (ID_BASE+SENSOR_TYPE_STATIONARY-1)
#define ID_RGBW						(ID_BASE+SENSOR_TYPE_RGBW-1)
#define ID_SENSOR_MAX_HANDLE	  (ID_BASE+SENSOR_TYPE_RGBW-1)
#define ID_NONE							    (ID_SENSOR_MAX_HANDLE+1)

#define ID_OFFSET                           (1)
#define ID_SCP_MAX_SENSOR_TYPE				(57)

#define MAX_ANDROID_SENSOR_NUM	(ID_SENSOR_MAX_HANDLE+1)
#define MAX_SENSOR_DATA_UPDATE_ONCE         (20)


/*---------------------------------------------------------------------------*/
#define SENSOR_ACCELEROMETER			(1 << ID_ACCELEROMETER)
#define SENSOR_MAGNETIC					(1 << ID_MAGNETIC)
#define SENSOR_ORIENTATION				(1 << ID_ORIENTATION)
#define SENSOR_GYROSCOPE				(1 << ID_GYROSCOPE)
#define SENSOR_LIGHT					(1 << ID_LIGHT)
#define SENSOR_HUMIDITY					(1 << ID_HUMIDITY)
#define SENSOR_PRESSURE					(1 << ID_PRESSURE)
#define SENSOR_TEMPRERATURE				(1 << ID_TEMPRERATURE)
#define SENSOR_PROXIMITY				(1 << ID_PROXIMITY)
#define SENSOR_GRAVITY					(1 << ID_GRAVITY)
#define SENSOR_LINEAR_ACCELERATION		(1 << ID_LINEAR_ACCELERATION)
#define SENSOR_ROTATION_VECTOR			(1 << ID_ROTATION_VECTOR)
#define SENSOR_RELATIVE_HUMIDITY		(1 << ID_RELATIVE_HUMIDITY)
#define SENSOR_AMBIENT_TEMPERATURE		(1 << ID_AMBIENT_TEMPERATURE)
#define SENSOR_MAGNETIC_UNCALIBRATED	(1 << ID_MAGNETIC_UNCALIBRATED)
#define SENSOR_GAME_ROTATION_VECTOR		(1 << ID_GAME_ROTATION_VECTOR)
#define SENSOR_GYROSCOPE_UNCALIBRATED	(1 << ID_GYROSCOPE_UNCALIBRATED)

#define SENSOR_SIGNIFICANT_MOTION           (1 << ID_SIGNIFICANT_MOTION)
#define SENSOR_STEP_DETECTOR                (1 << ID_STEP_DETECTOR)
#define SENSOR_STEP_COUNTER                 (1 << ID_STEP_COUNTER)
#define SENSOR_GEOMAGNETIC_ROTATION_VECTOR  (1 << ID_GEOMAGNETIC_ROTATION_VECTOR)

#define SENSOR_HEART_RATE           (1 << ID_HEART_RATE)
#define SENSOR_TILT_DETECTOR        (1 << ID_TILT_DETECTOR)
#define SENSOR_WAKE_GESTURE         (1 << ID_WAKE_GESTURE)
#define SENSOR_GLANCE_GESTURE       (1 << ID_GLANCE_GESTURE)
#define SENSOR_PICK_UP_GESTURE      (1 << ID_PICK_UP_GESTURE)
#define SENSOR_WRIST_TITL_GESTURE   (1 << ID_WRIST_TITL_GESTURE)

#define SENSOR_PEDOMETER                    (1 << ID_PEDOMETER)
#define SENSOR_IN_POCKET                    (1 << ID_IN_POCKET)
#define SENSOR_ACTIVITY                     (1 << ID_ACTIVITY)
#define SENSOR_PDR							(1 << ID_PDR)
#define SENSOR_FREEFALL                     (1 << ID_FREEFALL)
#define SENSOR_ACCELEROMETER_UNCALIBRATED   (1 << ID_ACCELEROMETER_UNCALIBRATED)

#define SENSOR_FACE_DOWN                    (1 << ID_FACE_DOWN)
#define SENSOR_SHAKE                        (1 << ID_SHAKE)
#define SENSOR_BRINGTOSEE                   (1 << ID_BRINGTOSEE)
#define SENSOR_ANSWER_CALL                   (1 << ID_ANSWER_CALL)

/*----------------------------------------------------------------------------*/
#define HWM_INPUTDEV_NAME               "hwmdata"
#define HWM_SENSOR_DEV_NAME             "hwmsensor"
#define HWM_SENSOR_DEV                  "/dev/hwmsensor"
#define C_MAX_HWMSEN_EVENT_NUM          4
/*----------------------------------------------------------------------------*/
#define ACC_PL_DEV_NAME                 "m_acc_pl"
#define ACC_INPUTDEV_NAME               "m_acc_input"
#define ACC_MISC_DEV_NAME               "m_acc_misc"
#define MAG_PL_DEV_NAME                 "m_mag_pl"
#define MAG_INPUTDEV_NAME               "m_mag_input"
#define MAG_MISC_DEV_NAME               "m_mag_misc"
#define UNCALI_MAG_PL_DEV_NAME                 "m_uncali_mag_pl"
#define UNCALI_MAG_INPUTDEV_NAME               "m_uncali_mag_input"
#define UNCALI_MAG_MISC_DEV_NAME               "m_uncali_mag_misc"
#define UNCALI_GYRO_PL_DEV_NAME         "m_uncali_gyro_pl"
#define UNCALI_GYRO_INPUTDEV_NAME       "m_uncali_gyro_input"
#define UNCALI_GYRO_MISC_DEV_NAME       "m_uncali_gyro_misc"
#define GYRO_PL_DEV_NAME                "m_gyro_pl"
#define GYRO_INPUTDEV_NAME              "m_gyro_input"
#define GYRO_MISC_DEV_NAME              "m_gyro_misc"
#define ALSPS_PL_DEV_NAME               "m_alsps_pl"
#define ALSPS_INPUTDEV_NAME             "m_alsps_input"
#define ALSPS_MISC_DEV_NAME             "m_alsps_misc"
#define RGBW_PL_DEV_NAME                "m_rgbw_pl"
#define RGBW_INPUTDEV_NAME              "m_rgbw_input"
#define RGBW_MISC_DEV_NAME              "m_rgbw_misc"
#define BARO_PL_DEV_NAME                "m_baro_pl"
#define BARO_INPUTDEV_NAME              "m_baro_input"
#define BARO_MISC_DEV_NAME              "m_baro_misc"
#define HMDY_PL_DEV_NAME		"m_hmdy_pl"
#define HMDY_INPUTDEV_NAME		"m_hmdy_input"
#define HMDY_MISC_DEV_NAME		"m_hmdy_misc"

#define STEP_C_PL_DEV_NAME                "m_step_c_pl"
#define STEP_C_INPUTDEV_NAME              "m_step_c_input"
#define STEP_C_MISC_DEV_NAME              "m_step_c_misc"

#define INPK_PL_DEV_NAME                "m_inpk_pl"
#define INPK_INPUTDEV_NAME              "m_inpk_input"
#define INPK_MISC_DEV_NAME              "m_inpk_misc"

#define SHK_PL_DEV_NAME                "m_shk_pl"
#define SHK_INPUTDEV_NAME              "m_shk_input"
#define SHK_MISC_DEV_NAME              "m_shk_misc"

#define FDN_PL_DEV_NAME                "m_fdn_pl"
#define FDN_INPUTDEV_NAME              "m_fdn_input"
#define FDN_MISC_DEV_NAME              "m_fdn_misc"

#define PKUP_PL_DEV_NAME                "m_pkup_pl"
#define PKUP_INPUTDEV_NAME              "m_pkup_input"
#define PKUP_MISC_DEV_NAME              "m_pkup_misc"

#define ACT_PL_DEV_NAME                "m_act_pl"
#define ACT_INPUTDEV_NAME              "m_act_input"
#define ACT_MISC_DEV_NAME              "m_act_misc"

#define PDR_PL_DEV_NAME                "m_pdr_pl"
#define PDR_INPUTDEV_NAME              "m_pdr_input"
#define PDR_MISC_DEV_NAME              "m_pdr_misc"

#define HRM_PL_DEV_NAME                "m_hrm_pl"
#define HRM_INPUTDEV_NAME              "m_hrm_input"
#define HRM_MISC_DEV_NAME              "m_hrm_misc"

#define TILT_PL_DEV_NAME               "m_tilt_pl"
#define TILT_INPUTDEV_NAME             "m_tilt_input"
#define TILT_MISC_DEV_NAME             "m_tilt_misc"

#define WAG_PL_DEV_NAME                "m_wag_pl"
#define WAG_INPUTDEV_NAME              "m_wag_input"
#define WAG_MISC_DEV_NAME              "m_wag_misc"

#define GLG_PL_DEV_NAME                "m_glg_pl"
#define GLG_INPUTDEV_NAME              "m_glg_input"
#define GLG_MISC_DEV_NAME              "m_glg_misc"

#define ANSWERCALL_PL_DEV_NAME          "m_ancall_pl"
#define ANSWERCALL_INPUTDEV_NAME        "m_ancall_input"
#define ANSWERCALL_MISC_DEV_NAME        "m_ancall_misc"

#define TEMP_PL_DEV_NAME               "m_temp_pl"
#define TEMP_INPUTDEV_NAME             "m_temp_input"
#define TEMP_MISC_DEV_NAME             "m_temp_misc"

#define BATCH_PL_DEV_NAME              "m_batch_pl"
#define BATCH_INPUTDEV_NAME            "m_batch_input"
#define BATCH_MISC_DEV_NAME            "m_batch_misc"

#define BTS_PL_DEV_NAME                "m_bts_pl"
#define BTS_INPUTDEV_NAME              "m_bts_input"
#define BTS_MISC_DEV_NAME              "m_bts_misc"

#define GRV_PL_DEV_NAME                "m_grv_pl"
#define GRV_INPUTDEV_NAME              "m_grv_input"
#define GRV_MISC_DEV_NAME              "m_grv_misc"

#define GMRV_PL_DEV_NAME               "m_gmrv_pl"
#define GMRV_INPUTDEV_NAME             "m_gmrv_input"
#define GMRV_MISC_DEV_NAME             "m_gmrv_misc"

#define GRAV_PL_DEV_NAME               "m_grav_pl"
#define GRAV_INPUTDEV_NAME             "m_grav_input"
#define GRAV_MISC_DEV_NAME             "m_grav_misc"

#define LA_PL_DEV_NAME                 "m_la_pl"
#define LA_INPUTDEV_NAME               "m_la_input"
#define LA_MISC_DEV_NAME               "m_la_misc"

#define RV_PL_DEV_NAME                 "m_rv_pl"
#define RV_INPUTDEV_NAME               "m_rv_input"
#define RV_MISC_DEV_NAME               "m_rv_misc"

#define FREEFALL_PL_DEV_NAME                 "m_frfl_pl"
#define FREEFALL_INPUTDEV_NAME               "m_frfl_input"
#define FREEFALL_MISC_DEV_NAME               "m_frfl_misc"

#define PEDO_PL_DEV_NAME                 "m_pedo_pl"
#define PEDO_INPUTDEV_NAME               "m_pedo_input"
#define PEDO_MISC_DEV_NAME               "m_pedo_misc"

#define GES_PL_DEV_NAME                 "m_ges_pl"
#define GES_INPUTDEV_NAME               "m_ges_input"
#define GES_MISC_DEV_NAME               "m_ges_misc"

#define EVENT_TYPE_SENSOR				0x01
#define EVENT_TYPE_SENSOR_EXT				0x02
#define EVENT_SENSOR_ACCELERATION		SENSOR_ACCELEROMETER
#define EVENT_SENSOR_MAGNETIC			SENSOR_MAGNETIC
#define EVENT_SENSOR_ORIENTATION		SENSOR_ORIENTATION
#define EVENT_SENSOR_GYROSCOPE			SENSOR_GYROSCOPE
#define EVENT_SENSOR_LIGHT				SENSOR_LIGHT
#define EVENT_SENSOR_PRESSURE			SENSOR_PRESSURE
#define EVENT_SENSOR_TEMPERATURE		SENSOR_TEMPRERATURE
#define EVENT_SENSOR_PROXIMITY			SENSOR_PROXIMITY
#define EVENT_SENSOR_GRAVITY			SENSOR_PRESSURE
#define EVENT_SENSOR_LINEAR_ACCELERATION		SENSOR_TEMPRERATURE
#define EVENT_SENSOR_ROTATION_VECTOR	SENSOR_PROXIMITY
#define EVENT_TYPE_INPK_VALUE            0x1
#define EVENT_TYPE_STATIONARY_VALUE      0x2

/*-----------------------------------------------------------------------------*/

enum {
	HWM_MODE_DISABLE = 0,
	HWM_MODE_ENABLE = 1,
};

/*------------sensors data----------------------------------------------------*/
struct hwm_sensor_data {
	/* sensor identifier */
	int sensor;
	/* sensor values */
	union {
		int values[6];
		uint8_t probability[12];
	};
	/* sensor values divide */
	uint32_t value_divide;
	/* sensor accuracy */
	int8_t status;
	/* whether updata? */
	int update;
	/* time is in nanosecond */
	int64_t time;

	uint32_t reserved;
};

#ifdef CONFIG_COMPAT
struct compat_hwm_sensor_data {
	/* sensor identifier */
	compat_int_t sensor;
	/* sensor values */
	union {
		compat_int_t	values[6];
		uint8_t probability[12];
	};
	/* sensor values divide */
	compat_uint_t value_divide;
	/* sensor accuracy */
	char status;
	/* whether updata? */
	compat_int_t update;
	/* time is in nanosecond */
	compat_s64 time;

	compat_uint_t reserved;
};
#endif

struct hwm_trans_data {
	struct hwm_sensor_data data[MAX_SENSOR_DATA_UPDATE_ONCE];
	uint64_t data_type;
};

#ifdef CONFIG_COMPAT
struct compat_hwm_trans_data {
	struct compat_hwm_sensor_data data[MAX_SENSOR_DATA_UPDATE_ONCE];
	compat_u64 data_type;
};
#endif

#define MAX_BATCH_DATA_PER_QUREY    18
struct batch_trans_data {
	int numOfDataReturn;
	int numOfDataLeft;
	struct hwm_sensor_data data[MAX_BATCH_DATA_PER_QUREY];
};

#ifdef CONFIG_COMPAT
struct compat_batch_trans_data {
	compat_int_t numOfDataReturn;
	compat_int_t numOfDataLeft;
	struct compat_hwm_sensor_data data[MAX_BATCH_DATA_PER_QUREY];
};
#endif

/*----------------------------------------------------------------------------*/
#define HWM_IOC_MAGIC           0x91

/* set delay */
#define HWM_IO_SET_DELAY		_IOW(HWM_IOC_MAGIC, 0x01, uint32_t)

/* wake up */
#define HWM_IO_SET_WAKE			_IO(HWM_IOC_MAGIC, 0x02)

/* Enable/Disable  sensor */
#define HWM_IO_ENABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x03, uint32_t)
#define HWM_IO_DISABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x04, uint32_t)

/* Enable/Disable sensor */
#define HWM_IO_ENABLE_SENSOR_NODATA		_IOW(HWM_IOC_MAGIC, 0x05, uint32_t)
#define HWM_IO_DISABLE_SENSOR_NODATA	_IOW(HWM_IOC_MAGIC, 0x06, uint32_t)
/* Get sensors data */
#define HWM_IO_GET_SENSORS_DATA			_IOWR(HWM_IOC_MAGIC, 0x07, struct hwm_trans_data)
#ifdef CONFIG_COMPAT
/* set delay */
#define COMPAT_HWM_IO_SET_DELAY		_IOW(HWM_IOC_MAGIC, 0x01, compat_uint_t)

/* wake up */
#define COMPAT_HWM_IO_SET_WAKE			_IO(HWM_IOC_MAGIC, 0x02)

/* Enable/Disable  sensor */
#define COMPAT_HWM_IO_ENABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x03, compat_uint_t)
#define COMPAT_HWM_IO_DISABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x04, compat_uint_t)

/* Enable/Disable sensor */
#define COMPAT_HWM_IO_ENABLE_SENSOR_NODATA		_IOW(HWM_IOC_MAGIC, 0x05, compat_uint_t)
#define COMPAT_HWM_IO_DISABLE_SENSOR_NODATA	_IOW(HWM_IOC_MAGIC, 0x06, compat_uint_t)
/* Get sensors data */
#define COMPAT_HWM_IO_GET_SENSORS_DATA			_IOWR(HWM_IOC_MAGIC, 0x07, struct compat_hwm_trans_data)
#endif
/*----------------------------------------------------------------------------*/
#define BATCH_IOC_MAGIC           0x92

/* Get sensor data */
#define BATCH_IO_GET_SENSORS_DATA			_IOWR(BATCH_IOC_MAGIC, 0x01, struct batch_trans_data)
#ifdef CONFIG_COMPAT
#define COMPAT_BATCH_IO_GET_SENSORS_DATA	_IOWR(BATCH_IOC_MAGIC, 0x01, struct compat_batch_trans_data)
#endif

#endif				/* __HWMSENSOR_H__ */
