all:
	g++ -std=c++11 -o plot glwin.c shader_utilities.c oglplot.cpp -I. -lX11 -lGL
