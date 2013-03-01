SRC = sharedutil.cpp
OBJ = $(SRC:.cpp=.o)
OUT = libsharedutil.a
INCLUDES = 
CXXFLAGS = -Wall -O2 -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32
CXX = g++
LIBS = 
LDFLAGS = -g
.SUFFIXES = .cpp

all: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)
