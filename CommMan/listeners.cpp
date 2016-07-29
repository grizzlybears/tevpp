#include <sys/socket.h>
#include <sys/un.h>

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
 
void BaseListener::start_listen_on_addr(const struct sockaddr *sa, int socklen
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

void BaseListener::start_listen_on_tcp(const char * addr , unsigned flags  )
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

void BaseListener::start_listen_on_un(const char * path , unsigned flags  )
{
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path , sizeof(addr.sun_path)-1);

	evutil_socket_t fd;
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        int code = errno; 
		char* msg = strerror(code ); 
        throw SimpleException("Failed to create unix domain socket, errno = %d, %s\n", code, msg);
    }

    if (evutil_make_socket_nonblocking(fd) < 0) {
		evutil_closesocket(fd);
        throw SimpleException("Failed to create unix domain socket at 'evutil_make_socket_nonblocking'\n");
	}

	if (flags & LEV_OPT_CLOSE_ON_EXEC) {
		if (evutil_make_socket_closeonexec(fd) < 0) {
			evutil_closesocket(fd);
            throw SimpleException("Failed to create unix domain socket at 'evutil_make_socket_closeonexec'\n");
		}
	}

    if (flags & LEV_OPT_REUSEABLE) {
		if (evutil_make_listen_socket_reuseable(fd) < 0) {
			evutil_closesocket(fd);
            throw SimpleException("Failed to create unix domain socket at 'evutil_make_listen_socket_reuseable'\n");
		}
	}

    unlink(path);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) <0) 
    {
        evutil_closesocket(fd); 
        int code = errno; 
		char* msg = strerror(code ); 
        throw SimpleException("Failed to bind  unix domain socket on '%s', errno = %d, %s\n", path , code, msg);
    }

    start_listen_on_fd( fd, flags );
}


