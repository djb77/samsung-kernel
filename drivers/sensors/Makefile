obj-$(CONFIG_SENSORS_SSC)	+= sensors_ssc.o
obj-$(CONFIG_SENSORS_CORE)	+= sensors_core.o

# acclerometer_sensor
obj-$(CONFIG_SENSORS_K2HH)			+= k2hh.o
obj-$(CONFIG_SENSORS_LIS2DS)		+= lis2ds_core.o
obj-$(CONFIG_SENSORS_LIS2DS_I2C)	+= lis2ds_i2c.o

# optical_sensor
obj-$(CONFIG_SENSORS_CM3323)	+= cm3323.o
obj-$(CONFIG_SENSORS_CM36655)	+= cm36655.o
obj-$(CONFIG_SENSORS_CM36657)	+= cm36657.o
obj-$(CONFIG_SENSORS_CM36658)	+= cm36658.o
obj-$(CONFIG_SENSORS_CM36686)	+= cm36686.o
obj-$(CONFIG_SENSORS_STK3328)	+= stk3328.o
obj-$(CONFIG_SENSORS_TMD3700)	+= tmd37xx.o
obj-$(CONFIG_SENSORS_STK3013)	+= stk3013.o
obj-$(CONFIG_SENSORS_GP2AP070S)	+= gp2ap070s.o
obj-$(CONFIG_SENSORS_CM36672P)	+= cm36672p.o

# magnetic_sensor
obj-$(CONFIG_SENSORS_AK09911C)	+= ak09911c.o
obj-$(CONFIG_SENSORS_AK09916C)	+= ak09916c.o
obj-$(CONFIG_SENSORS_YAS539)	+= yas_mag_drv-yas539.o yas_mag_kernel.o
obj-$(CONFIG_SENSORS_YAS537)	+= yas537/yas_mag_drv-yas537.o yas537/yas_mag_kernel.o

# grip_sensor
obj-$(CONFIG_SENSORS_SX9306)	+= sx9306.o
obj-$(CONFIG_SENSORS_SX9310)	+= sx9310.o
obj-$(CONFIG_SENSORS_SX9320)	+= sx9320.o
obj-$(CONFIG_SENSORS_A96T3X6)	+= a96t3x6.o

# wifi_grip_sensor
obj-$(CONFIG_SENSORS_SX9306_WIFI)	+= sx9306_wifi.o
obj-$(CONFIG_SENSORS_SX9320_WIFI)	+= sx9320_wifi.o
obj-$(CONFIG_SENSORS_A96T3X6_WIFI)	+= a96t3x6_wifi.o

# sub_grip_sensor
obj-$(CONFIG_SENSORS_SX9306_SUB)	+= sx9306_sub.o
obj-$(CONFIG_SENSORS_SX9320_SUB)	+= sx9320_sub.o
obj-$(CONFIG_SENSORS_A96T3X6_SUB)	+= a96t3x6_sub.o

# Bosch sensor
obj-$(CONFIG_SENSORS_BMI160)	+= bmi160_driver.o bmi160.o
ifeq ($(CONFIG_BMI160_MAG_INTERFACE_SUPPORT),y)
		EXTRA_CFLAGS += -DBMI160_MAG_INTERFACE_SUPPORT
endif
ifeq ($(CONFIG_SENSORS_BMI160_ENABLE_INT1),y)
		EXTRA_CFLAGS += -DBMI160_ENABLE_INT1
endif
ifeq ($(CONFIG_SENSORS_BMI160_ENABLE_INT2),y)
		EXTRA_CFLAGS += -DBMI160_ENABLE_INT2
endif
obj-$(CONFIG_SENSORS_BMI160_I2C)	+= bmi160_i2c.o
ifeq ($(CONFIG_SENSORS_BMI160_I2C),y)
		EXTRA_CFLAGS += -DBMI_USE_BASIC_I2C_FUNC
endif
obj-$(CONFIG_SENSORS_BMI160_SPI)	+= bmi160_spi.o

# STM sensor
obj-$(CONFIG_ST_LSM6DS3) += st_lsm6ds3.o
st_lsm6ds3-objs := st_lsm6ds3_core.o

obj-$(CONFIG_ST_LSM6DS3_I2C)	+= st_lsm6ds3_i2c.o
obj-$(CONFIG_ST_LSM6DS3_SPI)	+= st_lsm6ds3_spi.o
obj-$(CONFIG_SENSORS_LSM6DSL)	+= lsm6dsl_core.o
obj-$(CONFIG_SENSORS_LSM6DSL_I2C)	+= lsm6dsl_i2c.o
obj-$(CONFIG_SENSORS_LSM6DSL_SPI)	+= lsm6dsl_spi.o
