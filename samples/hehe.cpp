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

    HelloClientConnection(SimpleEventLoop* loop, const char* path)
         : BaseConnection(loop)
    {
        connect_generic( path);
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
    
    virtual void post_disconnected()
    {
        event_base_loopexit( get_event_base(), NULL); 
        delete this;
    }
};

int main(int argc, char **argv)
{
    try
	{ 
        SimpleEventLoop loop;
        
        if (2==argc)
        { 
            LOG_DEBUG("We take %s\n", argv[1]);
            HelloClientConnection *hehe =  new HelloClientConnection (&loop, argv[1]);
            (void)hehe; 
        }
        else
        {
            HelloClientConnection *hehe =  new HelloClientConnection (&loop, "127.0.0.1", PORT);
           (void)hehe; 
        }

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

