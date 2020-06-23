#ifndef  __SIGNALS_H__
#define  __SIGNALS_H__

#include <signal.h>
#include <event2/event.h>
#include <event2/util.h>

#include "utils.h"
#include "EventLoop.h"

class BaseSignalHandler
{
public: 
     BaseSignalHandler(SimpleEventLoop  * loop)
        :my_app(loop)
         ,signal_event(NULL)
    {
    }

    virtual ~BaseSignalHandler()
    {
        if (signal_event)
        {
            event_free(signal_event);
            signal_event = NULL;
        }
    }

    static void trampoline(evutil_socket_t sig, short events, void *user_data);
    virtual void start_handle_signal( int sig_num);

    virtual void signal_cb() = 0;
    

    struct event_base * get_event_base()
    {
        return my_app->get_event_base();
    }

protected: 
    SimpleEventLoop  *my_app;  // just ref, dont touch its life cycle.
	struct event     *signal_event;

};

class QuitSignalHandler
    : public BaseSignalHandler
{
public: 
    QuitSignalHandler(SimpleEventLoop  * loop,int sig = SIGINT ) 
        : BaseSignalHandler(loop )
    {
        start_handle_signal(sig );
    }

    virtual void signal_cb()
    {	
        struct timeval delay = { 1, 0 };

        LOG_DEBUG("Caught an interrupt signal; exiting cleanly in 1 second.\n");
        event_base_loopexit( get_event_base(), &delay);
    }
};


#endif
