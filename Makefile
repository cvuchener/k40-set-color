CXX=g++
CXXFLAGS=-Wall -std=c++11
CXXFLAGS+=-g -O0
CXXFLAGS+=$(shell pkg-config libusb-1.0 --cflags)
LDFLAGS=$(shell pkg-config libusb-1.0 --libs)
#CXXFLAGS+=-DREQUEST_50

TARGET=k40-set-color
SRC=main.cpp

all: $(TARGET)

$(TARGET): $(SRC:.cpp=.o) 
	$(CXX) $^ $(LDFLAGS) -o $@

%.deps: %.cpp
	$(CXX) -M $(CXXFLAGS) $< > $@

-include $(SRC:.cpp=.deps)

%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

clean:
	rm -f $(SRC:.cpp=.o) $(SRC:.cpp=.deps)

