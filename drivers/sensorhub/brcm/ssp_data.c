/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"
#include <linux/math64.h>
#include <linux/sched.h>

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x37
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_DEBUG_DATA			0x03
#define MSG2AP_INST_BIG_DATA			0x04
#define MSG2AP_INST_META_DATA			0x05
#define MSG2AP_INST_TIME_SYNC			0x06
#define MSG2AP_INST_RESET			0x07
#define MSG2AP_INST_GYRO_CAL			0x08

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
#define U64_MS2NS 1000000ULL
#define U64_US2NS 1000ULL
#define U64_MS2US 1000ULL
#define MS_IDENTIFIER 1000000000U
#endif

/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
static u64 get_ts_average(struct ssp_data *data, int sensor_type, u64 ts_delta)
{
	u8 cnt = data->ts_avg_buffer_cnt[sensor_type];
	u8 idx = data->ts_avg_buffer_idx[sensor_type];
	u64 avg = 0ULL;
	
	// Get idx for insert data.
	idx = (idx + 1) % SIZE_MOVING_AVG_BUFFER;

	// Remove data from idx and insert new one.
	if (cnt == SIZE_MOVING_AVG_BUFFER) {
		data->ts_avg_buffer_sum[sensor_type] -= data->ts_avg_buffer[sensor_type][idx];
	} else {
		cnt++;
	}
	//ssp_dbg("[SSP_AVG] [%3d] cnt %d idx %d,sum %lld, delta %lld\n", 
		//sensor_type, cnt, idx, data->ts_avg_buffer_sum[sensor_type], ts_delta);

	if (ts_delta > MS_IDENTIFIER) //delta lager then 1sec, change to 1sec
		ts_delta = MS_IDENTIFIER;
		
	// Insert Data to idx.
	data->ts_avg_buffer[sensor_type][idx] = ts_delta;
	data->ts_avg_buffer_sum[sensor_type] += data->ts_avg_buffer[sensor_type][idx];

	//ssp_dbg("[SSP_AVG] [%3d] cnt %d idx %d,sum %lld\n", 
		//sensor_type, cnt, idx, data->ts_avg_buffer_sum[sensor_type]);

	avg = data->ts_avg_buffer_sum[sensor_type] / cnt;

	data->ts_avg_buffer_cnt[sensor_type] = cnt;
	data->ts_avg_buffer_idx[sensor_type] = idx;
	
	//ssp_dbg("[SSP_AVG] [%3d] %lld   IN  %lld   [IDX %5u  CNT %5u]\n",  sensor_type, avg, ts_delta, idx, cnt);

	return avg;
}

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata,
		u16 batch_mode, int sensor_type)
{
	unsigned int idxTimestamp = 0;
	unsigned int time_delta_us = 0;
	u64 time_delta_ns = 0;
	u64 cur_timestamp = get_current_timestamp();

	memset(&time_delta_us, 0, 4);
	memcpy(&time_delta_us, pchRcvDataFrame + *iDataIdx, 4);
	memset(&idxTimestamp, 0, 4);
	memcpy(&idxTimestamp, pchRcvDataFrame + *iDataIdx + 4, 4);
	//ssp_dbg("[SSP_IDX] [%3d] TS %d idx %d]\n", sensor_type, time_delta_us, idxTimestamp);

	if (time_delta_us > MS_IDENTIFIER) {
		//We condsider, unit is ms (MS->NS)
		time_delta_ns = ((u64) (time_delta_us % MS_IDENTIFIER)) * U64_MS2NS;
	} else {
		time_delta_ns = (((u64) time_delta_us) * U64_US2NS);//US->NS
	}

	// TODO: Reverse calculation of timestamp when non wake up batching.
	if (batch_mode == BATCH_MODE_RUN) {
		// BATCHING MODE 
		//ssp_dbg("[SSP_TIM] BATCH [%3d] TS %lld %lld\n", sensor_type, data->lastTimestamp[sensor_type],time_delta_ns);
		data->lastTimestamp[sensor_type] += time_delta_ns;

	} else {
		// NORMAL MODE
		unsigned int tmpIndex = 0;
		unsigned int tmpPrevIndex = 0;

		u64 tmp_ts_delta = 0ULL;
		u64 tmp_ts_avg = 0ULL;

		switch (sensor_type) {
		case ACCELEROMETER_SENSOR:
		case GYROSCOPE_SENSOR:
		case GYRO_UNCALIB_SENSOR:
		case GEOMAGNETIC_SENSOR:
		case GEOMAGNETIC_UNCALIB_SENSOR:
		case GEOMAGNETIC_RAW:
		case ROTATION_VECTOR:
		case GAME_ROTATION_VECTOR:
		case PRESSURE_SENSOR:
		case LIGHT_SENSOR:
		case PROXIMITY_RAW:
			if (data->ts_stacked_cnt < idxTimestamp) {
				int i = 0;
				unsigned int offset = 0;
				offset = idxTimestamp - data->ts_stacked_cnt;

				//ssp_dbg("[SSP_TIM]		 %20s	 ERROR : TS_STACK_OVERFLOW [MCU %5u  AP  %5u   OFFSET  %5u]\n",
					//__func__, idxTimestamp, data->ts_stacked_cnt, offset);


				// INSERT DUMMY INDEXING & SKIP 1 EVENT.
				for (i = 1; i <= offset; i++) {
					// TODO: Test if this can causing duplicated timestamp.
					data->ts_index_buffer[(data->ts_stacked_cnt + i) % SIZE_TIMESTAMP_BUFFER] 
						= data->ts_index_buffer[data->ts_stacked_cnt % SIZE_TIMESTAMP_BUFFER];
				}
				data->ts_stacked_cnt += offset;
				data->skipEventReport = true;// SKIP OVERFLOWED INDEX

				tmpIndex = data->ts_stacked_cnt % SIZE_TIMESTAMP_BUFFER;
			} else {
				tmpIndex = idxTimestamp % SIZE_TIMESTAMP_BUFFER;
				tmpPrevIndex = data->ts_prev_index[sensor_type] % SIZE_TIMESTAMP_BUFFER;
				
				if (data->ts_avg_skip_cnt[sensor_type] > 0) { // Skip 2 events for avg.
					//pr_err("[SSP_TIM] skip cnt %d-%d\n", sensor_type, data->ts_avg_skip_cnt[sensor_type]);
					data->ts_avg_skip_cnt[sensor_type]--;
					if(data->lastTimestamp[sensor_type] > data->ts_index_buffer[tmpIndex])
					{
						data->lastTimestamp[sensor_type] = data->lastTimestamp[sensor_type] + time_delta_ns;
						//pr_err("[SSP_TIM] %d +DT %lld\n", sensor_type, time_delta_ns);
					}
					else
					{
						data->lastTimestamp[sensor_type] = data->ts_index_buffer[tmpIndex];
						//pr_err("[SSP_TIM] %d lst=buffts %lld\n",sensor_type,data->ts_index_buffer[tmpIndex]);
					}
					//pr_err("[SSP_TIM] %d lts %lld", sensor_type, data->lastTimestamp[sensor_type]);
					data->ts_prev_index[sensor_type] = idxTimestamp;// KEEP LAST INDEX;
				} else {
					tmp_ts_delta = data->ts_index_buffer[tmpIndex] - data->ts_index_buffer[tmpPrevIndex];
					/*
					pr_err("[SSP_TIM] ts_delta %lld c %lld Pr %lld cIdx %d Pre %d\n",tmp_ts_delta,
						data->ts_index_buffer[tmpIndex], data->ts_index_buffer[tmpPrevIndex],
						tmpIndex, tmpPrevIndex);
					*/
					if((sensor_type == GYROSCOPE_SENSOR) && (data->cameraGyroSyncMode == true))
						tmp_ts_avg = 0;
					else
						tmp_ts_avg = get_ts_average(data, sensor_type, tmp_ts_delta);
					
					if (tmp_ts_avg == 0) {
						if(data->lastTimestamp[sensor_type] > data->ts_index_buffer[tmpIndex])
						{
							//pr_err("[SSP_TIM] %d after avg + DT %lld", sensor_type, time_delta_ns);
							pr_err("[SSP] %s - %d using delta last=%llu buf=%llu delta=%llu\n",
								__func__,sensor_type,data->lastTimestamp[sensor_type],data->ts_index_buffer[tmpIndex],tmp_ts_delta);
							data->lastTimestamp[sensor_type] = data->lastTimestamp[sensor_type] + tmp_ts_delta;
						}
						else
						{
							//pr_err("[SSP_TIM] %d lst=buffts %lld\n",sensor_type,data->ts_index_buffer[tmpIndex]);
							data->lastTimestamp[sensor_type] = data->ts_index_buffer[tmpIndex];
						}
					} else {
						//pr_err("[SSP_TIM] %d +avg %lld\n",sensor_type, tmp_ts_avg);
						if(data->lastTimestamp[sensor_type] + tmp_ts_avg + 5000000000ULL < cur_timestamp)
						{
							pr_info("[SSP] %s - %d 5s buf=%llu cur=%llu\n",__func__,sensor_type,data->ts_index_buffer[tmpIndex],cur_timestamp);
							data->lastTimestamp[sensor_type] = data->ts_index_buffer[tmpIndex];
						}
						else if(data->lastTimestamp[sensor_type] + tmp_ts_avg > cur_timestamp)
						{						
							pr_info("[SSP] %s - %d curtime cur=%llu last=%llu buf=%llu avg=%llu\n",
								__func__,sensor_type,cur_timestamp,data->lastTimestamp[sensor_type],data->ts_index_buffer[tmpIndex],tmp_ts_avg);
							data->lastTimestamp[sensor_type] = cur_timestamp;
						}						
						else
							data->lastTimestamp[sensor_type] = data->lastTimestamp[sensor_type] + tmp_ts_avg;
					}
					data->ts_prev_index[sensor_type] = idxTimestamp;// KEEP LAST INDEX;
				}
			}
			break;

		// None Indexing
		default:
			data->lastTimestamp[sensor_type] = data->timestamp;
			break;
		}
		//ssp_dbg("[SSP_TIM] NORMA [%3d] TS %lld\n", 
			//sensor_type, data->lastTimestamp[sensor_type]);
	}

	sensorsdata->timestamp = data->lastTimestamp[sensor_type];
	//sensorsdata->timestamp = data->lastTimestamp[sensor_type] - (6ULL * U64_MS2NS);

#if 0// TEST To correcting timestamp sync... (why 6ms? we needs PROOF)
	if(sensor_type == GYROSCOPE_SENSOR){
		u64 cTS = get_current_timestamp();
		pr_err("[SSP_TIM]gyroTS %lld DT %lld[MCU %5u AP %5u]curTS %lld\n", 
		sensorsdata->timestamp, time_delta_ns,idxTimestamp, data->ts_stacked_cnt, cTS);
	}
	ssp_dbg("[SSP_TIM] ----- [%3d] TS %lld DT %lld[MCU %5u	AP	%5u]\n", 
		sensor_type,
		sensorsdata->timestamp, time_delta_ns,idxTimestamp, data->ts_stacked_cnt);
#endif// TEST

	*iDataIdx += 8;
}
#else
static void generate_data(struct ssp_data *data,
	struct sensor_value *sensorsdata, int iSensorData, u64 timestamp)
{
	u64 move_timestamp = data->lastTimestamp[iSensorData];
	if ((iSensorData != PROXIMITY_SENSOR) && (iSensorData != GESTURE_SENSOR)
		&& (iSensorData != STEP_DETECTOR) && (iSensorData != SIG_MOTION_SENSOR)
		&& (iSensorData != STEP_COUNTER)
#ifdef CONFIG_SENSORS_SSP_SX9306
		&& (iSensorData != GRIP_SENSOR)
#endif
		) {
		while ((move_timestamp * 10 + data->adDelayBuf[iSensorData] * 13) < (timestamp * 10)) {
			move_timestamp += data->adDelayBuf[iSensorData];
			sensorsdata->timestamp = move_timestamp;
			data->report_sensor_data[iSensorData](data, sensorsdata);
		}
	}
}

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata,
		struct ssp_time_diff *sensortime, int iSensorData)
{
	if (sensortime->batch_mode == BATCH_MODE_RUN) {
		if (sensortime->batch_count == sensortime->batch_count_fixed) {
			if (sensortime->time_diff == data->adDelayBuf[iSensorData]) {
				generate_data(data, sensorsdata, iSensorData,
						(data->timestamp - data->adDelayBuf[iSensorData] * (sensortime->batch_count_fixed - 1)));
			}
			sensorsdata->timestamp = data->timestamp - ((sensortime->batch_count - 1) * sensortime->time_diff);
		} else {
			if (sensortime->batch_count > 1)
				sensorsdata->timestamp = data->timestamp - ((sensortime->batch_count - 1) * sensortime->time_diff);
			else
				sensorsdata->timestamp = data->timestamp;
		}
	} else {
		if (((sensortime->irq_diff * 10) > (data->adDelayBuf[iSensorData] * 13))
			&& ((sensortime->irq_diff * 10) < (data->adDelayBuf[iSensorData] * 100))) {
			generate_data(data, sensorsdata, iSensorData, data->timestamp);
		}
		sensorsdata->timestamp = data->timestamp;
	}
	*iDataIdx += 4;
}

