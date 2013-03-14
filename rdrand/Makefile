# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

SRC = rdrand.cpp
OBJ = $(SRC:.cpp=.o)
CXX = g++
INCLUDES = -I../sharedutil -I../rng
LIBS = -L../sharedutil -L../rng -lsharedutil -lrng -lrt -lpthread
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	$(MAKE) CXXFLAGS="-Wall -O3 -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" OUT="rdrand" configured

x64-release:
	$(MAKE) CXXFLAGS="-Wall -O3 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" OUT="rdrand64" configured

x86-debug:
	$(MAKE) CXXFLAGS="-Wall -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="rdrand" configured

x64-debug:
	$(MAKE) CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="rdrand64" configured


configured: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) rdrand rdrand64

mrproper: clean
	rm -f *.dat
