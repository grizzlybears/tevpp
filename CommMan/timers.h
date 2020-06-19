#ifndef  __TIMERS_H__
#define  __TIMERS_H__

#include <event2/event.h>
#include <event2/util.h>

#include "utils.h"
#include "EventLoop.h"

class TimerHandler
{
public: 
     TimerHandler(SimpleEventLoop  * the_loop)
        :loop(the_loop)
         ,timer_event(NULL)
    {
    }

    virtual ~TimerHandler()
    {
        stop_timer();
    }

    static void trampoline(evutil_socket_t sig, short events, void *user_data);
    virtual void start_timer(const struct timeval& interval);
    virtual void stop_timer();

    virtual void timer_cb() = 0;
    

    struct event_base * get_event_base()
    {
        return loop->get_event_base();
    }
    
    SimpleEventLoop  * get_app()
    {
        return loop;
    }

protected: 
    SimpleEventLoop  *loop;  // just ref, dont touch its life cycle.
	struct event     *timer_event;

};



#endif


