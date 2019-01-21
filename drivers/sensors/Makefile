obj-$(CONFIG_SENSORS_SSC)	+= sensors_ssc.o
obj-$(CONFIG_SENSORS_CORE)      += sensors_core.o

# acclerometer_sensor
obj-$(CONFIG_SENSORS_K2HH)          += k2hh.o

# optical_sensor
obj-$(CONFIG_SENSORS_CM3323)	    += cm3323.o
obj-$(CONFIG_SENSORS_CM36655)	+= cm36655.o
obj-$(CONFIG_SENSORS_CM36686)	+= cm36686.o
obj-$(CONFIG_SENSORS_TMD3700)	+= tmd37xx.o
obj-$(CONFIG_SENSORS_STK3013)	+= stk3013.o
obj-$(CONFIG_SENSORS_GP2AP070S)	+= gp2ap070s.o
obj-$(CONFIG_SENSORS_CM36672P)	+= cm36672p.o
# magnetic_sensor
obj-$(CONFIG_SENSORS_AK09911C)	+= ak09911c.o
obj-$(CONFIG_SENSORS_AK09916C)	+= ak09916c.o
obj-$(CONFIG_SENSORS_YAS539)	+= yas_mag_drv-yas539.o yas_mag_kernel.o
obj-$(CONFIG_SENSORS_YAS537)        += yas537/yas_mag_drv-yas537.o yas537/yas_mag_kernel.o

# grip_sensor
obj-$(CONFIG_SENSORS_SX9306)        += sx9306.o
obj-$(CONFIG_SENSORS_SX9310)        +=sx9310.o

# sub_grip_sensor
obj-$(CONFIG_SENSORS_SX9306_SUB)    += sx9306_sub.o

# wifi_grip_sensor
obj-$(CONFIG_SENSORS_SX9306_WIFI)    += sx9306_wifi.o

# Bosch sensor driver.
obj-$(CONFIG_SENSORS_BMI160)    += bmi160_driver.o bmi160.o
ifeq ($(CONFIG_BMI160_MAG_INTERFACE_SUPPORT),y)
		EXTRA_CFLAGS += -DBMI160_MAG_INTERFACE_SUPPORT
endif
ifeq ($(CONFIG_SENSORS_BMI160_ENABLE_INT1),y)
		EXTRA_CFLAGS += -DBMI160_ENABLE_INT1
endif

ifeq ($(CONFIG_SENSORS_BMI160_ENABLE_INT2),y)
		EXTRA_CFLAGS += -DBMI160_ENABLE_INT2
endif

obj-$(CONFIG_SENSORS_BMI160_I2C)  += bmi160_i2c.o
ifeq ($(CONFIG_SENSORS_BMI160_I2C),y)
		EXTRA_CFLAGS += -DBMI_USE_BASIC_I2C_FUNC
endif
obj-$(CONFIG_SENSORS_BMI160_SPI)  += bmi160_spi.o

obj-$(CONFIG_ST_LSM6DS3) += st_lsm6ds3.o
st_lsm6ds3-objs := st_lsm6ds3_core.o

obj-$(CONFIG_ST_LSM6DS3_I2C) 	+= st_lsm6ds3_i2c.o
obj-$(CONFIG_ST_LSM6DS3_SPI) 	+= st_lsm6ds3_spi.o