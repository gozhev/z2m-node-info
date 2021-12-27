include make/make.mk

$.cxxflags = \
	-Wall \
	-Wpedantic \
	-std=c++17

$(cc_binary)
	name = z2m-node-observer
	srcs = main.cc
$;
