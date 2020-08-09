SRC_DIR ?= $(PWD)

COMMON_FLAGS = -Wall -Wextra
CXXFLAGS += $(COMMON_FLAGS)
CPPFLAGS += -I$(SRC_DIR)

objects = CartridgeData.o Cartridge.o CPU.o head.o
name = header

$(name) : $(objects)
		@echo Linking $@
		$(CXX) -o $@ $(CXXFLAGS) $^

%.o : %.cpp
		@echo Compiling $*.cpp
		$(CXX) -c $< $(CPPFLAGS) -o $@

.PHONY: clean
clean:
		rm -f $(name) $(objects)