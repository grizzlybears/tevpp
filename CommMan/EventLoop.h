#ifndef  __EVENT_LOOP_H__
#define  __EVENT_LOOP_H__

#include <event2/event.h>
#include <event2/util.h>
#include <event2/dns.h>
#include <event2/thread.h>

#include "utils.h"
class SimpleEventLoop
{
public:
    SimpleEventLoop():dns_base(NULL)
    {
        evthread_use_pthreads();
        
        base = event_base_new();
        if (!base) {
            throw SimpleException("Could not initialize event_base.\n");
        }
    }

    virtual ~SimpleEventLoop()
    {
        if (dns_base)
        { 
            evdns_base_free( dns_base, 1);
            dns_base = NULL;
        }

        if (base)
        {
            event_base_free(base);
            base = NULL;
        }
    }
	
    operator struct event_base *()
    {
        return base;
    }

    struct event_base * get_event_base()
    {
        return base;
    }
    
    struct evdns_base * get_dns_base()
    {
        if (dns_base)
        {
            return dns_base;
        }
        dns_base = evdns_base_new(base, 1);
        return dns_base;
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
    struct evdns_base *dns_base;
};

#endif

