#
# simple Makefile template :)
#
Target=$(hello_target) $(hehe_target)

hello_target=hello 
hello_src=hello.cpp
hello_objs:=$(patsubst %.cpp,%.o,$(hello_src)) 

# the 'client' part of hello
hehe_target=hehe
hehe_src=hehe.cpp
hehe_objs:=$(patsubst %.cpp,%.o,$(hehe_src)) 


CommMan_objs:=$(patsubst %.cpp,%.o,$(wildcard CommMan/*.cpp))

Objs:= $(hello_objs) $(hehe_objs) $(CommMan_objs)

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


CPPFLAGS=  -DDEBUG -I. -ICommMan
CFLAGS= -ggdb3 -Wall -MMD  -pthread -fPIC
CXXFLAGS= $(CFLAGS)

LDFLAGS= -pthread 
LOADLIBES=
LDLIBS= -levent


##########################################################################

Deps= $(Objs:.o=.d) 

all:$(Target)

-include $(Deps)

$(hello_target): $(hello_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(hehe_target): $(hehe_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@


clean:
	rm -fr $(Objs) $(Target) $(Deps)

run_hello:$(hello_target)
	./$(hello_target)

test_hello:$(hello_target) $(hehe_target)
	./test_hello.sh

