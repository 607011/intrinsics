# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

SRC = crc.cpp
OBJ = $(SRC:.cpp=.o)
CXX = g++
INCLUDES = -I../sharedutil -I../rng
LIBS = -L../sharedutil -L../rng -lsharedutil -lrng -lrt -lpthread
LDFLAGS = -s
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86
	@echo x64

x86:
	make CXXFLAGS="-Wall -O3 -m32 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32" OUT="crc" configured

x64:
	make CXXFLAGS="-Wall -O3 -m64 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32" OUT="crc64" configured

configured: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) crc crc64
