
#include "connections.h"

void BaseConnection::release_bev()
{ 
    if (bev)
    {
        bufferevent_setcb(bev, NULL, NULL, NULL, (void*) this);
        //bufferevent_disable(bev, EV_READ|EV_WRITE);
        bufferevent_free(bev);
        bev = NULL;
    }
}

int BaseConnection::queue_to_send( const void *data, size_t size)
{
    if (!bev)
    {
        return -1;
    }

    return bufferevent_write(bev, data, size);
}

void BaseConnection::trampoline_readable(struct bufferevent *bev, void *ctx)
{
    BaseConnection * real_one = ( BaseConnection*)ctx;
    real_one->on_readable();
}

void BaseConnection::trampoline_writable(struct bufferevent *bev, void *ctx)
{ 
    BaseConnection * real_one = ( BaseConnection*)ctx;
    real_one->on_writable();
}

void BaseConnection::trampoline_event(struct bufferevent *bev, short what, void *ctx)
{ 
    BaseConnection * real_one = ( BaseConnection*)ctx;
    real_one->on_conn_event(what);
}


void BaseConnection::take_socket(evutil_socket_t fd, short event_mask, int options)
{
    bev = bufferevent_socket_new( get_event_base(), fd, options);
    if (!bev) {
        throw SimpleException("Error constructing bufferevent on fd #%d", fd);
    }

    bufferevent_setcb(bev, trampoline_readable, trampoline_writable , trampoline_event , (void*) this);
    bufferevent_enable(bev, event_mask);
}

// attach 'this' to an existing bufferevent
void BaseConnection::take_bev(struct bufferevent * the_bev
            , short event_mask
            , int   options )
{
    release_bev();
    this->bev = the_bev;

    bufferevent_setcb(bev, trampoline_readable, trampoline_writable , trampoline_event , (void*) this);
    bufferevent_enable(bev, event_mask);
}


void BaseConnection::connect_tcp(const char *hostname, int port)
{
    bev = bufferevent_socket_new( get_event_base(), -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        throw SimpleException("Error constructing bufferevent");
    }

    bufferevent_setcb(bev, trampoline_readable, trampoline_writable , trampoline_event , (void*) this);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    bufferevent_socket_connect_hostname( bev, evbase->get_dns_base(), AF_UNSPEC, hostname, port);
}

void BaseConnection::on_conn_event(short events)
{ 
    //default implemention

    if (events & BEV_EVENT_EOF) {
        int fd = bufferevent_getfd(bev); 
        LOG_INFO("Connection closed on fd #%d.\n", fd);
        delete this;
    } else if (events & BEV_EVENT_ERROR) {
        int fd = bufferevent_getfd(bev); 
        int last_err = errno;
        int err = bufferevent_socket_get_dns_error(bev);
        if (err)
        {
            LOG_WARN("DNS error: %s\n", evutil_gai_strerror(err));
        }
        else
        {
            LOG_WARN("Got an error on the connection fd #%d: %s\n"
                ,fd 
                ,strerror(last_err)
                );/*XXX win32*/
        }
        delete this;
    }

}

