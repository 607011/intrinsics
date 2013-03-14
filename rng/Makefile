# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

SRC = mersenne_twister.cpp
OBJ = $(SRC:.cpp=.o)
OUT = librng.a
INCLUDES = -I../sharedutil
CXX = g++
LIBS =
LDFLAGS =
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" configured

x64-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" configured

x86-debug:
	$(MAKE) CXXFLAGS="-Wall -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" configured

x64-debug:
	$(MAKE) CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" configured

dep:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -MM $(SRC) > $@

include dep

configured: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)

mrproper: clean
	rm -f dep
