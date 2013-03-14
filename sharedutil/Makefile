# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

SRC = sharedutil.cpp
OBJ = $(SRC:.cpp=.o)
OUT = libsharedutil.a
INCLUDES = 
CXX = g++
LIBS = 
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32" LDFLAGS="-s" configured

x64-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32" LDFLAGS="-s" configured

x86-debug:
	$(MAKE) CXXFLAGS="-Wall -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" LDFLAGS="-g" configured

x64-debug:
	$(MAKE) CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" LDFLAGS="-g" configured

configured: $(OUT)

.cpp.o: sharedutil.h
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)