#endif
static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_uncalib_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}

static void get_geomagnetic_uncaldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}

static void get_geomagnetic_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_geomagnetic_caldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 7);
	*iDataIdx += 7;
}

static void get_rot_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 17);
	*iDataIdx += 17;
}

static void get_step_det_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 10);
	*iDataIdx += 10;
}

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
static void get_light_ir_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}
#endif

static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	s16 temperature = 0;
	memcpy(&sensorsdata->pressure[0], pchRcvDataFrame + *iDataIdx, 4);
	memcpy(&temperature, pchRcvDataFrame + *iDataIdx + 4, 2);
	sensorsdata->pressure[1] = temperature;
	*iDataIdx += 6;
}

static void get_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 20);
	*iDataIdx += 20;
}

static void get_proximity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined(CONFIG_SENSORS_SSP_TMG399x)
	memset(&sensorsdata->prox[0], 0, 1);
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
#else	//CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782
	memset(&sensorsdata->prox[0], 0, 2);
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 1);
	memcpy(&sensorsdata->prox[1], pchRcvDataFrame + *iDataIdx + 1, 2);
	*iDataIdx += 3;
#endif
}

static void get_proximity_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined(CONFIG_SENSORS_SSP_TMG399x)
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
#else	//CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
#endif
}

#ifdef CONFIG_SENSORS_SSP_SX9306
static void get_grip_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 9);
		*iDataIdx += 9;
}
#endif

