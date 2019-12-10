#ifndef  __CONNECTIONS_H__
#define  __CONNECTIONS_H__

#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "utils.h"
#include "EventLoop.h"

class BaseConnection
{
public:
    BaseConnection(SimpleEventLoop  * loop)
        :evbase(loop), bev(NULL)
    {
    }

    virtual ~BaseConnection()
    {
        //debug_printf("yes, dtor \n");
        release_bev();
    }

    struct event_base * get_event_base()
    {
        return evbase->get_event_base();
    }

    // attach 'this' to an existing fd
    virtual void take_socket(evutil_socket_t fd
            , short event_mask = EV_WRITE |  EV_READ
            , int   options =  BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE);

    // attach 'this' to an existing bufferevent
    virtual void take_bev(struct bufferevent    *bev
            , short event_mask = EV_WRITE |  EV_READ
            , int   options =  BEV_OPT_THREADSAFE );


    // connect 'this' to tcp addr:port
    virtual void connect_tcp(const char *hostname, int port);

    virtual void post_disconnected()
    {
        delete this;
        // In case of  'this' is managed by outer, override this member
    }

    virtual void release_bev();

    virtual int queue_to_send( const void *data, size_t size);

    static  void trampoline_readable(struct bufferevent *bev,  void *ctx);
    static  void trampoline_writable(struct bufferevent *bev,  void *ctx);
    static  void trampoline_event(struct bufferevent *bev, short what, void *ctx);

    virtual void on_readable() 
    {
    }

    virtual void on_writable()
    {
    }

    virtual void on_conn_event(short what);

    struct bufferevent * get_bev()
    {
        return bev;
    }

protected:
    SimpleEventLoop       *evbase;  // just ref, dont touch its life cycle.
    struct bufferevent    *bev;
};

class EvBufAutoLocker
{
public:
    struct evbuffer *buf; 
    EvBufAutoLocker(struct evbuffer * to_lock)
    {
        buf = to_lock;
        
        if (!buf)
        {
            return;
        }
        evbuffer_lock(buf);
    }

    virtual ~EvBufAutoLocker()
    { 
        if (!buf)
        {
            return;
        }
        evbuffer_unlock(buf);
        buf = NULL;
    }

};


#define MAKE_SURE_OUTGOING_DIAGRAM_CONTINUE  EvBufAutoLocker __make_sure_msg_conti(bufferevent_get_output(get_bev()))

#endif
