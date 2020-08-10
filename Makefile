SRC_DIR ?= $(PWD)

COMMON_FLAGS = -Wall -Wextra
CXXFLAGS += $(COMMON_FLAGS)
CPPFLAGS += -I$(SRC_DIR)

objects = CartridgeData.o Cartridge.o CPU.o GPU.o head.o
name = header

$(name) : $(objects)
		@echo Linking $@
		$(CXX) -o $@ $(CXXFLAGS) $^ libminifb.a -fobjc-link-runtime -framework CoreGraphics -framework AppKit
		
%.o : %.cpp
		@echo Compiling $*.cpp
		$(CXX) -c -std=c++11 $< $(CPPFLAGS) -o $@

.PHONY: clean
clean:
		rm -f $(name) $(objects)