static void get_temp_humidity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memset(&sensorsdata->data[2], 0, 2);
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 5);
	*iDataIdx += 5;
}

static void get_sig_motion_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_step_cnt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->step_diff, pchRcvDataFrame + *iDataIdx, 4);
	*iDataIdx += 4;
}

static void get_shake_cam_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_tilt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_pickup_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
/*
static void get_sensor_data(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata, int data_size)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, data_size);
	*iDataIdx += data_size;
}
*/

bool ssp_check_buffer(struct ssp_data *data)
{
	int idx_data = 0;
	u8 sensor_type = 0;
	bool res = true;
	u64 ts = get_current_timestamp();
	pr_err("[SSP_BUF] start check %lld\n", ts);
	do{
		sensor_type = data->batch_event.batch_data[idx_data++];

		if ((sensor_type != ACCELEROMETER_SENSOR) &&
			(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
			(sensor_type != PRESSURE_SENSOR) &&
			(sensor_type != GAME_ROTATION_VECTOR) &&
			(sensor_type != PROXIMITY_SENSOR) &&
			(sensor_type != META_SENSOR)) {
			pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
					sensor_type, idx_data - 1);
			res = false;
			break;
		}

		switch(sensor_type)
		{
		case ACCELEROMETER_SENSOR:
			idx_data += 14;
			break;
		case GEOMAGNETIC_UNCALIB_SENSOR:
			idx_data += 20;
			break;
		case PRESSURE_SENSOR:
			idx_data += 14;
			break;
		case GAME_ROTATION_VECTOR:
			idx_data += 25;
			break;
		case PROXIMITY_SENSOR:
			idx_data += 11;
			break;
		case META_SENSOR:
			idx_data += 1;
			break;
		}

		if(idx_data > data->batch_event.batch_length){
			//stop index over max length
			pr_info("[SSP_CHK] invalid data1\n");
			res = false;
			break;
		}

		// run until max length
		if(idx_data == data->batch_event.batch_length){
			//pr_info("[SSP_CHK] valid data\n");
			break;
		}
		else if(idx_data + 1 == data->batch_event.batch_length){
			//stop if only sensor type exist
			pr_info("[SSP_CHK] invalid data2\n");
			res = false;
			break;
		}
	}while(true);
	ts = get_current_timestamp();
	pr_err("[SSP_BUF] finish check %lld\n", ts);

	return res;
}

