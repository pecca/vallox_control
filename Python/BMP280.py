# Distributed with a free-will license.
# Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
# BMP280
# This code is designed to work with the BMP280_I2CS I2C Mini Module available from ControlEverything.com.
# https://www.controleverything.com/content/Barometer?sku=BMP280_I2CSs#tabs-0-product_tabset-2


#import smbus
import time

ctrl_meas_req   = 0xF4
config_reg      = 0xF5

oversampling_map = {
    'skipped'           : '000',
    'oversampling_x1'   : '001',
    'oversampling_x2'   : '010',
    'oversampling_x4'   : '011',
    'oversampling_x8'   : '100',
    'oversampling_x16'  : '101',    
}

power_mode_map = {
    'sleep'     : '00',
    'forced'    : '01',
    'normal'    : '11'
}

filter_map = {
    'off'       : '000',
    'coeff_2'   : '001',
    'coeff_4'   : '010',
    'coeff_6'   : '011',
    'coeff_8'   : '100',
    'coeff_16'  : '101' 
}

standby_map = {
    '0_5ms'     :   '000',
    '62_5ms'    :   '001',
    '125ms'     :   '010',
    '250ms'     :   '011',
    '500ms'     :   '100',
    '1000ms'    :   '101',
    '2000ms'    :   '110',
    '4000ms'    :   '111'
}

def create_ctrl_meas_req(press_oversampling, temp_oversampling, power_mode):
    binStr = press_oversampling + temp_oversampling + power_mode
    return int(binStr, 2)
    

def create_config_req(standby_time, filter):
    padding = '0'
    enables_3_wire_spi = '0'
    
    binStr = standby_time + filter + padding + enables_3_wire_spi
    return int(binStr, 2) 
 
    

                                  
# Get I2C bus
bus = smbus.SMBus(1)

# BMP280 address, 0x76(118)
# Read data back from 0x88(136), 24 bytes
b1 = bus.read_i2c_block_data(0x76, 0x88, 24)

# Convert the data
# Temp coefficents
dig_T1 = b1[1] * 256 + b1[0]
dig_T2 = b1[3] * 256 + b1[2]
if dig_T2 > 32767 :
    dig_T2 -= 65536
dig_T3 = b1[5] * 256 + b1[4]
if dig_T3 > 32767 :
    dig_T3 -= 65536

# Pressure coefficents
dig_P1 = b1[7] * 256 + b1[6]
dig_P2 = b1[9] * 256 + b1[8]
if dig_P2 > 32767 :
    dig_P2 -= 65536
dig_P3 = b1[11] * 256 + b1[10]
if dig_P3 > 32767 :
    dig_P3 -= 65536
dig_P4 = b1[13] * 256 + b1[12]
if dig_P4 > 32767 :
    dig_P4 -= 65536
dig_P5 = b1[15] * 256 + b1[14]
if dig_P5 > 32767 :
    dig_P5 -= 65536
dig_P6 = b1[17] * 256 + b1[16]
if dig_P6 > 32767 :
    dig_P6 -= 65536
dig_P7 = b1[19] * 256 + b1[18]
if dig_P7 > 32767 :
    dig_P7 -= 65536
dig_P8 = b1[21] * 256 + b1[20]
if dig_P8 > 32767 :
    dig_P8 -= 65536
dig_P9 = b1[23] * 256 + b1[22]
if dig_P9 > 32767 :
    dig_P9 -= 65536

ctrl_meas =  create_ctrl_meas_req(oversampling_map['oversampling_x16'],
                                  oversampling_map['oversampling_x2'],
                                  power_mode_map['normal'])
                                  
# BMP280 address, 0x76(118)
# Select Control measurement register, 0xF4(244)
#		0x27(39)	Pressure and Temperature Oversampling rate = 1
#					Normal mode
bus.write_byte_data(0x76, 0xF4, ctrl_meas)


config = create_config_req(standby_map['1000ms'], filter_map['coeff_16'])

# BMP280 address, 0x76(118)
# Select Configuration register, 0xF5(245)
#		0xA0(00)	Stand_by time = 1000 ms
bus.write_byte_data(0x76, 0xF5, config)

time.sleep(0.5)

while(True):

    # BMP280 address, 0x76(118)
    # Read data back from 0xF7(247), 8 bytes
    # Pressure MSB, Pressure LSB, Pressure xLSB, Temperature MSB, Temperature LSB
    # Temperature xLSB, Humidity MSB, Humidity LSB
    data = bus.read_i2c_block_data(0x76, 0xF7, 8)

    # Convert pressure and temperature data to 19-bits
    adc_p = ((data[0] * 65536) + (data[1] * 256) + (data[2] & 0xF0)) / 16
    adc_t = ((data[3] * 65536) + (data[4] * 256) + (data[5] & 0xF0)) / 16

    # Temperature offset calculations
    var1 = ((adc_t) / 16384.0 - (dig_T1) / 1024.0) * (dig_T2)
    var2 = (((adc_t) / 131072.0 - (dig_T1) / 8192.0) * ((adc_t)/131072.0 - (dig_T1)/8192.0)) * (dig_T3)
    t_fine = (var1 + var2)
    cTemp = (var1 + var2) / 5120.0
    fTemp = cTemp * 1.8 + 32

    # Pressure offset calculations
    var1 = (t_fine / 2.0) - 64000.0
    var2 = var1 * var1 * (dig_P6) / 32768.0
    var2 = var2 + var1 * (dig_P5) * 2.0
    var2 = (var2 / 4.0) + ((dig_P4) * 65536.0)
    var1 = ((dig_P3) * var1 * var1 / 524288.0 + ( dig_P2) * var1) / 524288.0
    var1 = (1.0 + var1 / 32768.0) * (dig_P1)
    p = 1048576.0 - adc_p
    p = (p - (var2 / 4096.0)) * 6250.0 / var1
    var1 = (dig_P9) * p * p / 2147483648.0
    var2 = p * (dig_P8) / 32768.0
    pressure = (p + (var1 + var2 + (dig_P7)) / 16.0) / 100

    # Output data to screen
    print "Pressure: %.2f hPa, temperature: %.2f C" %pressure %cTemp
    time.sleep(1.0)