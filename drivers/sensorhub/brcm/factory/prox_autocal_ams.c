#include "../ssp.h"

#define	VENDOR		"AMS"
#define CHIP_ID		"TMD4905"

#define PROX_ADC_BITS_NUM 		14

/*************************************************************************/
/* Functions                                                             */
/*************************************************************************/

static u16 get_proximity_rawdata(struct ssp_data *data)
{
	u16 uRowdata = 0;
	char chTempbuf[4] = { 0 };

	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);

	if (data->bProximityRawEnabled == false) {
		send_instruction(data, ADD_SENSOR, PROXIMITY_RAW, chTempbuf, 4);
		msleep(200);
		uRowdata = data->buf[PROXIMITY_RAW].prox[0];
		send_instruction(data, REMOVE_SENSOR, PROXIMITY_RAW,
			chTempbuf, 4);
	} else {
		uRowdata = data->buf[PROXIMITY_RAW].prox[0];
	}

	return uRowdata;
}

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

static ssize_t proximity_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t proximity_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t proximity_probe_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	bool probe_pass_fail = FAIL;

	if (data->uSensorState & (1 << PROXIMITY_SENSOR)) {
        probe_pass_fail = SUCCESS;
	}
	else {
        probe_pass_fail = FAIL;
	}

	pr_info("[SSP]: %s - All sensor 0x%llx, prox_sensor %d \n",
		__func__, data->uSensorState, probe_pass_fail);
	
	return snprintf(buf, PAGE_SIZE, "%d\n", probe_pass_fail);
}

static ssize_t proximity_thresh_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: ProxThresh = hi - %u \n", data->uProxHiThresh);

	return sprintf(buf, "%u\n", data->uProxHiThresh);
}

static ssize_t proximity_thresh_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewThresh;
	int iRet, i=0;
	u16 prox_bits_mask = 0, prox_non_bits_mask = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	
	while(i<PROX_ADC_BITS_NUM)
		prox_bits_mask += (1 << i++);

	while(i<16)
		prox_non_bits_mask += (1<<i++);

	iRet = kstrtou16(buf, 10, &uNewThresh);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrto16 failed.(%d)\n", __func__, iRet);
	else {
		if(uNewThresh & prox_non_bits_mask)
			pr_err("[SSP]: %s - allow %ubits.(%d)\n", __func__, PROX_ADC_BITS_NUM,uNewThresh);
		else {
			uNewThresh &= prox_bits_mask;
			data->uProxHiThresh = uNewThresh;
		}
	}

	ssp_dbg("[SSP]: %s - new prox threshold : hi - %u \n", __func__, data->uProxHiThresh);
	
	return size;
}

static ssize_t proximity_thresh_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: ProxThresh = lo - %u \n", data->uProxLoThresh);

	return sprintf(buf, "%u\n", data->uProxLoThresh);
}

static ssize_t proximity_thresh_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewThresh;
	int iRet, i=0;
	u16 prox_bits_mask = 0, prox_non_bits_mask = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	
	while(i<PROX_ADC_BITS_NUM)
		prox_bits_mask += (1 << i++);

	while(i<16)
		prox_non_bits_mask += (1<<i++);

	iRet = kstrtou16(buf, 10, &uNewThresh);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrto16 failed.(%d)\n", __func__, iRet);
	else {
		if(uNewThresh & prox_non_bits_mask)
			pr_err("[SSP]: %s - allow %ubits.(%d)\n", __func__, PROX_ADC_BITS_NUM,uNewThresh);
		else {
			uNewThresh &= prox_bits_mask;
			data->uProxLoThresh = uNewThresh;
		}
	}

	ssp_dbg("[SSP]: %s - new prox threshold : lo - %u \n", __func__, data->uProxLoThresh);
	
	return size;
}

static ssize_t proximity_thresh_high_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: ProxThresh = hical - %u \n", data->uProxHiThresh_cal);

	return sprintf(buf, "%u\n", data->uProxHiThresh_cal);
}

static ssize_t proximity_thresh_high_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewThresh;
	int iRet, i=0;
	u16 prox_bits_mask = 0, prox_non_bits_mask = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	
	while(i<PROX_ADC_BITS_NUM)
		prox_bits_mask += (1 << i++);

	while(i<16)
		prox_non_bits_mask += (1<<i++);

	iRet = kstrtou16(buf, 10, &uNewThresh);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrto16 failed.(%d)\n", __func__, iRet);
	else {
		if(uNewThresh & prox_non_bits_mask)
			pr_err("[SSP]: %s - allow %ubits.(%d)\n", __func__, PROX_ADC_BITS_NUM,uNewThresh);
		else {
			uNewThresh &= prox_bits_mask;
			data->uProxHiThresh_cal = uNewThresh;
		}
	}

	ssp_dbg("[SSP]: %s - new prox threshold : hical - %u \n", __func__, data->uProxHiThresh_cal);
	
	return size;
}

