SRC = mersenne_twister.cpp

DEPS = circ.h marsaglia.h mcg.h mersenne_twister.h 

OBJ = $(SRC:.cpp=.o)

OUT = librng.a

INCLUDES = -I../sharedutil

CXXFLAGS = -Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 

CXX = g++

LIBS = 

LDFLAGS = -g

.SUFFIXES = .cpp

all: $(OUT)

.cpp.o:	$(DEPS)
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)
