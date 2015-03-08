CC=gcc
CFLAGS= -std=gnu99 -I.
DEPS = 
OBJ = \
    main.o \
    digit_protocol.o \
    temperature_conversion.o \
    rs485.o \
    udp-server.o \
    DS18B20.o \
    RPI.o \
    ctrl_logic.o \
    relay_control.o \
    defrost_resistor.o \
    pre_heating_resistor.o \
    post_heating_counter.o \
    jsmn.o \
    json_codecs.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

vallox_ctrl: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm -lpthread -lbcm2835