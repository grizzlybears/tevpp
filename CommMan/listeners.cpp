#include "listeners.h"


void BaseListener::trampoline(struct evconnlistener * listner
        , evutil_socket_t fd
        , struct sockaddr * addr
        , int socklen
        , void * user_data)
{ 
    BaseListener* real_one = (BaseListener*) user_data;

    real_one->listener_cb( fd, addr , socklen);
}

void BaseListener::start_listen_on_fd( evutil_socket_t fd, unsigned flags )
{	
    the_listener = evconnlistener_new( get_event_base(), trampoline, (void*)this, flags, -1 , fd);
    if (!the_listener) 
    {
        throw SimpleException("Failed while start_listen on fd '%d', with flags=0x%x.\n", fd, flags);
    }
}
 
void BaseTcpListener::start_listen_on_addr(const struct sockaddr *sa, int socklen
        , unsigned flags  )
{	
    the_listener = evconnlistener_new_bind( get_event_base(), trampoline, (void*)this
            , flags, -1 
            , sa, socklen
            );
    if (!the_listener) 
    {
        throw SimpleException("Failed while start_listen on addr with flags=0x%x.\n",  flags);
    }
}

void BaseTcpListener::start_listen_on_addr2(const char * addr , unsigned flags  )
{
    struct sockaddr_storage listen_on_addr; 
	int socklen = sizeof(listen_on_addr);

	memset(&listen_on_addr, 0, sizeof(listen_on_addr));
    
    int i = evutil_parse_sockaddr_port(addr,(struct sockaddr*)&listen_on_addr, &socklen); 

    if ( i)
    {
        throw SimpleException("Bad listen addr: %s.\n", addr);
    }

    the_listener = evconnlistener_new_bind( get_event_base(), trampoline, (void*)this
            , flags, -1 
            , (struct sockaddr*)&listen_on_addr, socklen
            );
    if (!the_listener) 
    {
        throw SimpleException("Failed while start_listen on '%s' with flags=0x%x.\n", addr, flags);
    }
}


