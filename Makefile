include make/make.mk

$.cxxflags += \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Werror \
	#

$.cxxflags += \
	-std=c++17 \
	-pthread \
	#

$.cppflags += \
	-D BOOST_ASIO_SEPARATE_COMPILATION \
	-D BOOST_BEAST_SEPARATE_COMPILATION \
	-D BOOST_BEAST_USE_STD_STRING_VIEW \
	#

$.ldflags += \
	-pthread \
	#

$.ldlibs += \
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
