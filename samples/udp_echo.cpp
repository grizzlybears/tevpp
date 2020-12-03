/*
  This exmple program provides a trivial server program that listens for UDP on port 9995, and echo back whatever it recv.

 It exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "signals.h"
#include "connections.h"

static const int PORT = 9995;
#define BIND_ADDR "0.0.0.0"

#define BUF_SIZE (1500)

class HelloServerUdp
    :public   BaseDiagram
{
public:
    HelloServerUdp(SimpleEventLoop* loop)
         :  BaseDiagram(loop)
    {
    }

    virtual void on_readable()
    { 
        char                buf[BUF_SIZE];
        int                 len;
        socklen_t           size = sizeof(struct sockaddr);
        struct sockaddr_in  client_addr;

        memset(buf, 0, sizeof(buf));
        len = recvfrom( this->sock_fd
                , buf, sizeof(buf), 0
                , (struct sockaddr *)&client_addr, &size);

        if (len == -1) {
            perror("recvfrom()");
        } else if (len == 0) {
            printf("Connection Closed\n");
        } else {
            printf("Read: len [%d] - content [%s]\n", len, buf);

            /* Echo */
            sendto( this->sock_fd, buf, len, 0, (struct sockaddr *)&client_addr, size);
        }

    }
};


int main(int argc, char **argv)
{
    try
	{ 
        SimpleEventLoop loop;
        
        //1. bind on PORT
        HelloServerUdp  udp_receiver( &loop);
        udp_receiver.bind_udp( BIND_ADDR, PORT);

        printf("Yes, binded on UDP: %s:%d\n", BIND_ADDR, PORT);
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);
        printf("Ctrl-C to exit gracefully\n");

        //3. the main loop
        loop.run();

        printf("done\n");

	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Exception in main(): %s\n", ex.what());
        return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception in main()\n");
        return 1;
	}
    return 0;
}

