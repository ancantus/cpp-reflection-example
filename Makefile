CXX := /opt/gcc-trunk/bin/g++
CXXFLAGS ?= -std=c++26 -freflection

TARGETS := enum pack
ALL_TARGETS := $(TARGETS) id_of

.PHONY: all clean

all: $(TARGETS)

enum: enum.cpp utils.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

pack: pack_test.cpp leyline.hpp leyline_impl.hpp print.hpp utils.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

id_of: id_of.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(ALL_TARGETS)
