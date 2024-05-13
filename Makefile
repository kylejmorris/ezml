CXX=g++
CXXFLAGS=-std=c++17 -I/opt/homebrew/Cellar/libssh/0.10.6/include
LDLIBS=-L/opt/homebrew/Cellar/libssh/0.10.6/lib -lssh 

all: ezml

ezml: ezml.cpp
	$(CXX) $(CXXFLAGS) -o ezml ezml.cpp $(LDLIBS)