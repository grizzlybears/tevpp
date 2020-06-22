/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).

  And now, it also listens on a unix domain socket.
*/
#include <stdio.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const int PORT = 9995;
static const char US_DOMAIN[] = ".haha";

class EchoToAllApp
    : public SimpleEventLoop 
{
public:  
    EchoToAllApp()
        :pipe_man(this)
    {
    }

    OuterPipeMan pipe_man;

};


class EchoToAllServerConnection
    :public OuterPipe
{
public:
    EchoToAllServerConnection(SimpleEventLoop* loop, evutil_socket_t fd )
         : OuterPipe(loop)
    {
        take_socket(fd);
        conn_at = time(NULL);
        peer_info.get_peer_info(fd);

    } 
     
    time_t   conn_at;
    AddrInfo peer_info;

    virtual  CString dump_2_str() const
    {
        CString s;

        int fd = bufferevent_getfd(bev);
        CString conn_time(conn_at);
            
        if (is_managed())
        {
            s.Format("pipe #%lu, fd #%d, peer %s:%d, conn at %s"
                , get_id()
                , fd
                ,  peer_info.peer_ipstr,  peer_info.peer_port
                , conn_time.c_str()
                );
        }
        else
        { 
            // 未纳入pipe_man
            s.Format("fd #%d, peer %s:%d, conn at %s"
                , fd
                ,  peer_info.peer_ipstr,  peer_info.peer_port
                , conn_time.c_str()
                );

        }

        return s;
    }


    virtual OuterPipeMan * get_pipe_man()
    {
        EchoToAllApp *app = (EchoToAllApp*)  get_app();
        return & app->pipe_man;
    }


    virtual void on_readable()
    { 
        struct evbuffer *input = bufferevent_get_input(get_bev());
        size_t buffer_len = evbuffer_get_length( input );

        std::vector<char> buf;
        buf.resize(buffer_len+1);
    
        evbuffer_remove(input , &buf[0], buffer_len );
        buf[buffer_len] = 0;

        CString out_msg;
        out_msg.Format(
                "From '%s': %s\n"
                , dump_2_str().c_str()
                , &buf[0]
                );

        echo_2_all(out_msg);

    }

    void echo_2_all(const CString& s)
    { 
        OuterPipeMan* man = get_pipe_man();

        OuterPipeMan::iterator it;
        for (it = man->begin(); it != man->end() ; it++)
        { 
            OuterPipe* the_pipe = it->second; 

            the_pipe->send_str(s);
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
        EchoToAllServerConnection* conn = new EchoToAllServerConnection( my_app, fd ); 

        EchoToAllApp *app = (EchoToAllApp*)  get_app();
        int i =  app->pipe_man.register_connection( conn);

        if (i)
        {
            LOG_ERROR("Failed in register new incoming conn.");
            free(conn); 
        }
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

void print_hint()
{
    printf("To make some clients:\n");
    printf("\tsocat READLINE tcp4:127.0.0.1:%d\n",PORT);
    printf("\tsocat READLINE unix:%s\n", US_DOMAIN);
    printf("\tctrl-c to exit\n");

}

int main(int argc, char **argv)
{
    try
	{ 
        evthread_use_pthreads();

        EchoToAllApp loop;
        
        //1. listen on PORT
        HelloWorldListener tcp_listener( &loop);
        tcp_listener.start_listen_on_tcp( CString("0.0.0.0:%d", PORT).c_str() );
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        control_c_handler.start_handle_signal( SIGINT );

        //3. also listen on a unix domain socket
        HelloWorldListener un_listener( &loop);
        un_listener.start_listen_on_un(  US_DOMAIN  );

        //4. the main loop
        print_hint();
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

