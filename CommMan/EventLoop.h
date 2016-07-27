#ifndef  __EVENT_LOOP_H__
#define  __EVENT_LOOP_H__

#include <event2/event.h>
#include <event2/util.h>

#include "utils.h"
class SimpleEventLoop
{
public:
    SimpleEventLoop()
    {
        base = event_base_new();
        if (!base) {

            throw SimpleException("Could not initialize event_base.\n");
        }
    }

    virtual ~SimpleEventLoop()
    {
        if (base)
            event_base_free(base);
    }
	
    operator struct event_base *()
    {
        return base;
    }

    struct event_base * get_event_base()
    {
        return base;
    }

    virtual void run()
    { 
        if (!base) {
            throw SimpleException("Could not 'run' with no 'event_base' available.\n");
        }
        event_base_dispatch(base);
    }
protected:
	struct event_base *base;

};



#endif

