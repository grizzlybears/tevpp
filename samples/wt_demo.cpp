/*
    demo of 'worker_thead' and timer 

*/
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"
#include "timers.h"
#include "inner_pipe.h"

class ClientConnection
    :public BaseConnection
{
public:
    ClientConnection(SimpleEventLoop* loop, const char* host, int port )
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
    
    virtual void post_disconnected()
    {
        event_base_loopexit( get_event_base(), NULL); 
        delete this;
    }
};


class DemoApp
    : public SimpleEventLoop 
{
public:
    MsgSwitch * msg_switch;           // doesn't maintain life-cycle. 
    ClientConnection * client_conn;   // doesn't maintain life-cycle. 
    
    int     seq;
    
    DemoApp()
    { 
        msg_switch  = NULL;
        client_conn = NULL;
        seq = 0;
    }
};

    
class DemoJob: public JobMessage
{
public: 
    DemoJob( DemoApp* the_app)
        :app(the_app)
    {
    }

    virtual ~DemoJob()
    {
    }

    DemoApp * app;

    CString msg_2_server;

    virtual void do_in_main_thread()
    {
        debug_printf("In main, we got %s\n\n\n", msg_2_server.c_str());

        if (app->client_conn)
        {
            app->client_conn->send_str(msg_2_server);
        }

        // msg 自己负责释放
        delete this;
    }

    virtual void do_in_worker_thread()
    {
        debug_printf("let's do sth in worker\n"); 
        
        msg_2_server.Format("#%d msg from worker thread\n", app->seq );
        app->seq++;
        
        // 接力到主线程
        app->msg_switch->queue_to_main_thread( this);
        // msg 自己负责释放
        // delete this;
    }

    virtual CString dump_2_str() const
    {
        return "DemoJobMessage";
    }
};

class DemoTimer
     :public TimerHandler
{
public:
    DemoTimer(SimpleEventLoop* loop )
         : TimerHandler(loop)
    {  
        
        struct timeval ten_sec = { 3 , 0 }; // heartbeat every 3 secs
        // We're going to set up a repeating timer
        start_timer(ten_sec);
    }
    
    virtual void  timer_cb()
    { 
        debug_printf("timer in main thread\n");
        DemoApp * app = (DemoApp*)get_app();

        DemoJob*  job = new DemoJob(app);
        app->msg_switch->queue_to_worker_thread(job );
    }
 
};


int main(int argc, char *argv[])
{
    try
	{ 
        DemoApp loop;
        debug_printf("demo worker thread\n");
        
        // create worker thead, who communicates with main thread via msg_switch
        MsgSwitch msg_switch(&loop);
        loop.msg_switch = &msg_switch; 
        
        if (3 == argc)
        {
            loop.client_conn = new ClientConnection(&loop, argv[1], atoi(argv[2]));
        }
        
        int i = msg_switch.create_inner_pipe();
        if (i)
        {
            return 1;
        }

        msg_switch.start_worker();

        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        //3. create a timer 
        DemoTimer timer(&loop);

        //4.  the main loop
        loop.run();
        
        LOG_INFO("Let's clean up.\n");
        msg_switch.stop_worker();

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

