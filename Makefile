CXX=g++
CXXFLAGS=-Wall -std=c++11

AR=ar
ARFLAGS=rs

DEPS=./deps/shader-utilities
INCLUDE=-I./include -I$(DEPS)

all:
	g++ -std=c++11 -o plot glwin.c shader_utilities.c oglplot.cpp -I. -lX11 -lGL

plotdemo: demo/plotdemo.o liboglplot.a
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -loglplot -lX11 -lGL

demo/plotdemo.o: demo/plotdemo.cpp include/oglplot.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE)
	
liboglplot.a: src/oglplot.o $(DEPS)/shader_utilities.o
	$(AR) $(ARFLAGS) $@ $^

src/oglplot.o: src/oglplot.cpp include/oglplot.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE)

$(DEPS)/shader_utilities.o:
	$(MAKE) -C $(DEPS)
