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
    HelloClientConnection(SimpleEventLoop* loop, const char* host, int port )
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
            // we just quit.
            event_base_loopexit( get_event_base(), NULL); 
        }
        // dont' let base class 'delete this'
    };
};

int main(int argc, char **argv)
{
    try
	{ 
        SimpleEventLoop loop;
        HelloClientConnection  hehe(&loop, "127.0.0.1", PORT);

        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        // the main loop
        loop.run();
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

