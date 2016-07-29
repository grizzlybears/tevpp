/*
  The 'cat' util writen in libevent style.
*/
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "signals.h"
#include "connections.h"

class CatStdIn
    :public BaseConnection
{
public:
    CatStdIn(SimpleEventLoop* loop, int options =  BEV_OPT_CLOSE_ON_FREE )
         : BaseConnection(loop)
    {
        take_socket( STDIN_FILENO );
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
        event_base_loopexit( get_event_base(), NULL); 
    }
};

int main(int argc, char **argv)
{
    try
	{ 
        SimpleEventLoop loop;
        CatStdIn  miaomiao(&loop);

        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        control_c_handler.start_handle_signal( SIGINT );

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

