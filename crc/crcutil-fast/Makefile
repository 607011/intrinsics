SRC = crc32c_sse4.cpp \
  multiword_128_64_gcc_amd64_sse2.cpp \
  multiword_64_64_cl_i386_mmx.cpp \
  multiword_64_64_gcc_amd64_asm.cpp \
  multiword_64_64_gcc_i386_mmx.cpp \
  multiword_64_64_intrinsic_i386_mmx.cpp
OBJ = $(SRC:.cpp=.o)
CXX = g++
INCLUDES = 
LIBS = 
LDFLAGS = -s
.SUFFIXES = .cpp

all:
	@echo Bitte mit \"make target\" aufrufen, wobei \"target\" eine der folgenden Optionen ist:
	@echo x86-release
	@echo x64-release
	@echo x86-debug
	@echo x64-debug

x86-release:
	make CXXFLAGS="-Wall -O3 -m32 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG" OUT="libcrcfast.a" configured

x64-release:
	make CXXFLAGS="-Wall -O3 -m64 -fno-rtti -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DNDEBUG" OUT="libcrcfast64.a" configured

x86-debug:
	make CXXFLAGS="-Wall -m32 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="libcrcfast.a" configured

x64-debug:
	make CXXFLAGS="-Wall -m64 -march=corei7 -msse4.2 -mrdrnd -mcrc32 -DDEBUG -ggdb" OUT="libcrcfast64.a" configured

configured: $(OUT)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) libcrcfast.a libcrcfast64.a

