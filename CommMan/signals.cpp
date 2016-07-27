
#include "signals.h"

void  BaseSignalHandler::start_handle_signal( int sig_num)
{	
    signal_event = evsignal_new(get_event_base(), sig_num , trampoline , (void *)this);

    if (!signal_event || event_add(signal_event, NULL)<0) {
        throw SimpleException("Could not create/add a signal handler on sig #%d\n", sig_num);
    }
}

void  BaseSignalHandler::trampoline(evutil_socket_t sig, short events, void *user_data)
{ 
    BaseSignalHandler* real_one = (BaseSignalHandler*) user_data;

    real_one->signal_cb( );
}


