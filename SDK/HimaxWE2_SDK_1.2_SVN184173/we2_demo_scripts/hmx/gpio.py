# Here is the SPI Slave HAL.
# For Himax WE-I Auto Test Framework.

import logging
import logger as HxLogger
from time import sleep

# FT4222 Driver module
import ft4222
import ft4222.GPIO

logging.basicConfig(level=logging.DEBUG)

# Set Logger
#Log = logging.getLogger(HxLogger.SPI_HAL)
Log = logging
    
class Gpio():
    def __init__(self):
        Log.info('Open GPIO device...')
        # Open FT4222 Device
        TARGET_DEV_DESC = 'FT4222 B'
        dev_hnd = None
        try:
            dev_hnd = ft4222.openByDescription(TARGET_DEV_DESC)
            logging.debug('openByDescription done.')
        except:
            logging.error('Failed to Open %s.' % TARGET_DEV_DESC)

        try:
            dev_hnd.setSuspendOut(False)
            Log.debug('spi_SetSuspendOut done.')
        except Exception as e:
            Log.error('Failed to spi_SetSuspendOut due to %s' % e)

        try:
            dev_hnd.setWakeUpInterrupt(False)
            Log.debug('setWakeUpInterrupt done.')
        except Exception as e:
            Log.error('Failed to setWakeUpInterrut due to %s' % e)

        # Init SPI GPIO
        try:
            dev_hnd.gpio_Init(gpio0=ft4222.GPIO.Dir.OUTPUT, gpio1=ft4222.GPIO.Dir.OUTPUT, gpio2=ft4222.GPIO.Dir.OUTPUT, gpio3=ft4222.GPIO.Dir.OUTPUT)
            Log.debug('spi_GpioInit done.')
        except Exception as e:
            Log.error('Failed to spi_GpioInit due to %s' % e)
        
        if dev_hnd != None:
            self.dev_hnd = dev_hnd
            Log.debug('Open FT4222 GPIO done.')
        else:
            Log.error('Open FT4222 GPIO FAILED!')

    def close(self):
        try:
            self.dev_hnd.close()
            Log.info('FT4222 GPIO closed.')
        except Exception as e:
            Log.error('Failed to close due to %s' % e)
    
    def writeGPIO2(self, value):	        
        try:
            self.dev_hnd.gpio_Write(portNum=ft4222.GPIO.Port.P2, value=value)
            Log.debug('Write GPIO2 done.')
        except Exception as e:
            Log.error('Failed to set GPIO2 to low due to %s' % e)

    def writeGPIO3(self, value):	        
        try:
            self.dev_hnd.gpio_Write(portNum=ft4222.GPIO.Port.P3, value=value)
            Log.debug('Write GPIO3 done.')
        except Exception as e:
            Log.error('Failed to set GPIO3 to low due to %s' % e)

    def readGPIO2(self):
        try:
            value_gpio2 = self.dev_hnd.gpio_Read(ft4222.GPIO.Port.P2)
        except Exception as e:
            Log.error('Failed to read GPIO2 due to %s' % e)
            return None
        return value_gpio2

if __name__ == '__main__':
    gpio = Gpio()

    print('Set GPIO_02 to HIGH.')
    gpio.writeGPIO2(True)
    gpio.writeGPIO3(True)
    sleep(0.1)

    print('Set GPIO_02 to LOW.')
    gpio.writeGPIO2(False)
    gpio.writeGPIO3(False)
    gpio.close()

        