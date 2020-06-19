#
# simple Makefile template :)
#
Target=$(hello_target) $(hehe_target) $(cat_target) $(wt_target)

hello_target=hello 
hello_src=hello.cpp
hello_objs:=$(patsubst %.cpp,%.o,$(hello_src)) 

# the 'client' part of hello
hehe_target=hehe
hehe_src=hehe.cpp
hehe_objs:=$(patsubst %.cpp,%.o,$(hehe_src)) 

# 'cat' in libevent style
cat_target=evcat
cat_src=cat.cpp
cat_objs:=$(patsubst %.cpp,%.o,$(cat_src)) 

# demo of 'worker thread'
wt_target=wt_demo
wt_src=wt_demo.cpp
wt_objs:=$(patsubst %.cpp,%.o,$(wt_src)) 


CommMan_objs:=$(patsubst %.cpp,%.o,$(wildcard CommMan/*.cpp))

Objs:= $(hello_objs) $(hehe_objs) $(CommMan_objs) $(cat_objs) $(wt_objs)

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
CFLAGS= -ggdb3 -Wall -MMD  -pthread -fPIC -std=c++11
CXXFLAGS= $(CFLAGS)

LDFLAGS= -pthread 
LOADLIBES=
LDLIBS= -levent -levent_pthreads


##########################################################################

Deps= $(Objs:.o=.d) 

all:$(Target)

-include $(Deps)

$(hello_target): $(hello_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(hehe_target): $(hehe_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(cat_target): $(cat_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(wt_target): $(wt_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@


clean:
	rm -fr $(Objs) $(Target) $(Deps)

run_hello:$(hello_target)
	./$(hello_target)

test_hello:$(hello_target) $(hehe_target)
	./test_hello.sh

test_cat:$(cat_target)
	cat cat.cpp | ./$(cat_target)

test_wt:$(wt_target) 
	./$(wt_target) 

