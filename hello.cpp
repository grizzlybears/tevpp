/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"

static const char MESSAGE[] = "Hello, World!\n";

static const int PORT = 9995;

static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);

class RawHelloWorldListener
    : public BaseTcpListener 
{
public: 
     RawHelloWorldListener(SimpleEventLoop  * loop) 
        :BaseTcpListener(loop )
    {
    }

    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen)
    {
        struct event_base *base =  get_event_base();
        struct bufferevent *bev;

        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        if (!bev) {
            fprintf(stderr, "Error constructing bufferevent!");
            event_base_loopbreak(base);
            return;
        }
        bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL);
        bufferevent_enable(bev, EV_WRITE);
        bufferevent_disable(bev, EV_READ);

        bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
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
        RawHelloWorldListener hehe( &loop);
        hehe.start_listen_on_addr2( CString("0.0.0.0:%d", PORT).c_str() );
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        control_c_handler.start_handle_signal( SIGINT );

        //3. the main loop
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

static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		printf("flushed answer\n");
		bufferevent_free(bev);
	}
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
		    strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}


