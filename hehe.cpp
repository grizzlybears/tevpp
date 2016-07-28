/*
    this 'client part' of the sample 'hello'.

    It behaves like 'socat READLINE tcp:127.0.0.1:9995'.
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const int PORT = 9995;

class HelloClientConnection
    :public BaseConnection
{
public:
    HelloClientConnection(SimpleEventLoop* loop, const char* host, int port, int options =  BEV_OPT_CLOSE_ON_FREE )
         : BaseConnection(loop)
    {
        connect_tcp( host, port);
    }

    virtual void on_readable()
    {
        char buf[1024];
        int n;
        struct evbuffer *input = bufferevent_get_input(bev);
        while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
            fwrite(buf, 1, n, stdout);
        }
    }
    
    virtual void on_conn_event(short events)
    { 
        if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) 
        {
            // we quit.
            event_base_loopexit( get_event_base(), NULL); 
        }
    
        // let base class to the cleanup stuff
        BaseConnection::on_conn_event( events);
    };
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
    try
	{ 
        SimpleEventLoop loop;
        HelloClientConnection*  hehe = new HelloClientConnection(&loop, "127.0.0.1", PORT);
        (void)hehe;

        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        control_c_handler.start_handle_signal( SIGINT );

        // the main loop
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