void ssp_batch_resume_check(struct ssp_data *data)
{
	u64 acc_offset = 0, uncal_mag_offset = 0, press_offset = 0, grv_offset = 0, proxi_offset = 0;
	//if suspend -> wakeup case. calc. FIFO last timestamp
	if(data->bIsResumed)
	{
		u8 sensor_type = 0;
		struct sensor_value sensor_data;
		unsigned int delta_time_us = 0;
		unsigned int idx_timestamp = 0;
		int idx_data = 0;
		u64 timestamp = get_current_timestamp();
		//ssp_dbg("[SSP_BAT] LENGTH = %d, start index = %d ts %lld resume %lld\n", data->batch_event.batch_length, idx_data, timestamp, data->resumeTimestamp);

		timestamp = data->resumeTimestamp = data->timestamp;
		
		while (idx_data < data->batch_event.batch_length)
		{
			sensor_type = data->batch_event.batch_data[idx_data++];
			if(sensor_type == META_SENSOR)	{
				sensor_data.meta_data.sensor = data->batch_event.batch_data[idx_data++];
				continue;
			}
			
			if ((sensor_type != ACCELEROMETER_SENSOR) &&
				(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
				(sensor_type != PRESSURE_SENSOR) &&
				(sensor_type != GAME_ROTATION_VECTOR) &&
				(sensor_type != PROXIMITY_SENSOR)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
						sensor_type, idx_data - 1);
				data->bIsResumed = false;
				data->resumeTimestamp = 0ULL;
				return ;
			}

			data->get_sensor_data[sensor_type](data->batch_event.batch_data, &idx_data, &sensor_data);

			memset(&delta_time_us, 0, 4);
			memcpy(&delta_time_us, data->batch_event.batch_data + idx_data, 4);
			memset(&idx_timestamp, 0, 4);
			memcpy(&idx_timestamp, data->batch_event.batch_data + idx_data + 4, 4);
			if (delta_time_us > MS_IDENTIFIER) {
				//We condsider, unit is ms (MS->NS)
				delta_time_us = ((u64) (delta_time_us % MS_IDENTIFIER)) * U64_MS2NS;
			} else {
				delta_time_us = (((u64) delta_time_us) * U64_US2NS);//US->NS
			}

			switch(sensor_type)
			{
				case ACCELEROMETER_SENSOR:
					acc_offset += delta_time_us;
					break;
				case GEOMAGNETIC_UNCALIB_SENSOR:
					uncal_mag_offset += delta_time_us;
					break;
				case GAME_ROTATION_VECTOR:
					grv_offset += delta_time_us;
					break;
				case PRESSURE_SENSOR:
					press_offset += delta_time_us;
					break;
				case PROXIMITY_SENSOR:
					proxi_offset += delta_time_us;
					break;
				default:
					break;				
			}
			idx_data += 8;
		}


		if(acc_offset > 0)
			data->lastTimestamp[ACCELEROMETER_SENSOR] = timestamp - acc_offset;
		if(uncal_mag_offset > 0)
			data->lastTimestamp[GEOMAGNETIC_UNCALIB_SENSOR] = timestamp - uncal_mag_offset;
		if(press_offset > 0)
			data->lastTimestamp[PRESSURE_SENSOR] = timestamp - press_offset;
		if(grv_offset > 0)
			data->lastTimestamp[GAME_ROTATION_VECTOR] = timestamp - grv_offset;
		if(proxi_offset > 0)
			data->lastTimestamp[PROXIMITY_SENSOR] = timestamp - proxi_offset;

		ssp_dbg("[SSP_BAT] resume calc. acc %lld. uncalmag %lld. pressure %lld. GRV %lld proxi %lld\n", acc_offset, uncal_mag_offset, press_offset, grv_offset, proxi_offset);
	}
	data->bIsResumed = false;
	data->resumeTimestamp = 0ULL;
}

