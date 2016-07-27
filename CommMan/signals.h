#ifndef  __SIGNALS_H__
#define  __SIGNALS_H__

#include <event2/event.h>
#include <event2/util.h>

#include "utils.h"
#include "EventLoop.h"

class BaseSignalHandler
{
public: 
     BaseSignalHandler(SimpleEventLoop  * loop)
        :evbase(loop)
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
        return evbase->get_event_base();
    }

protected: 
    SimpleEventLoop  *evbase;  // just ref, dont touch its life cycle.
	struct event     *signal_event;

};


#endif