static ssize_t proximity_thresh_low_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: ProxThresh = local - %u \n", data->uProxLoThresh_cal);

	return sprintf(buf, "%u\n", data->uProxLoThresh_cal);
}

static ssize_t proximity_thresh_low_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewThresh;
	int iRet, i=0;
	u16 prox_bits_mask = 0, prox_non_bits_mask = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	
	while(i<PROX_ADC_BITS_NUM)
		prox_bits_mask += (1 << i++);

	while(i<16)
		prox_non_bits_mask += (1<<i++);

	iRet = kstrtou16(buf, 10, &uNewThresh);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrto16 failed.(%d)\n", __func__, iRet);
	else {
		if(uNewThresh & prox_non_bits_mask)
			pr_err("[SSP]: %s - allow %ubits.(%d)\n", __func__, PROX_ADC_BITS_NUM,uNewThresh);
		else {
			uNewThresh &= prox_bits_mask;
			pr_err("[SSP]: %s - don't satisfy condition.\n",__func__);
		}
	}

	ssp_dbg("[SSP]: %s - new prox threshold : local - %u \n", __func__, data->uProxLoThresh_cal);
	
	return size;
}


static ssize_t proximity_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", get_proximity_rawdata(data));
}

static ssize_t proximity_default_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet, iReties = 0;
	struct ssp_msg *msg;
	u8 buffer[8] = {0,};
	int trim;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		pr_err("[SSP]: %s - failed to allocate memory\n", __func__);
		return FAIL;
	}
	msg->cmd = MSG2SSP_AP_PROX_GET_TRIM;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	trim = (int)buffer[0];
	
	pr_info("[SSP] %s - %d \n", __func__, trim);
	
	return snprintf(buf, PAGE_SIZE, "%d\n", trim);
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[PROXIMITY_RAW].prox[1],
		data->buf[PROXIMITY_RAW].prox[2],
		data->buf[PROXIMITY_RAW].prox[3]);
}

/*
	Proximity Raw Sensor Register/Unregister
*/
static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char chTempbuf[4] = { 0 };
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);

	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		send_instruction(data, ADD_SENSOR, PROXIMITY_RAW, chTempbuf, 4);
		data->bProximityRawEnabled = true;
	} else {
		send_instruction(data, REMOVE_SENSOR, PROXIMITY_RAW,
			chTempbuf, 4);
		data->bProximityRawEnabled = false;
	}

	return size;
}

static ssize_t proximity_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: uProxThresh : hi : %u lo : %u, hi-cal : %u\n",
		data->uProxHiThresh, data->uProxLoThresh, data->uProxHiThresh_cal);

	return sprintf(buf, "%u,%u,%u\n", data->uProxHiThresh_cal,
		data->uProxHiThresh, data->uProxLoThresh);
}

static ssize_t barcode_emul_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->bBarcodeEnabled);
}

static ssize_t barcode_emul_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable)
		set_proximity_barcode_enable(data, true);
	else
		set_proximity_barcode_enable(data, false);

	return size;
}

static DEVICE_ATTR(vendor, S_IRUGO, proximity_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, proximity_name_show, NULL);
static DEVICE_ATTR(prox_probe, S_IRUGO, proximity_probe_show, NULL);
static DEVICE_ATTR(thresh_high, S_IRUGO | S_IWUSR | S_IWGRP, proximity_thresh_high_show, proximity_thresh_high_store);
static DEVICE_ATTR(thresh_low, S_IRUGO | S_IWUSR | S_IWGRP, proximity_thresh_low_show, proximity_thresh_low_store);
static DEVICE_ATTR(thresh_cal_high, S_IRUGO | S_IWUSR | S_IWGRP, proximity_thresh_high_cal_show, proximity_thresh_high_cal_store);
static DEVICE_ATTR(thresh_cal_low, S_IRUGO | S_IWUSR | S_IWGRP, proximity_thresh_low_cal_show, proximity_thresh_low_cal_store);
static DEVICE_ATTR(prox_trim, S_IRUGO, proximity_default_trim_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, proximity_raw_data_show, NULL);
static DEVICE_ATTR(prox_avg, S_IRUGO | S_IWUSR | S_IWGRP, proximity_avg_show, proximity_avg_store);

static DEVICE_ATTR(prox_cal, S_IRUGO, proximity_cal_show, NULL);

static DEVICE_ATTR(barcode_emul_en, S_IRUGO | S_IWUSR | S_IWGRP, barcode_emul_enable_show, barcode_emul_enable_store);

static struct device_attribute *prox_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_prox_probe,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_thresh_cal_high,
	&dev_attr_thresh_cal_low,
	&dev_attr_prox_trim,
	&dev_attr_raw_data,
	&dev_attr_prox_avg,
	&dev_attr_prox_cal,
	&dev_attr_barcode_emul_en,
	NULL,
};

void initialize_prox_factorytest(struct ssp_data *data)
{
	sensors_register(data->prox_device, data,
		prox_attrs, "proximity_sensor");
}

void remove_prox_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->prox_device, prox_attrs);
}


