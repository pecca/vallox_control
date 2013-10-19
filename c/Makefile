CC=gcc
CFLAGS=-std=gnu99 -I. -lpthread -lbcm2835
DEPS = 
OBJ = vallox_ctrl.o digit_protocol.o temperature_conversion.o rs485.o udp-server.o DS18B20.o Adafruit_DHT.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

vallox_ctrl: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)