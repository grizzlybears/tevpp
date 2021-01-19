#ifndef  __LISTENERS_H__
#define  __LISTENERS_H__

#include <event2/event.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "utils.h"

#include "EventLoop.h"

class BaseListener
{
public:
    BaseListener(SimpleEventLoop  * loop)
        :my_app(loop)
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
        return my_app->get_event_base();
    }
    
    SimpleEventLoop  * get_app() const
    {
        return my_app;
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

    virtual void start_listen_on_fd( evutil_socket_t fd, unsigned flags = LEV_OPT_CLOSE_ON_EXEC |LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE);

    virtual void start_listen_on_addr(const struct sockaddr *sa, int socklen
            , unsigned flags = LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE );

    virtual void start_listen_on_tcp(const char* addr
            , unsigned flags = LEV_OPT_CLOSE_ON_EXEC |LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE );
#ifdef __GNUC__    
    virtual void start_listen_on_un(const char* addr
            , unsigned flags = LEV_OPT_CLOSE_ON_EXEC |LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE );
#endif

protected:
    SimpleEventLoop       *my_app;  // just ref, dont touch its life cycle.
    struct evconnlistener *the_listener;
};

#endif 

