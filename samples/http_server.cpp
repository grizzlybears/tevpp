/*
  This exmple program provides a trivial http server program that listens on port 9090.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).

  And now, it also listens on a unix domain socket.
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "EventLoop.h"
#include "signals.h"
#include "simple_http_server.h"

static const int PORT = 9090;

class SampleHttpServer
    :public BaseHttpServer
{
public: 
    SampleHttpServer(SimpleEventLoop  * loop, const char * addr, ev_uint16_t port )
        :BaseHttpServer( loop, addr, port )
    {
    }
};

class HandlerFoo
    :public SimpleHttpRequestHandler
{
public:
     HandlerFoo(SimpleEventLoop  * loop, BaseHttpServer * server)
         :SimpleHttpRequestHandler(loop,  server, "/foo")
    {
    }

    virtual void handler(struct evhttp_request * req)
    { 
        struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
        if (!out_buf)
        {
            LOG_WARN("Failed to  evhttp_request_get_output_buffer(). \n");
            return;
        }

        evbuffer_add_printf(out_buf
                , "<html><body>Yes, I am foo.</body></html>\n");

        evhttp_send_reply(req, HTTP_OK, "", out_buf);

    }

};


int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif
    try
	{ 
        SimpleEventLoop loop;
        
        //1. listen as http server
        SampleHttpServer server(&loop, "0.0.0.0", PORT); 

        HandlerFoo foo(&loop, &server);
 
        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);

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

