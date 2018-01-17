CC=gcc
CFLAGS= -std=gnu99 -I.
DEPS = 

CXX = g++
CXXFLAGS = -Wall -D__STDC_LIMIT_MACROS -I.

C_SRC = \
    main.c \
    digit_protocol.c \
    temperature_conversion.c \
    rs485.c \
    udp-server.c \
    DS18B20.c \
    RPI.c \
    ctrl_logic.c \
    relay_control.c \
    defrost_resistor.c \
    pre_heating_resistor.c \
    post_heating_counter.c \
    jsmn.c

CPP_SRC = json_basic_codecs.cpp json_codecs.cpp
    
C_SRC_OBJS = ${C_SRC:.c=.o}
    
CPP_SRC_OBJS = ${CPP_SRC:.cpp=.o}    

.SUFFIXES:
.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $*.o
.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $< -o $*.o
    
vallox_ctrl: $(C_SRC_OBJS) $(CPP_SRC_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lm -lpthread -lbcm2835