# Protocoale de comunicatii
# Laborator 11 - E-mail
# Makefile

CFLAGS = -Wall -g

SERVER_IP = 127.0.0.1
FILENAME = file.txt

default: send_mail

send_mail: send_mail.c
	gcc $(CFLAGS) send_mail.c -o send_mail

run: send_mail
	./send_mail ${SERVER_IP} ${FILENAME}

.PHONY: clean

clean:
	rm -f send_mail
