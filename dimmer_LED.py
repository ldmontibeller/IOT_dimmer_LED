import RPi.GPIO as GPIO
import pygame
import sys

LED_pin = 4
pygame.init()
#myfont = pygame.font.SysFont("monospace", 15)
pygame.display.set_mode((640,480))

GPIO.setmode(GPIO.BCM)
#GPIO.setwarnings(False)
GPIO.setup(LED_pin,GPIO.OUT)

duty_cycle = 100 
dimmer_LED = GPIO.PWM(LED_pin,50)
dimmer_LED.start(duty_cycle) #Changing the duty cycle of the PWM changes the brigthness

#label = myfont.render("Use UP and DOWN to change the brightness of the LED, press ESC to quit.", 1, (255,255,0))
#screen.blit(label, (50, 50))

while True:
	events = pygame.event.get()
	for event in events: 
		if event.type == pygame.KEYDOWN: #KEYDOWN is ANY key pressed, now have to add another if to specify the key
			if event.key == pygame.K_ESCAPE:
				dimmer_LED.stop()   
				GPIO.cleanup()
				pygame.quit()
				sys.exit() 
			if event.key == pygame.K_DOWN:
				duty_cycle -= 10
				if duty_cycle <= 0:
					duty_cycle = 0
			if event.key == pygame.K_UP:
				duty_cycle += 10
				if duty_cycle >= 100:
					duty_cycle = 100  
	dimmer_LED.ChangeDutyCycle(duty_cycle)
	pygame.event.pump()
 
