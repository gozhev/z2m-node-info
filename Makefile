include make/make.mk

$.build_dir = build/
$.cxx = g++
$.cxxflags = \
	-Wall \
	-Wpedantic \
	-std=c++17

$(cc_binary)
	name = z2m-node-observer
	srcs = main.cc
$;
