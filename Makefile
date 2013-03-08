# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

MAKE = make
SUBDIRS = sharedutil rng crc rdrandck rdrand 


all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	$(MAKE) TARGET="x86-release" configured

x64-release:
	$(MAKE) TARGET="x64-release" configured

x86-debug:
	$(MAKE) TARGET="x86-debug" configured

x64-debug:
	$(MAKE) TARGET="x64-debug" configured

configured:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $(TARGET); \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

mrproper:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir mrproper; \
	done
