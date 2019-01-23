###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: joycon_interface libs

libs: libhidapi-hidraw.so

CC       ?= gcc
CFLAGS   ?= -Wall -g -fpic

CXX      ?= g++
CXXFLAGS ?= -Wall -g -fpic

LDFLAGS  ?= -Wall -g


COBJS     = ../linux/hidapi/linux/hid.o
CPPOBJS   = main.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS_UDEV = `pkg-config libudev --libs` -lrt
LIBS      = $(LIBS_UDEV)
INCLUDES ?= -I../linux/hidapi/hidapi `pkg-config libusb-1.0 --cflags`


# Console Test Program
joycon_interface: $(COBJS) joycon.o main.o
	$(CXX) $(LDFLAGS) $(LIBS_UDEV) -o joycon_interface main.o $(COBJS) joycon.o

# Shared Libs
libhidapi-hidraw.so: $(COBJS)
	$(CC) $(LDFLAGS) $(LIBS_UDEV) -shared -fpic -Wl,-soname,$@.0 $^ -o $@

# Objects

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

joycon.o: joycon.cpp joycon.h
	$(CXX) $(CFLAGS) -c $(INCLUDES) joycon.cpp

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) main.cpp


clean:
	rm -f $(OBJS) joycon_interface libhidapi-hidraw.so joycon.o

.PHONY: clean libs
