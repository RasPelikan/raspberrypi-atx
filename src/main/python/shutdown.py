#!/usr/bin/python

import os
import sys
import RPi.GPIO as GPIO

ON_INDICATOR_GPIO=23
SHUTDOWN_GPIO=24

GPIO.setmode(GPIO.BCM)
GPIO.setup(ON_INDICATOR_GPIO, GPIO.OUT, initial=GPIO.HIGH)
GPIO.setup(SHUTDOWN_GPIO, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print "   Waiting for shutdown signal on GPIO ", SHUTDOWN_GPIO

try:
        GPIO.wait_for_edge(SHUTDOWN_GPIO, GPIO.RISING)
        os.system("poweroff")
        sys.exit()
except:
        pass
finally:
        GPIO.cleanup()
