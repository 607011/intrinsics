# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

SRC = rdrandck.cpp
OUT = rdrandck
DEPS = 
OBJ = $(SRC:.cpp=.o)
INCLUDES = -I../sharedutil
CXX = g++
LIBS = -L../sharedutil -lsharedutil
LDFLAGS =
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" configured

x64-release:
	$(MAKE) CXXFLAGS="-Wall -O2 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG -s" configured

x86-debug:
	$(MAKE) CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" configured

x64-debug:
	$(MAKE) CXXFLAGS="-Wall -O2 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" configured


configured: $(OUT)

.cpp.o:	$(DEPS)
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) $(OUT)
