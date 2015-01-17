CC=gcc
CFLAGS= -std=gnu99 -I. -lpthread -lbcm2835
DEPS = 
OBJ = \
    vallox_ctrl.o \
    digit_protocol.o \
    temperature_conversion.o \
    rs485.o \
    tcp-server.o \
    udp-server.o \
    DS18B20.o \
    Adafruit_DHT.o \
    ctrl_logic.o \
    RPI.o \
    relay_control.o \
    defrost_resistor.o \
    pre_heating_resistor.o \
    post_heating_counter.o \
    ctrl_variables.o \
    jsmn.o \
    json_codecs.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

vallox_ctrl: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm