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

/*
void  BaseTcpListener::listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen)
{
}
*/
