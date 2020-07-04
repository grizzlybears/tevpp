/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).

  And now, it also listens on a unix domain socket.
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "json/json.h"  // this header must be 1st one, it is somehow sick.
#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"
#include "jmsg_connection.h"

static const int PORT   = 9995;
static const int PORT_J = 9996;
static const char US_DOMAIN[] = ".haha";

#define PIPE_TYPE_PLAIN  (0)
#define PIPE_TYPE_JSON   (1)

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
        pipe_type = PIPE_TYPE_PLAIN;

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
            s.Format("pipe #%lu, fd #%d, %s, conn at %s"
                , get_id()
                , fd
                ,  peer_info.to_str().c_str() 
                , conn_time.c_str()
                );
        }
        else
        { 
            // 未纳入pipe_man
            s.Format("fd #%d, %s, conn at %s"
                , fd
                ,  peer_info.to_str().c_str() 
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

            if (the_pipe->pipe_type != pipe_type)
            {
                continue;
            }

            the_pipe->send_str(s);
        }
    }

};


class EchoToAllServerJMsgConnection
    :public  JMsgConnection
{
public:
    typedef   JMsgConnection MyBase;
    EchoToAllServerJMsgConnection(SimpleEventLoop* loop, evutil_socket_t fd )
         :MyBase(loop,fd)
    {
        conn_at = time(NULL);
        peer_info.get_peer_info(fd);
        pipe_type = PIPE_TYPE_JSON;
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
            s.Format("pipe(J) #%lu, fd #%d, %s, conn at %s"
                , get_id()
                , fd
                ,  peer_info.to_str().c_str() 
                , conn_time.c_str()
                );
        }
        else
        { 
            // 未纳入pipe_man
            s.Format("fd(J) #%d, %s, conn at %s"
                , fd
                ,  peer_info.to_str().c_str() 
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


    virtual int really_process_msg(BaseJMessage* msg)
    {
        Json::Value ack = msg->jdata;
        ack["Source"] =  dump_2_str();

        echo_2_all(msg->sequence , ack);
     
        return 0;
    }


    void echo_2_all(int32_t seq, const Json::Value& jdata )
    { 
        OuterPipeMan* man = get_pipe_man();

        OuterPipeMan::iterator it;
        for (it = man->begin(); it != man->end() ; it++)
        { 
            OuterPipe* the_pipe = it->second;  
            if (the_pipe->pipe_type != pipe_type)
            {
                continue;
            } 
            
            EchoToAllServerJMsgConnection * j= checked_cast(the_pipe,(EchoToAllServerJMsgConnection *) NULL );
            j->send_msg( seq, jdata);
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

class HelloJMsgListener
    : public BaseListener 
{
public: 
     HelloJMsgListener(SimpleEventLoop  * loop) 
        :BaseListener(loop )
    {
    }

    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen)
    {
        EchoToAllServerJMsgConnection* conn = new EchoToAllServerJMsgConnection( my_app, fd ); 

        EchoToAllApp *app = (EchoToAllApp*)  get_app();
        int i =  app->pipe_man.register_connection( conn);

        if (i)
        {
            LOG_ERROR("Failed in register new incoming conn.");
            free(conn); 
        }
    }

};

void print_hint()
{
    printf("To make some clients:\n");
    printf("\tsocat READLINE tcp4:127.0.0.1:%d\n",PORT);
    printf("\t./wt_demo 127.0.0.1 %d\n",PORT);
    printf("\tsocat READLINE unix:%s\n", US_DOMAIN);
    printf("\t./wt_demo -j 127.0.0.1 %d\n",PORT_J);
    printf("\tctrl-c to exit\n");

}

void spawn_bg_wt_demo_as_my_client();

int main(int argc, char **argv)
{
    try
	{ 

        EchoToAllApp loop;
        
        //1. listen on PORT
        HelloWorldListener tcp_listener( &loop);
        tcp_listener.start_listen_on_tcp( CString("0.0.0.0:%d", PORT).c_str() );
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        //3. also listen on a unix domain socket
        HelloWorldListener un_listener( &loop);
        un_listener.start_listen_on_un(  US_DOMAIN  );

        //4. listen on PORT_J for jmsg clients
        HelloJMsgListener tcp_listener2( &loop);
        tcp_listener2.start_listen_on_tcp( CString("0.0.0.0:%d", PORT_J).c_str() );
 

        //5. make a client of plain text protocal
        spawn_bg_wt_demo_as_my_client();

        // the main loop
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

void spawn_bg_wt_demo_as_my_client()
{
    pid_t child = fork();
    if (child > 0 )
    {
        // I am parent.
        int status;
        waitpid( child, & status, 0);
        return ;
    }
    else if (child < 0)
    {
        SIMPLE_LOG_LIBC_ERROR("First time spawn"  , errno);
        return ;
    }


    // level 1 child

    pid_t child2 = fork();
    if (child2 > 0 )
    {
        // I am parent.
        exit(0);
        return ;
    }
    else if (child < 0)
    {
        SIMPLE_LOG_LIBC_ERROR("2nd time  spawn"  , errno);
        return ;
    }

    // level 2 child
    setsid();
    sleep(1);
    
    execlp("./wt_demo"
            , "./wt_demo", "127.0.0.1", CString(PORT).c_str()
            , NULL
            );

    SIMPLE_LOG_LIBC_ERROR( "exec"  , errno);
    exit(-1);
}

