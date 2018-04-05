CC = g++
INCLUDE_DIR = -I/usr/local/include -I../include
CFLAGS = -std=c++0x
CFLAGS += `pkg-config --cflags opencv`
OPENCV = `pkg-config --libs opencv`
PROG = TugasAkhirRPI

_ODIR = obj
_SDIR = src
_HDIR = src

SOURCES = $(wildcard $(_SDIR)/*.cpp)
OBJECTS = $(patsubst $(_SDIR)/%.cpp, $(_ODIR)/%.o, $(SOURCES))
HEADERS = $(wildcard $(_HDIR)/*.hpp)


$(_ODIR)/%.o: $(_SDIR)/%.cpp $(HEADERS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) $(INCLUDE_DIR) -O3 -Wall -c -std=c++11 $< -o $@ 
	@echo 'Finished building: $<'
	@echo ' '

setup:
	mkdir -p $(_ODIR)
	@echo '$(OBJECTS)'


all: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROG) $^ $(OPENCV)
	@echo 'DONEZO'

.PHONY: clean
clean:
	rm -rf $(_ODIR) $(PROG)
