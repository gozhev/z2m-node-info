include make/make.mk

$(cc_binary)
	name = z2m-node-observer
	srcs = main.cc
	cxx = g++
	cxxflags = \
		-Wall \
		-Wpedantic \
		-std=c++17
$;

$(cc_binary)
	name = z2m-node-test
	srcs = main.cc
	cxx = g++
	cxxflags = \
		-std=c++17
$;
