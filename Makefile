include make/make.mk

$.cxxflags = \
	-Wall \
	-Wpedantic \
	-std=c++17 \
	-D BOOST_ASIO_SEPARATE_COMPILATION \
	-D BOOST_BEAST_SEPARATE_COMPILATION \
	-D BOOST_BEAST_USE_STD_STRING_VIEW \
	-pthread \
	#

$.ldflags = \
	-pthread \
	#

$.ldlibs = \
	-lpaho-mqttpp3 \
	-lpaho-mqtt3as \
	-lboost_program_options \
	#

$(cc_binary)
	name = z2m-node-info
	srcs = \
		main.cc \
		boost.cc \
		#
$;
