# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

SRC = crc.cpp
OBJ = $(SRC:.cpp=.o)
CXX = g++
DEPS = Makefile
INCLUDES = -I../sharedutil -I../rng -Icrcutil-fast
LIBS = -L../sharedutil -L../rng -lsharedutil -lrng -lrt -lpthread -Lcrcutil-fast -lcrcfast64
LDFLAGS = -s
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	make CXXFLAGS="-Wall -O3 -m32 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG" OUT="crc" configured

x64-release:
	make CXXFLAGS="-Wall -O3 -m64 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG" OUT="crc64" configured

x86-debug:
	make CXXFLAGS="-Wall -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="crc" configured

x64-debug:
	make CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="crc64" configured

configured: $(OUT)

crc.o: crc.cpp crc32.h $(DEPS)
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c crc.cpp -o crc.o

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) crc crc64