void ssp_batch_report(struct ssp_data *data)
{
	u8 sensor_type = 0;
	struct sensor_value sensor_data;
	int idx_data = 0;
	int count = 0;
	u64 timestamp = get_current_timestamp();

	ssp_dbg("[SSP_BAT] LENGTH = %d, start index = %d ts %lld\n", data->batch_event.batch_length, idx_data, timestamp);

	while (idx_data < data->batch_event.batch_length)
	{
		//ssp_dbg("[SSP_BAT] bcnt %d\n", count);
		sensor_type = data->batch_event.batch_data[idx_data++];

		if(sensor_type == META_SENSOR)	{
			sensor_data.meta_data.sensor = data->batch_event.batch_data[idx_data++];
			report_meta_data(data, &sensor_data);
			count++;
			continue;
		}

		if ((sensor_type != ACCELEROMETER_SENSOR) &&
			(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
			(sensor_type != PRESSURE_SENSOR) &&
			(sensor_type != GAME_ROTATION_VECTOR) &&
			(sensor_type != PROXIMITY_SENSOR)) {
			pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
					sensor_type, idx_data - 1);
			return ;
		}

		if(count%80 == 0)
			usleep_range(1000,1000);
		//ssp_dbg("[SSP_BAT] cnt %d\n", count);
		data->get_sensor_data[sensor_type](data->batch_event.batch_data, &idx_data, &sensor_data);

		data->skipEventReport = false;
		get_timestamp(data, data->batch_event.batch_data, &idx_data, &sensor_data, BATCH_MODE_RUN, sensor_type);
		if (data->skipEventReport == false) {
			data->report_sensor_data[sensor_type](data, &sensor_data);
		}

		data->reportedData[sensor_type] = true;
		count++;
	}
	ssp_dbg("[SSP_BAT] max cnt %d\n", count);
}


