Thinking libevent in C++
============================

Brief
----

'libevent' does help communication programming a lot! 
While there are still too many '*callbacks*' and '*void* opaque*' making things not easy.

Can we just think communication stuff in OO?
For example, when we need to connect to somewhere via tcp, we need something like this:

```c++
class BaseConnection
{
public:
	void connect_tcp(const char *hostname, int port); 
	
	// we dont't want callback functions with 'void* opaque', C++ virual function plays this role.
	virtual void on_conn_event(short what);  // when get connected or disconnected, this 'CB' will be called.
	virtual void on_readable();              // when income data available, this 'CB' will be called.
	
    virtual int queue_to_send( const void *data, size_t size);  // as name implies, this the 'aync write' helper.
};
```

When  we are on 'server side' ,  we need:
```c++
class BaseListener
{
public:
    void start_listen_on_tcp(const char* addr);
    
    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen) = 0;  // implement this 'CB' to hadle incoming connections
};
```

So, a tiny tcp server program will be as simple as:
```c++
#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 9995;

class HelloServerConnection  // our 'connection', when connected it says a hello msg to peer then disconnect.
    :public BaseConnection
{
public:
    HelloServerConnection(SimpleEventLoop* loop, evutil_socket_t fd )
         : BaseConnection(loop) {
        take_socket(fd, EV_WRITE );
    }

    virtual void on_writable()
    {
        struct evbuffer *output = bufferevent_get_output(bev);
        if (evbuffer_get_length(output) == 0) {
            printf("flushed answer\n");
            delete this;
        }
    }
};

class HelloWorldListener   // our real 'listener', creates our 'connection' instances to handle incoming connecting request.
    : public BaseListener 
{
public: 
     HelloWorldListener(SimpleEventLoop  * loop) 
        :BaseListener(loop ) {
    }

    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen) {
        HelloServerConnection* conn = new HelloServerConnection( my_app, fd );
        conn->queue_to_send( MESSAGE, strlen(MESSAGE));
    }
};

int main(int argc, char **argv){
    try	{ 
        SimpleEventLoop loop;
        //1. TCP listen on PORT
        HelloWorldListener tcp_listener( &loop);
        tcp_listener.start_listen_on_tcp( CString("0.0.0.0:%d", PORT).c_str() );
     
        //2. also we quit gracefully when ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        //3. the main loop
        loop.run();
    
        printf("done\n");
    }
    catch (std::exception& ex) {
    	LOG_ERROR("Exception in main(): %s\n", ex.what());
        return 1;
    }
    
    return 0;
}
```

Sure, in real world, we need much more.
The 'BaseConnection' should treat   'unix domain socket',  'serial port', 'generic file' , 'stdio' in similar way, and there should also be 'udp'  wrapper,  'timer', 'signal handler',  'http server wrapper', and so on.
What is more important, we need a 'connection manager' when we build real (server ) applications.  And since we are on async style, we can not do any block operation (such as DB accessing) in any  'CB', we need worker-thread / thread-pool, and should be able to 'marshal' jobs between main-thread and worker-thread freely.
When we have  all stuff mentioned above in hand, we can view the  framework of a brand new back-end service.

Build dependences
-----------------
    (On Fedora) yum install gcc-c++ libstdc++-devel libevent-devel socat.

Build and run
-------------
    $ make             #build all samples
    
    $ make test_hello  #run the 'libevent hello-world sample' C++ ported.
    
    $ make test_cat    #run the 'cat' sample, which implement 'cat' in libevent style.
    
    $ make test_wt     #run the 'wt_demo' sample, which does some stuff in worker thread.
    
    $ make test_wt2    #run the 'wt_demo2' sample, which does some stuff in worker thread pool.
    
    $ make test_echo   #run the 'echo to all' sample  -- a chat server echoes any income msg to all cliets.
    
    $ make test_http_server  #run sample of httpserver wrapper. 



