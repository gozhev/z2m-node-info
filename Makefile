include make/make.mk

$.cxxflags = \
	-Wall \
	-Wpedantic \
	-std=c++17 \
	-D BOOST_ASIO_SEPARATE_COMPILATION \
	-D BOOST_BEAST_SEPARATE_COMPILATION \
	-D BOOST_BEAST_USE_STD_STRING_VIEW \

$.cxxflags += \
	-pthread
$.ldflags += \
	-pthread

$(cc_binary)
	name = z2m-node-info
	srcs = \
		main.cc \
		boost.cc
$;