// Control batched data with long term
// Ref ssp_read_big_library_task
void ssp_batch_data_read_task(struct work_struct *work)
{
	struct ssp_big *big = container_of(work, struct ssp_big, work);
	struct ssp_data *data = big->data;
	struct ssp_msg *msg;
	int buf_len, residue, ret = 0, index = 0, pos = 0;
	u64 ts = 0;

	mutex_lock(&data->batch_events_lock);
	wake_lock(&data->ssp_wake_lock);

	residue = big->length;
	data->batch_event.batch_length = big->length;
	data->batch_event.batch_data = vmalloc(big->length);
	if (data->batch_event.batch_data == NULL)
	{
		ssp_dbg("[SSP_BAT] batch data alloc fail \n");
		kfree(big);
		wake_unlock(&data->ssp_wake_lock);
		mutex_unlock(&data->batch_events_lock);
		return;
	}

	//ssp_dbg("[SSP_BAT] IN : LENGTH = %d \n", big->length);

	while (residue > 0) {
		buf_len = residue > DATA_PACKET_SIZE
			? DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg),GFP_ATOMIC);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = buf_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = data->batch_event.batch_data + pos;
		msg->free_buffer = 0;

		ret = ssp_spi_sync(big->data, msg, 1000);
		if (ret != SUCCESS) {
			pr_err("[SSP_BAT] read batch data err(%d) ignor\n", ret);
			vfree(data->batch_event.batch_data);
			data->batch_event.batch_data = NULL;
			data->batch_event.batch_length = 0;
			kfree(big);
			wake_unlock(&data->ssp_wake_lock);
			mutex_unlock(&data->batch_events_lock);
			return;
		}

		pos += buf_len;
		residue -= buf_len;
		pr_info("[SSP_BAT] read batch data (%5d / %5d)\n", pos, big->length);
	}

	// TODO: Do not parse, jut put in to FIFO, and wake_up thread.

	// READ DATA FROM MCU COMPLETED 
	//Wake up check
	if(ssp_check_buffer(data)){
		ssp_batch_resume_check(data);

		// PARSE DATA FRAMES, Should run loop
		ts = get_current_timestamp();
		pr_info("[SSP] report start %lld\n", ts);
		ssp_batch_report(data);
		ts = get_current_timestamp();
		pr_info("[SSP] report finish %lld\n", ts);
	}

	vfree(data->batch_event.batch_data);
	data->batch_event.batch_data = NULL;
	data->batch_event.batch_length = 0;
	kfree(big);
	wake_unlock(&data->ssp_wake_lock);
	mutex_unlock(&data->batch_events_lock);
}
#endif
int handle_big_data(struct ssp_data *data, char *pchRcvDataFrame, int *pDataIdx)
{
	u8 bigType = 0;
	struct ssp_big *big = kzalloc(sizeof(*big), GFP_KERNEL);
	big->data = data;
	bigType = pchRcvDataFrame[(*pDataIdx)++];
	memcpy(&big->length, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;
	memcpy(&big->addr, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;

	if (bigType >= BIG_TYPE_MAX) {
		kfree(big);
		return FAIL;
	}

	INIT_WORK(&big->work, data->ssp_big_task[bigType]);
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
	if(bigType != BIG_TYPE_READ_HIFI_BATCH)
		queue_work(data->debug_wq, &big->work);
	else
		queue_work(data->batch_wq, &big->work);
#else
	queue_work(data->debug_wq, &big->work);
#endif
	return SUCCESS;
}

int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength)
{
	int iDataIdx;
	int sensor_type;
	u16 length = 0;

	struct sensor_value sensorsdata;
	s16 caldata[3] = { 0, };
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
	u16 batch_event_count;
	u16 batch_mode;
#else
	struct ssp_time_diff sensortime;
	sensortime.time_diff = 0;
#endif
	data->uIrqCnt++;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		switch (pchRcvDataFrame[iDataIdx++]) {
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
		case MSG2AP_INST_BYPASS_DATA:
			sensor_type = pchRcvDataFrame[iDataIdx++];
			if ((sensor_type < 0) || (sensor_type >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;

			batch_event_count = length;
			batch_mode = length > 1 ? BATCH_MODE_RUN : BATCH_MODE_NONE;
			
			//pr_err("[SSP]: %s batch count (%d)\n", __func__, batch_event_count);

			// TODO: When batch_event_count = 0, we should not run.
			do {
				data->get_sensor_data[sensor_type](pchRcvDataFrame, &iDataIdx, &sensorsdata);
				// TODO: Integrate get_sensor_data function.
				// TODO: get_sensor_data(pchRcvDataFrame, &iDataIdx, &sensorsdata, data->sensor_data_size[sensor_type]);
				// TODO: Divide control data batch and non batch.

				data->skipEventReport = false;
				//if(sensor_type == GYROSCOPE_SENSOR) //check packet recieve time
				//	pr_err("[SSP_PARSE] bbd event time %lld\n", data->timestamp);
				get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata, batch_mode, sensor_type);	
				if (data->skipEventReport == false)
					data->report_sensor_data[sensor_type](data, &sensorsdata);
				batch_event_count--;
				//pr_err("[SSP]: %s batch count (%d)\n", __func__, batch_event_count);
			} while ((batch_event_count > 0) && (iDataIdx < iLength));

			if (batch_event_count > 0)
				pr_err("[SSP]: %s batch count error (%d)\n", __func__, batch_event_count);
			data->reportedData[sensor_type] = true;
			//pr_err("[SSP]: (%d / %d)\n", iDataIdx, iLength);
			break;
		case MSG2AP_INST_DEBUG_DATA:
			sensor_type = print_mcu_debug(pchRcvDataFrame, &iDataIdx, iLength);
			if (sensor_type) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}
			break;
#else
		case MSG2AP_INST_BYPASS_DATA:
			sensor_type = pchRcvDataFrame[iDataIdx++];
			if ((sensor_type < 0) || (sensor_type >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}

			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			sensortime.batch_count = sensortime.batch_count_fixed = length;
			sensortime.batch_mode = length > 1 ? BATCH_MODE_RUN : BATCH_MODE_NONE;
			sensortime.irq_diff = data->timestamp - data->lastTimestamp[sensor_type];

			if (sensortime.batch_mode == BATCH_MODE_RUN) {
				if (data->reportedData[sensor_type] == true) {
					u64 time;
					sensortime.time_diff = div64_long((s64)(data->timestamp - data->lastTimestamp[sensor_type]), (s64)length);
					if (length > 8)
						time = data->adDelayBuf[sensor_type] * 18;
					else if (length > 4)
						time = data->adDelayBuf[sensor_type] * 25;
					else if (length > 2)
						time = data->adDelayBuf[sensor_type] * 50;
					else
						time = data->adDelayBuf[sensor_type] * 130;
					if ((sensortime.time_diff * 10) > time) {
						data->lastTimestamp[sensor_type] = data->timestamp - (data->adDelayBuf[sensor_type] * length);
						sensortime.time_diff = data->adDelayBuf[sensor_type];
					} else {
						time = data->adDelayBuf[sensor_type] * 11;
						if ((sensortime.time_diff * 10) > time)
							sensortime.time_diff = data->adDelayBuf[sensor_type];
					}
				} else {
					if (data->lastTimestamp[sensor_type] < (data->timestamp - (data->adDelayBuf[sensor_type] * length))) {
						data->lastTimestamp[sensor_type] = data->timestamp - (data->adDelayBuf[sensor_type] * length);
						sensortime.time_diff = data->adDelayBuf[sensor_type];
					} else
						sensortime.time_diff = div64_long((s64)(data->timestamp - data->lastTimestamp[sensor_type]), (s64)length);
				}
			} else {
				if (data->reportedData[sensor_type] == false)
					sensortime.irq_diff = data->adDelayBuf[sensor_type];
			}

			do {
				data->get_sensor_data[sensor_type](pchRcvDataFrame, &iDataIdx, &sensorsdata);
				get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata, &sensortime, sensor_type);
				if (sensortime.irq_diff > 1000000)
					data->report_sensor_data[sensor_type](data, &sensorsdata);
				else if ((sensor_type == PROXIMITY_SENSOR) || (sensor_type == PROXIMITY_RAW)
						|| (sensor_type == STEP_COUNTER)   || (sensor_type == STEP_DETECTOR)
						|| (sensor_type == GESTURE_SENSOR) || (sensor_type == SIG_MOTION_SENSOR))
					data->report_sensor_data[sensor_type](data, &sensorsdata);
				else
					pr_err("[SSP]: %s irq_diff is under 1msec (%d)\n", __func__, sensor_type);
				sensortime.batch_count--;
			} while ((sensortime.batch_count > 0) && (iDataIdx < iLength));

			if (sensortime.batch_count > 0)
				pr_err("[SSP]: %s batch count error (%d)\n", __func__, sensortime.batch_count);

			data->lastTimestamp[sensor_type] = data->timestamp;
			data->reportedData[sensor_type] = true;
			break;
		case MSG2AP_INST_DEBUG_DATA:
			sensor_type = print_mcu_debug(pchRcvDataFrame, &iDataIdx, iLength);
			if (sensor_type) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}
			break;
#endif
		case MSG2AP_INST_LIBRARY_DATA:
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			ssp_sensorhub_handle_data(data, pchRcvDataFrame, iDataIdx,
					iDataIdx + length);
			iDataIdx += length;
			break;
		case MSG2AP_INST_BIG_DATA:
			handle_big_data(data, pchRcvDataFrame, &iDataIdx);
			break;
		case MSG2AP_INST_META_DATA:
			sensorsdata.meta_data.what = pchRcvDataFrame[iDataIdx++];
			sensorsdata.meta_data.sensor = pchRcvDataFrame[iDataIdx++];
			report_meta_data(data, &sensorsdata);
			break;
		case MSG2AP_INST_TIME_SYNC:
			data->bTimeSyncing = true;
			break;
		case MSG2AP_INST_GYRO_CAL:
			pr_info("[SSP]: %s - Gyro caldata received from MCU\n",  __func__);
			memcpy(caldata, pchRcvDataFrame + iDataIdx, sizeof(caldata));
			wake_lock(&data->ssp_wake_lock);
			save_gyro_caldata(data, caldata);
			wake_unlock(&data->ssp_wake_lock);
			iDataIdx += sizeof(caldata);
			break;
		case SH_MSG2AP_GYRO_CALIBRATION_EVENT_OCCUR:
			data->gyro_lib_state = GYRO_CALIBRATION_STATE_EVENT_OCCUR;
			break;
		}
	}

	return SUCCESS;
}

