# CXX := /opt/gcc-trunk/bin/g++
CXXFLAGS ?= -std=c++26 -freflection

TARGETS := pack
.PHONY: all clean

all: $(TARGETS)

pack: pack_test.cpp leyline.hpp leyline_impl.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGETS) enum id_of
