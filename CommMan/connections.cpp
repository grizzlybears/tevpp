
#include "connections.h"

void BaseConnection::safe_release()
{ 
    if (bev)
    {
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
    bev = bufferevent_socket_new( get_event_base(), fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        throw SimpleException("Error constructing bufferevent on fd #%d", fd);
    }

    bufferevent_setcb(bev, trampoline_readable, trampoline_writable , trampoline_event , (void*) this);
    bufferevent_enable(bev, event_mask);

}

