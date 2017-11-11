#CC=/usr/local/arm/aarch64-linux-android-4.9/bin/aarch64-linux-android-gcc
CC=/usr/local/arm/arm-2009q3/bin/arm-none-linux-gnueabi-gcc

KERN_PATH=/home/wooki/project/adonis/kernel/samsung/exynos
INCLUDES = -I$(KERN_PATH)/include -I../include -I../../include -I../include

CFLAGS=-static -g -Wall $(INCLUDES)

CSRCS =	test-main.c \
	test-client.c \
	test-api.c \
	test-util.c \
	test-case-verify.c \
	test-case-unit.c \
	test-sample0.c \
	test-sample1.c \
	test-sample2.c \
	libion.c

OBJS = $(CSRCS:.c=.o)

.SUFFIXES:.c.o

all: vpu_test

vpu_test: $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $(OBJS) -lpthread

.c.o:
	$(CC) $(CFLAGS) -g -c -o $@ $<

clean:
	rm -f vpu_test $(OBJS)
