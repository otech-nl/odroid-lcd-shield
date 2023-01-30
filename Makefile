lcd: lcd.c
	gcc -g -o lcd lcd.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt
