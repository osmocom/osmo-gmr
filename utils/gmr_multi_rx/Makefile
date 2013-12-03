CFLAGS = -Wall -Wextra -Wno-unused -O2

TARGET ?= usrp

ifeq ($(TARGET),fcd)
PKGLIBS += gnuradio-fcd
CFLAGS += -DHAVE_FCD
endif
ifeq ($(TARGET),uhd)
PKGLIBS += gnuradio-uhd uhd
CFLAGS += -DHAVE_UHD
endif
ifeq ($(TARGET),usrp)
PKGLIBS += gnuradio-usrp
endif

CFLAGS += $(shell pkg-config --cflags $(PKGLIBS))
LDLIBS = $(shell pkg-config --libs $(PKGLIBS)) \
	-lboost_program_options \
	-lboost_system \
	-lboost_thread -lpthread \

all: gmr_multi_rx

gmr_multi_rx: gmr_multi_rx.o
	$(CXX) -o $@ $< $(LDFLAGS) $(LDLIBS)

.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) gmr_multi_rx *.o
