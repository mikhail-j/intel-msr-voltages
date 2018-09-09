CC:=gcc
CFLAGS:=--std=gnu99

.PHONY: default install clean uninstall

default: intel-msr-voltages

intel-msr-voltages: intel-msr-voltages.c intel-msr-voltages.h
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm intel-msr-voltages

install:
	cp intel-msr-voltages.conf /etc/
	cp intel-msr-voltages /usr/bin/
	cp intel-msr-voltages.service /etc/systemd/system/
	systemctl enable intel-msr-voltages

uninstall:
	rm /etc/intel-msr-voltages.conf
	rm /usr/bin/intel-msr-voltages
	systemctl disable intel-msr-voltages
	rm /etc/systemd/system/intel-msr-voltages.service
