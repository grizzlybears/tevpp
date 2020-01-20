#include "timers.h" 


void  TimerHandler::trampoline(evutil_socket_t sig, short events, void *user_data)
{
    TimerHandler* h = (TimerHandler*)user_data;
    h->timer_cb();

}

void  TimerHandler::start_timer(const struct timeval& interval)
{ 
    if (timer_event)
    {
        throw SimpleException("Timer already started");
    }
    
    timer_event = event_new(evbase->get_event_base() 
            , -1
            , EV_PERSIST
            , trampoline, (void*)this);
    if (!timer_event )
    {
        throw SimpleException("Error making timer_hb_checker");
    }

    event_add(timer_event, &interval);

}

void  TimerHandler::stop_timer()
{ 
    if (!timer_event)
    {
        return;
    }

    evtimer_del(timer_event);
    event_free(timer_event);
    timer_event = NULL;
}



