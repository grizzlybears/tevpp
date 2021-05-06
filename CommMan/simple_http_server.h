#pragma once 

#include <event2/event.h>
#include <event2/util.h>
#include <evhttp.h>

#include "utils.h"
#include "EventLoop.h"
#include "thread_utils.h"
#include "timers.h"
#include <deque>

class SimpleHttpRequestHandler;


class BaseHttpServer
{
public:
     BaseHttpServer(SimpleEventLoop  * loop)
        :my_app(loop), http(NULL)
    {
    } 

     BaseHttpServer(SimpleEventLoop  * loop, const char * addr, ev_uint16_t port )
        :my_app(loop), http(NULL)
    {
        start_server(addr, port);
    }


    virtual ~ BaseHttpServer()
    {
        //debug_printf("yes, dtor \n");
         release_server();
    }

    struct event_base * get_event_base()
    {
        return my_app->get_event_base();
    }

    SimpleEventLoop  * get_app() const
    {
        return my_app;
    }

    int start_server(const char * 	address, ev_uint16_t port );
 
    virtual void release_server()
    {
        if (http)
        {
            evhttp_free(http);
            http= NULL;
        }
    }

    static void trampoline(struct evhttp_request *, void *);
    
    virtual void generic_handler(struct evhttp_request *) ;


    struct evhttp *  get_http_server()
    {
        return http;
    } 

protected:
    SimpleEventLoop       *my_app;  // just ref, dont touch its life cycle.
    struct evhttp *http;
};

class SimpleHttpRequestHandler
{
public:

     SimpleHttpRequestHandler(SimpleEventLoop  * loop)
        :my_app(loop)
    {
    } 

     SimpleHttpRequestHandler(SimpleEventLoop  * loop, BaseHttpServer * server, const char * path)
        :my_app(loop)
    {
        handle_path(server , path );
    }


    virtual ~SimpleHttpRequestHandler()
    {
    }

    struct event_base * get_event_base()
    {
        return my_app->get_event_base();
    }

    SimpleEventLoop  * get_app() const
    {
        return my_app;
    }
    static void trampoline(struct evhttp_request *, void *);
    
    virtual void handler(struct evhttp_request * req);

    int handle_path(BaseHttpServer  *server ,const char * path );

protected:
    SimpleEventLoop     *my_app;  // just ref, dont touch its life cycle.

};

