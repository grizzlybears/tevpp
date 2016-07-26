#
# simple Makefile template :)
#
Target=CommMan
Objs=$(patsubst %.cpp,%.o,$(wildcard *.cpp))




#      以下摘自 `info make`
#
#Compiling C programs
# `N.o' is made automatically from `N.c' with a recipe of the form
#  `$(CC) $(CPPFLAGS) $(CFLAGS) -c'.
#
#Compiling C++ programs
#  `N.o' is made automatically from `N.cc', `N.cpp', or `N.C' with a recipe of the form 
#  `$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c'.  
#  We encourage you to use the suffix `.cc' for C++ source files insteadof `.C'.
#
#Linking a single object file
#  `N' is made automatically from `N.o' by running the linker
#  (usually called `ld') via the C compiler.  The precise recipe used
#  is `$(CC) $(LDFLAGS) N.o $(LOADLIBES) $(LDLIBS)'.

CC = g++
CXX= g++

CPPFLAGS=  -DDEBUG 
CFLAGS= -ggdb3 -Wall -MMD  -pthread -fPIC
CXXFLAGS= $(CFLAGS)

LDFLAGS= -pthread 
LOADLIBES=
LDLIBS= -levent


##########################################################################

Deps= $(Objs:.o=.d) 

all:$(Target)

-include $(Deps)

$(Target): $(Objs)
	$(CC) $(LDFLAGS) $(Objs) $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -fr $(Objs) $(Target) $(Deps)

test:all
	./$(Target)


