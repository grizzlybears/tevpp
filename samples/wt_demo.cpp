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

class DemoApp
    : public SimpleEventLoop 
{
public:
    MsgSwitch * msg_switch;  // doesn't maintain life-cycle.
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
    virtual void do_in_main_thread()
    {
        debug_printf("continue to do sthin main\n\n\n");
        // msg 自己负责释放
        delete this;
    }

    virtual void do_in_worker_thread()
    {
        debug_printf("let's do sth in worker\n");
        
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



int main(int argc, char **argv)
{
    try
	{ 
        DemoApp loop;
        debug_printf("demo worker thread\n");
        
        // create worker thead, who communicates with main thread via msg_switch
        MsgSwitch msg_switch(&loop);
        loop.msg_switch = &msg_switch;
        int i = msg_switch.create_inner_pipe();
        if (i)
        {
            return 1;
        }
        
        //DemoJob*  job = new DemoJob(&loop);
        //msg_switch.queue_to_worker_thread(job );


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

