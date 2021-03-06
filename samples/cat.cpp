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

class CatStdIo
    :public BaseConnection
{
public:
    CatStdIo(SimpleEventLoop* loop, int fd, int options =  BEV_OPT_CLOSE_ON_FREE )
         : BaseConnection(loop)
           ,another(NULL)
    {
        take_socket( fd , EV_WRITE |  EV_READ, options ); // BEV_OPT_THREADSAFE causes 'bufferevent_socket_new fails on STDIO fd'
    }

    CatStdIo * another;

    virtual void on_readable()
    {
        if (!another)
        {
            return;
        }
        struct evbuffer *input = bufferevent_get_input(bev);
        bufferevent_write_buffer( another->get_bev(), input);
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

        //1. dump stdin to stdout
        CatStdIo   in(&loop , STDIN_FILENO );
        CatStdIo   out(&loop, STDOUT_FILENO);
        in.another = &out;

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

