
#include "connections.h"

#include <arpa/inet.h>


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
    bufferevent_socket_connect_hostname( bev, my_app->get_dns_base(), AF_UNSPEC, hostname, port);
}

void BaseConnection::on_conn_event(short events)
{ 
    //default implemention

    if (events & BEV_EVENT_EOF) {
        int fd = bufferevent_getfd(bev); 
        LOG_INFO("Connection closed on fd #%d.\n", fd);
        post_disconnected();
    }
    else if (events & BEV_EVENT_ERROR) {
        int fd = bufferevent_getfd(bev); 
        int last_err = errno;
        int err = bufferevent_socket_get_dns_error(bev);
        if (err)
        {
            LOG_WARN("DNS error: '%s' on fd #%d.\n", evutil_gai_strerror(err), fd);
        }
        else
        {
            LOG_WARN("Got an error on the connection fd #%d: %s\n"
                ,fd 
                ,strerror(last_err)
                );/*XXX win32*/
        }
        post_disconnected();
    }
    else if (events & BEV_EVENT_TIMEOUT) {
        int fd = bufferevent_getfd(bev); 
        LOG_INFO("Connection got time-out on fd #%d.\n", fd);
        post_disconnected();
    }

}

void AddrInfo::get_peer_info(int s)
{ 
    socklen_t len;
    struct sockaddr_storage addr;

    len = sizeof addr;
    getpeername(s, (struct sockaddr*)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        peer_port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, peer_ipstr, sizeof peer_ipstr);
    } 
    else if (addr.ss_family == AF_INET6)
    { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        peer_port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, peer_ipstr, sizeof peer_ipstr);
    }
    else
    {
        peer_port = 0;
        strncpy(peer_ipstr, "non-inet peer", sizeof peer_ipstr);
    }

}

void  OuterPipe::release_self()
{
    if (is_managed())
    {
        debug_printf("unreg from clientman then delete '%s'\n", dump_2_str().c_str());
        get_pipe_man() -> unregister_connection( get_id() );
    }
    else
    {
        debug_printf("directly delete '%s'\n", dump_2_str().c_str());
        delete this;
    }
}

CString OuterPipeMan::dump_2_str()
{
    CString s;
    iterator it;
    for (it = begin(); it != end() ; it++)
    { 
        OuterPipe* the_pipe = it->second; 
        s.format_append("%s\n" , the_pipe->dump_2_str().c_str());
    }

    return s;
}

OuterPipeMan::OuterPipeMan(SimpleEventLoop  * loop)
    :TimerHandler(loop),event_loop(loop)
{  

    next_pipe_id =0;  

    //struct timeval ten_sec = { HB_CHECK_INTERVAL , 0 };
    //start_timer(ten_sec);
 
}

OuterPipeMan::~OuterPipeMan()
{ 
}

void OuterPipeMan::timer_cb()
{
    //debug_printf("Let's check heartbeat.\n");
    iterator it;

    while (1)
    {
        int finished = 1;

        for (it = begin(); it != end() ; it++)
        { 
            OuterPipe * the_client = it->second; 
            if (! the_client->is_alive())
            {
                LOG_WARN("'%s' is no longer alive, drop it.\n" , the_client->dump_2_str().c_str());
                unregister_connection(  the_client->get_id() );
                finished = 0;
                break;
            }
        }

        if ( finished)
        {
            break;
        }
    }
}


unsigned long OuterPipeMan::allocate_new_id()
{
    AutoLocker _yes_locked( MyBase::lock ); 

    unsigned long  r=  next_pipe_id;
    next_pipe_id ++;

    return r;
    
}

// 从此OuterPipeMan来管理 pipe
int  OuterPipeMan::register_connection(OuterPipe* pipe  )
{
    //AutoLocker _yes_locked( lock );
    unsigned long new_id = allocate_new_id();
    int r =  add_new_item(new_id, pipe);
    if ( r)
    {
        return r;
    }

    pipe->take_it(new_id);

    return 0;
}


void OuterPipeMan::unregister_connection(int client_id)
{
    //AutoLocker _yes_locked( lock );
    OuterPipe* p = get_item(client_id);
    if (!p)
    {
        return;
    }

    remove_item( client_id);
}

