SRC = crc.cpp

OBJ = $(SRC:.cpp=.o)

OUT = crc64

INCLUDES = -I../sharedutil -I../rng

CXXFLAGS = -Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -std=c++11

CXX = g++

LIBS = -L../sharedutil -L../rng -lsharedutil -lrng -lrt -lpthread

LDFLAGS = -s

.SUFFIXES = .cpp

all: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) $(OUT)
