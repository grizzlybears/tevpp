/*
  This exmple program provides a trivial http server program that listens on port 9090.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).

  And now, it also listens on a unix domain socket.
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string>

#include "EventLoop.h"
#include "signals.h"
#include "simple_http_server.h"

static const int PORT = 9090;
typedef std::vector<char > DynaBuf;

class SampleHttpServer
    :public BaseHttpServer
{
public: 
    SampleHttpServer(SimpleEventLoop  * loop, const char * addr, ev_uint16_t port )
        :BaseHttpServer( loop, addr, port )
    {
    }

    static const char * decode_method(int req_method );
    
    void generic_handler(struct evhttp_request * req); 
    
    void handle_foo(struct evhttp_request * req);

    std::vector<CString> posts;
    void handle_list(struct evhttp_request * req);
     
    void handle_new(struct evhttp_request * req, const DynaBuf& post_data);
    
    void show_list(struct evhttp_request * req, struct evbuffer* out_buf );
    void show_form(struct evhttp_request * req, struct evbuffer* out_buf );
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

const char * SampleHttpServer::decode_method(int req_method )
{ 
    const char *cmdtype;

	switch (req_method ) {
	case EVHTTP_REQ_GET: cmdtype = "GET"; break;
	case EVHTTP_REQ_POST: cmdtype = "POST"; break;
	case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	default: cmdtype = "unknown"; break;
	}

    return cmdtype;

}


void  SampleHttpServer::generic_handler(struct evhttp_request * req)
{ 
    int req_method = evhttp_request_get_command(req);
    const char *cmdtype =   SampleHttpServer::decode_method(req_method);

    const std::string uri = evhttp_request_get_uri(req);

	CString s("Received a %s request for %s , req = %p\n"
            ,  cmdtype
            , uri.c_str()
            , req
            );

    struct evkeyvalq *headers;
	struct evkeyval *header;

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
        s.format_append("    %s: %s\n", header->key, header->value);
	}

	s+= "post data: <<<\n";
    LOG_DEBUG("%s", s.c_str());

    DynaBuf post_data; 
    struct evbuffer *buf;
	buf = evhttp_request_get_input_buffer(req);

    size_t data_len = evbuffer_get_length(buf); 
    post_data.resize( data_len + 1);
    post_data[data_len] = 0;

	evbuffer_remove(buf, & post_data[0], data_len);

	fwrite(& post_data[0], 1, data_len , stderr);    // LOG_XXX 最终写到stderr
	RAW_LOG("\n>>>\n");

    ////////////////////////////////////////////////////////////// 
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    try
    {
        if ("/foo" == uri )
        {
            this->handle_foo(req);
        }
        else if ("/list" == uri )
        {
            this->handle_list(req);
        }
        else if ("/new" == uri )
        {
            this->handle_new(req, post_data);
        }
    }
    catch (std::exception& e)
    { 
        LOG_ERROR( "While handle '%s', got '%s'.\n"
            , uri.c_str()
            , e.what());

        evbuffer_add_printf(out_buf
            , "While handle '%s', got '%s'.\n"
            , uri.c_str()
            , e.what()
            );

        evhttp_send_reply(req, HTTP_OK, "", out_buf);
        
        return;
    }

    evbuffer_add_printf(out_buf
            , "'%s' is not handled.\n", uri.c_str());

    evhttp_send_reply(req, HTTP_NOTFOUND, "", out_buf);
    
    //LOG_WARN("'%s' is not handled.\n", uri.c_str());

}

void SampleHttpServer::handle_foo(struct evhttp_request * req)
{ 
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    if (!out_buf)
    {
        LOG_THEN_THROW("Failed to  evhttp_request_get_output_buffer(). \n");
        return;
    }

    evbuffer_add_printf(out_buf
            , "<html><body>Yes, I am foo.</body></html>\n");

    evhttp_send_reply(req, HTTP_OK, "", out_buf);

}

void SampleHttpServer::handle_list(struct evhttp_request * req)
{ 
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    if (!out_buf)
    {
        LOG_THEN_THROW("Failed to  evhttp_request_get_output_buffer(). \n");
        return;
    }

    evbuffer_add_printf(out_buf
            , "<html><body>\n"); 

    this->show_list(req, out_buf);
    this->show_form(req, out_buf);

    evbuffer_add_printf(out_buf
            , "</body></html>\n");


    evhttp_send_reply(req, HTTP_OK, "", out_buf);

}

void SampleHttpServer::show_list(struct evhttp_request * req, struct evbuffer* out_buf )
{
    std::vector<CString>::iterator it;
    for (it = this->posts.begin(); it != this->posts.end(); it++ )
    {
        CString s("<pre>%s</pre><hr/>\n", it->c_str());
        evbuffer_add_printf(out_buf, "%s", s.c_str());
    }


}

void SampleHttpServer::show_form(struct evhttp_request * req, struct evbuffer* out_buf )
{
    CString form;
    form += "<form action=\"/new\"  method=\"post\">\n";
    form += "<input type=\"text\"  name=\"content\"><br><br>\n";
    form += "<input type=\"submit\" value=\"Submit\">\n</form>\n";
    evbuffer_add_printf(out_buf, "%s", form.c_str());
}

void SampleHttpServer::handle_new(struct evhttp_request * req, const DynaBuf& post_data)
{ 
    struct evbuffer* out_buf = evhttp_request_get_output_buffer(req);
    if (!out_buf)
    {
        LOG_THEN_THROW("Failed to  evhttp_request_get_output_buffer(). \n");
        return;
    }
    std::vector<CString> pairs; 
    size_t n = str_split( pairs, &post_data[0],  '&');
    if (!n)
    {
        LOG_THEN_THROW("no kv pair found in post data.\n");
        return;
    }
    
    std::vector<CString> kv; 
    n = str_split( kv, pairs[0].c_str(),  '='); 
    if (2 != n){
        LOG_THEN_THROW("bad kv pair '%s'.\n", pairs[0].c_str());
        return;
    }

    this->posts.push_back(kv[1]);


    ////////////////////////////////////////////

    evbuffer_add_printf(out_buf
            , "<html><body>\n"); 

    this->show_list(req, out_buf);
    this->show_form(req, out_buf);

    evbuffer_add_printf(out_buf
            , "</body></html>\n");


    evhttp_send_reply(req, HTTP_OK, "", out_buf);

}