void initialize_function_pointer(struct ssp_data *data)
{
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		get_geomagnetic_uncaldata;
	data->get_sensor_data[GEOMAGNETIC_RAW] = get_geomagnetic_rawdata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] =
		get_geomagnetic_caldata;
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->get_sensor_data[GESTURE_SENSOR] = get_gesture_sensordata;
	data->get_sensor_data[PROXIMITY_SENSOR] = get_proximity_sensordata;
	data->get_sensor_data[PROXIMITY_RAW] = get_proximity_rawdata;
#ifdef CONFIG_SENSORS_SSP_SX9306
	data->get_sensor_data[GRIP_SENSOR] = get_grip_sensordata;
#endif
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	data->get_sensor_data[LIGHT_IR_SENSOR] = get_light_ir_sensordata;
#endif
	data->get_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		get_temp_humidity_sensordata;
	data->get_sensor_data[ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[GAME_ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[STEP_DETECTOR] = get_step_det_sensordata;
	data->get_sensor_data[SIG_MOTION_SENSOR] = get_sig_motion_sensordata;
	data->get_sensor_data[GYRO_UNCALIB_SENSOR] = get_uncalib_sensordata;
	data->get_sensor_data[STEP_COUNTER] = get_step_cnt_sensordata;
	data->get_sensor_data[SHAKE_CAM] = get_shake_cam_sensordata;
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	data->get_sensor_data[INTERRUPT_GYRO_SENSOR] = get_3axis_sensordata;
#endif
	data->get_sensor_data[TILT_DETECTOR] = get_tilt_sensordata;
	data->get_sensor_data[PICKUP_GESTURE] = get_pickup_sensordata;
	
	data->get_sensor_data[BULK_SENSOR] = NULL;
	data->get_sensor_data[GPS_SENSOR] = NULL;

	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		report_mag_uncaldata;
	data->report_sensor_data[GEOMAGNETIC_RAW] = report_geomagnetic_raw_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] =
		report_mag_data;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
	data->report_sensor_data[GESTURE_SENSOR] = report_gesture_data;
	data->report_sensor_data[PROXIMITY_SENSOR] = report_prox_data;
	data->report_sensor_data[PROXIMITY_RAW] = report_prox_raw_data;
#ifdef CONFIG_SENSORS_SSP_SX9306
	data->report_sensor_data[GRIP_SENSOR] = report_grip_data;
#endif
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	data->report_sensor_data[LIGHT_IR_SENSOR] = report_light_ir_data;
#endif
	data->report_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		report_temp_humidity_data;
	data->report_sensor_data[ROTATION_VECTOR] = report_rot_data;
	data->report_sensor_data[GAME_ROTATION_VECTOR] = report_game_rot_data;
	data->report_sensor_data[STEP_DETECTOR] = report_step_det_data;
	data->report_sensor_data[SIG_MOTION_SENSOR] = report_sig_motion_data;
	data->report_sensor_data[GYRO_UNCALIB_SENSOR] = report_uncalib_gyro_data;
	data->report_sensor_data[STEP_COUNTER] = report_step_cnt_data;
	data->report_sensor_data[SHAKE_CAM] = report_shake_cam_data;
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	data->report_sensor_data[INTERRUPT_GYRO_SENSOR] = report_interrupt_gyro_data;
#endif
	data->report_sensor_data[TILT_DETECTOR] = report_tilt_data;
	data->report_sensor_data[PICKUP_GESTURE] = report_pickup_data;

	data->report_sensor_data[BULK_SENSOR] = NULL;
	data->report_sensor_data[GPS_SENSOR] = NULL;

	data->ssp_big_task[BIG_TYPE_DUMP] = ssp_dump_task;
	data->ssp_big_task[BIG_TYPE_READ_LIB] = ssp_read_big_library_task;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
	data->ssp_big_task[BIG_TYPE_READ_HIFI_BATCH] = ssp_batch_data_read_task;
#endif
	data->ssp_big_task[BIG_TYPE_VOICE_NET] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_GRAM] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_PCM] = ssp_pcm_dump_task;
	data->ssp_big_task[BIG_TYPE_TEMP] = ssp_temp_task;
}
