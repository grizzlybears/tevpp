#ifndef __EVENT__LOOP_H__
#define  __EVENT_LOOP_H__

#include <event2/event.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "utils.h"

#include "EventLoop.h"

class BaseListener
{
public:
    BaseListener(SimpleEventLoop  * loop)
        :evbase(loop)
         ,the_listener(NULL)
    {
    }

    virtual ~BaseListener()
    {
        if (the_listener)
        {
            evconnlistener_free(the_listener);
            the_listener = NULL;
        }
    }


    struct event_base * get_event_base()
    {
        return evbase->get_event_base();
    }

    /**
      A callback that we invoke when a listener has a new connection.

      @param listener The evconnlistener
      @param fd The new file descriptor
      @param addr The source address of the connection
      @param socklen The length of addr
      @param user_arg the pointer passed to evconnlistener_new()
      */
    static  void trampoline(struct evconnlistener * listner, evutil_socket_t fd, struct sockaddr * addr, int socklen, void * user_data);


    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen) = 0;

    virtual void start_listen_on_fd( evutil_socket_t fd, unsigned flags = LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE)
    {	
        the_listener = evconnlistener_new( get_event_base(), trampoline, (void*)this, flags, -1 , fd);
        if (!the_listener) 
        {
            throw SimpleException("Failed while start_listen on fd '%d', with flags=0x%x.\n", fd, flags);
        }
    }

protected:
    SimpleEventLoop       *evbase;  // just ref, dont touch its life cycle.
    struct evconnlistener *the_listener;
};


class BaseTcpListener
    : public BaseListener 
{
public: 
    BaseTcpListener(SimpleEventLoop  * loop) 
        :BaseListener(loop)
    {
    }
    virtual void start_listen_on_addr(const struct sockaddr *sa, int socklen
            , unsigned flags = LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE )
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

    //virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen);
};

#endif 

