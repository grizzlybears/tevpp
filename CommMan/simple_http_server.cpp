#include "simple_http_server.h"

int  BaseHttpServer::start_server(const char * 	address, ev_uint16_t port )
{
    if (http)
    {
        LOG_WARN("http server already started, will restart.\n");
        release_server();
    }

    http = evhttp_new(this->get_event_base());

    int r =  evhttp_bind_socket	(http,	address,port );
    if (r)
    {
        LOG_ERROR("evhttp_bind_socket() failed, code = %d\n", r);
        return r;
    }
    
    evhttp_set_gencb(http, trampoline,this);
    return 0;
}

void BaseHttpServer::trampoline(struct evhttp_request * req, void * opaque )
{ 
    BaseHttpServer * who = (BaseHttpServer * )opaque;
    who-> generic_handler(req);

}

void BaseHttpServer::generic_handler(struct evhttp_request * req) 
{ 
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    if (!out_buf)
    {
        LOG_WARN("Failed to  evhttp_request_get_output_buffer(). \n");
        return;
    }

    evbuffer_add_printf(out_buf
            , "<html><body>Hello, world.</body></html>\n");

    evhttp_send_reply(req, HTTP_OK, "", out_buf);

}



void  SimpleHttpRequestHandler::trampoline(struct evhttp_request * req, void * opaque)
{ 
    SimpleHttpRequestHandler * who = (SimpleHttpRequestHandler * )opaque;
    who->handler(req);
}

int SimpleHttpRequestHandler::handle_path(BaseHttpServer  *server ,const char * path )
{
    int r = evhttp_set_cb(server->get_http_server(), path, trampoline, this);
    if ( -1 == r )
    {
        LOG_WARN("'%s' was already handled\n", path);
    }
    else if( -2 == r)
    {
        LOG_ERROR("Failed in set_cb for '%s'\n", path);
    }

    return r;
}

void SimpleHttpRequestHandler::handler(struct evhttp_request * req)
{
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    if (!out_buf)
    {
        LOG_WARN("Failed to  evhttp_request_get_output_buffer(). \n");
        return;
    }

    evbuffer_add_printf(out_buf
            , "<html><body>This is default handler.</body></html>\n");

    evhttp_send_reply(req, HTTP_OK, "", out_buf);

}

