SRC_DIR ?= $(PWD)

COMMON_FLAGS = -Wall -Wextra
CXXFLAGS += $(COMMON_FLAGS)
CPPFLAGS += -I$(SRC_DIR)

objects = CartridgeData.o Cartridge.o Util.o CPU.o GPU.o APU.o Mmunit.o machine.o main.o
name = main

$(name) : $(objects)
		@echo Linking $@
		$(CXX) -o $@ $(CXXFLAGS) $^ libminifb.a -fobjc-link-runtime -framework CoreGraphics -framework AppKit
		
%.o : %.cpp
		@echo Compiling $*.cpp
		$(CXX) -c -std=c++11 $< $(CPPFLAGS) -o $@

.PHONY: clean
clean:
		rm -f $(name) $(objects)

