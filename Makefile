#
# simple Makefile template :)
#
Target=$(hello_target) $(hehe_target) $(cat_target) $(wt_target) $(echo_target) $(udp_target) $(wt2_target) $(http_server_target)

sample_dir=samples

hello_target=hello 
hello_src=$(sample_dir)/hello.cpp
hello_objs:=$(patsubst %.cpp,%.o,$(hello_src)) 

# the 'client' part of hello
hehe_target=hehe
hehe_src=$(sample_dir)/hehe.cpp
hehe_objs:=$(patsubst %.cpp,%.o,$(hehe_src)) 

# 'cat' in libevent style
cat_target=evcat
cat_src=$(sample_dir)/cat.cpp
cat_objs:=$(patsubst %.cpp,%.o,$(cat_src)) 

# demo of 'worker thread'
wt_target=wt_demo
wt_src=$(sample_dir)/wt_demo.cpp
wt_objs:=$(patsubst %.cpp,%.o,$(wt_src)) 

# demo of 'managed outer pipes'
echo_target=echo2all
echo_src=$(sample_dir)/echo_to_all.cpp
echo_objs:=$(patsubst %.cpp,%.o,$(echo_src)) 

# demo of UDP binder
udp_target=udp_echo
udp_src=$(sample_dir)/udp_echo.cpp
udp_objs:=$(patsubst %.cpp,%.o,$(udp_src)) 

# demo of 'worker thread pool'
wt2_target=wt_demo2
wt2_src=$(sample_dir)/wt_demo2.cpp
wt2_objs:=$(patsubst %.cpp,%.o,$(wt2_src)) 

# demo of 'simple http server'
http_server_target=http_server_demo
http_server_src=$(sample_dir)/http_server.cpp
http_server_objs:=$(patsubst %.cpp,%.o,$(http_server_src)) 



CommMan_objs:=$(patsubst %.cpp,%.o,$(wildcard CommMan/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard CommMan/json/*.cpp))

Objs:= $(hello_objs) $(hehe_objs) $(CommMan_objs) $(cat_objs) $(wt_objs) $(echo_objs) $(udp_objs) $(wt2_objs) $(http_server_objs)

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

$(wt2_target): $(wt2_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(echo_target): $(echo_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(udp_target): $(udp_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

$(http_server_target): $(http_server_objs) $(CommMan_objs)
	$(CC) $^ $(LDFLAGS)  $(LOADLIBES) $(LDLIBS) -o $@

YCM:
	make clean
	make > build.log
	compiledb -p build.log

clean:
	rm -fr $(Objs) $(Target) $(Deps)

run_hello:$(hello_target)
	./$(hello_target)

test_hello:$(hello_target) $(hehe_target)
	./test_hello.sh

test_cat:$(cat_target)
	cat $(sample_dir)/cat.cpp | ./$(cat_target)

test_wt:$(wt_target) 
	./$(wt_target) 

test_echo:$(echo_target)
	./$(echo_target)

test_udp:$(udp_target)
	./$(udp_target)

test_wt2:$(wt2_target) 
	./$(wt2_target) 

test_http_server:$(http_server_target) 
	./$(http_server_target)  &
	@echo
	curl http://127.0.0.1:9090/
	@echo
	curl http://127.0.0.1:9090/foo
	@echo
	curl http://127.0.0.1:9090/bar
	@echo
	@echo "Let's quit"
	pkill -2 -f  $(http_server_target)

