/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).

  And now, it also listens on a unix domain socket.
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 9995;

class HelloServerConnection
    :public BaseConnection
{
public:
    HelloServerConnection(SimpleEventLoop* loop, evutil_socket_t fd, int options =  BEV_OPT_CLOSE_ON_FREE )
         : BaseConnection(loop)
    {
        take_socket(fd, EV_WRITE , options);
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

class HelloWorldListener
    : public BaseListener 
{
public: 
     HelloWorldListener(SimpleEventLoop  * loop) 
        :BaseListener(loop )
    {
    }

    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen)
    {
        HelloServerConnection* conn = new HelloServerConnection( my_app, fd );
        conn->queue_to_send( MESSAGE, strlen(MESSAGE));
    }

};

class QuitSignalHandler
    : public BaseSignalHandler
{
public: 
    QuitSignalHandler(SimpleEventLoop  * loop) 
        : BaseSignalHandler(loop )
    {
    }

    virtual void signal_cb()
    {	
        struct timeval delay = { 1, 0 };

        LOG_DEBUG("Caught an interrupt signal; exiting cleanly in 1 second.\n");
        event_base_loopexit( get_event_base(), &delay);
    }
};

int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif
    try
	{ 
        SimpleEventLoop loop;
        
        //1. listen on PORT
        HelloWorldListener tcp_listener( &loop);
        tcp_listener.start_listen_on_tcp( CString("0.0.0.0:%d", PORT).c_str() );
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        control_c_handler.start_handle_signal( SIGINT );

        //3. also listen on a unix domain socket
        HelloWorldListener un_listener( &loop);
        un_listener.start_listen_on_un( ".haha" );

        //4. the main loop
        loop.run();

        printf("done\n");

	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Exception in main(): %s\n", ex.what());
        return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception in main()\n");
        return 1;
	}
    return 0;
}

