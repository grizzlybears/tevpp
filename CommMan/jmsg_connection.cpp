#include "jmsg_connection.h"

#define TRACE_JMSG (0)

bool  BaseJMessage::parse_from_plain_msg(PlainMessage * plain_msg, std::string& parse_error)
{
    Json::Reader reader;
    
    this->sequence =  parse_int( plain_msg->header.sequence, sizeof(plain_msg->header.sequence));
    int msg_size = parse_int(plain_msg->header.data_size , sizeof(plain_msg->header.data_size));

    bool b = reader.parse(
        plain_msg->data
        ,plain_msg->data + msg_size
        ,this->jdata
        );

    if (!b)
    {
        parse_error = reader.getFormattedErrorMessages();
        return false;
    }


    if (!this->jdata.isObject())
    {
        parse_error = "Not json object";
        return false;
    }

    //所有的msg都必须有"Command"
    this->Command = this->jdata["Command"].asString();
    if ("" == this->Command)
    { 
        Json::StyledWriter  writer;
        std::string jstr = writer.write(jdata);
        debug_printf("Bad?\n%s\n ", jstr.c_str());


        parse_error = "No 'Command' found";
        return false;
    }

    return true;
}

PlainMessage * JMsgConnection::fetch_msg()
{
    struct evbuffer *input = bufferevent_get_input(get_bev());
    size_t buffer_len = evbuffer_get_length( input );

    if (buffer_len < sizeof( ChunkHeader))
    {
        // 头还没到齐
        return NULL;
    } 
    
    ChunkHeader head;
    evbuffer_copyout(input , &head, sizeof(head));

    // 提取 head.data_size
    int data_size =  parse_int( head.data_size, sizeof(head.data_size)) ;

    if (data_size < 0 || data_size > MAX_DATASIZE)
    {
        throw ConnectionGoneException(CString("Bad datasize %d", data_size));
    }
    //debug_printf("datasize = %d\n", data_size);

    size_t msg_size = sizeof(head) + (size_t)data_size; 
    if (buffer_len < msg_size )
    {
        //没到齐
        return NULL;
    }

    PlainMessage * msg = (PlainMessage *)malloc(msg_size + 1);
    if (!msg)
    {
        head.space1[0] = 0;
        LOG_ERROR("Failed to alloc msg for %s , seq = %s\n", dump_2_str().c_str(), head.sequence);
        evbuffer_drain(input , msg_size);
        return NULL;
    }

    evbuffer_remove(input , msg, msg_size);
    msg->data[ data_size] = 0;
#if TRACE_JMSG
    LOG_DEBUG("Raw income  msg:\n%s\n", msg->header.sequence);
#endif
    return msg;
}

int JMsgConnection::process_then_free_msg(PlainMessage * plain_msg)
{
    AutoFree __dont_leak( plain_msg);
    BaseJMessage msg;
    bool b;
    std::string parse_error;

    b = msg.parse_from_plain_msg(plain_msg, parse_error);
    if (!b)
    {
        debug_printf("parse error:%s\n", parse_error.c_str());
        
        int seq = PARSE_INT(plain_msg->header.sequence);
        
        send_error_msg_then_bye( 
            seq
            , -2
            , parse_error.c_str());

        return 1;
    }

    return  really_process_msg( &msg);  
}

CString BaseJMessage::dump_2_str()const
{ 
    Json::StyledWriter  writer;
    CString s(
        "seq=%d\n%s"
        , this->sequence
        , writer.write( this->jdata).c_str()
        );
        
    return s;
}

int JMsgConnection::really_process_msg(BaseJMessage* msg)
{
    try
    { 
        debug_printf("Got msg with Command = '%s' , but won't process it :)\n/////////////////\n"
            //, msg->dump_2_str().c_str()
            , msg->Command.c_str()
            );
        return 0;
    }
    catch (std::exception& ex)
	{
		LOG_ERROR("Exception while process msg from %s: %s\n"
            , dump_2_str().c_str()
            , ex.what());
            
        send_error_msg_then_bye( 
                    msg->sequence
                    , -1
                    , ex.what()
                    );
        return 1;
	}

    return 1;
}


void JMsgConnection::send_msg( int32_t seq, const Json::Value& jdata  )
{
    Json::StyledWriter  writer;
    std::string jstr = writer.write(jdata);

    ChunkHeader head;
    snprintf( head.sequence, sizeof(head.sequence) + 1, "% *d", (int)sizeof(head.sequence),  seq);  
    int data_size = jstr.length();
    snprintf( head.data_size, sizeof(head.data_size) + 1, "% *d", (int)sizeof(head.data_size), data_size );
    head.space1[0] = ' ';
    head.lf[0] = '\n';

#if TRACE_JMSG
    char head_buf[sizeof(head) + 1];
    memcpy((void*)head_buf, (void*)head.sequence, sizeof(head));
    head_buf[sizeof(head) ] = 0;

    LOG_DEBUG("seq= %d, Outgoing:%s%s\n", seq, head_buf, jstr.c_str());
    MAKE_SURE_OUTGOING_DIAGRAM_CONTINUE;
#endif 

    queue_to_send( &head , sizeof(head));
    queue_to_send( jstr.c_str() , (size_t)data_size );
}

void JMsgConnection::send_error_msg( int32_t seq, int32_t code, const char * error_msg )
{ 
    Json::Value root;
    root["Result"] = code;  
    root["Command"] = "ACK";
    root["Message"]  = error_msg;
    

    send_msg( seq, root);
}

void JMsgConnection::send_error_msg_then_bye( int32_t seq
    , int32_t code
    , const char * error_msg )
{
    /*
    应答
    {
        "type": "Ack",
        "error_no": 0,
        "error_msg":""
    }
    */

    
    Json::Value root;
    root["Result"] = code; 
    root["Command"] = "ACK";
    root["Message"]  = error_msg;
    

    send_msg( seq, root);
    
    
       //mark_exiting();    // 另一个线程中这么做很可能不能触发 write_cb
    //  Bufferevents are internally reference-counted, 
    //  so if the bufferevent has pending deferred callbacks when you free it, 
    //  it won’t be deleted until the callbacks are done.
    
    // 所以
    //bufferevent_flush(get_bev(),EV_WRITE, BEV_FINISHED);
    //close(bufferevent_getfd(get_bev()));  // 这样也不能触发 connect_cb


    throw ConnectionGoneException("Bye");

}

    
void JMsgConnection::send_error_msg_then_disconn( int32_t seq,  int32_t code, const char * error_msg )
{ 

    Json::Value root;    
    
    root["Result"] = code;
    root["Command"] = "ACK";
    root["Message"]  = error_msg;
 
    mark_exiting();
    send_msg( seq, root);
}



void  JMsgConnection::on_readable()
{ 
    try
    { 
        //debug_printf("yes, we have incoming data\n");
        PlainMessage * msg;

        while ( (msg = fetch_msg() ) )
        {
            //debug_printf("got a plain msg\n");
            process_then_free_msg(msg);
        }

    } 
    catch (ConnectionGoneException& cge)
    {
		LOG_INFO("%s was interrupted.\n", dump_2_str().c_str());
        release_self();
        return;
    }
    catch (std::exception& ex)
	{
		LOG_ERROR("Exception while handle pipe command: %s\n", ex.what());
        return ;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception while handle command pipeb\n");
        return ;
	}
   
}


void JMsgConnection::on_writable()
{
    if (!disconnect_if_all_sent )
    {
        return;
    }
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0)
    {
        release_self();
    }
}

void  NakedJsonConnection::on_readable()
{ 
    try
    { 
        //debug_printf("yes, we have incoming data\n");
        Json::Value  root;
        while (  fetch_msg(root) > 0 )
        {
             really_process_msg(root);
        }

    } 
    catch (ConnectionGoneException& cge)
    {
		LOG_INFO("%s was interrupted.\n", dump_2_str().c_str());
        release_self();
        return;
    }
    catch (std::exception& ex)
	{
		LOG_ERROR("Exception while handle pipe command: %s\n", ex.what());
        return ;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception while handle command pipeb\n");
        return ;
	}
   
}


void NakedJsonConnection::on_writable()
{
    if (!disconnect_if_all_sent )
    {
        return;
    }
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0)
    {
        release_self();
    }
}

    //返回: >  0  成功获得json
    //      == 0  没获得json
    //      <  0  其他失败
int  NakedJsonConnection::fetch_msg(Json::Value&  root )
{
    struct evbuffer *input = bufferevent_get_input(get_bev());
    size_t buffer_len = evbuffer_get_length( input );

    if (!buffer_len)
    {
        return 0;
    }

    char  * msg = (char*)malloc( buffer_len + 1);
    if (!msg)
    {
        LOG_ERROR("Fail to allocate %u for income msg.\n", (unsigned int)buffer_len );
        return -1;
    }

    AutoFree yes_always(msg);

    evbuffer_remove(input , msg, buffer_len);
    msg[buffer_len ] = 0;

#if TRACE_JMSG
    LOG_DEBUG("Raw income  msg:\n%s\n", msg);
#endif

    Json::Reader reader;
    bool b = reader.parse(
        msg
        ,msg + buffer_len
        ,root
        );

    if (!b)
    {
        std::string parse_error = reader.getFormattedErrorMessages();
        LOG_ERROR("%s while parsing:\n%s\n", parse_error.c_str(), msg);
        return -2;
    }


    if (!root.isObject())
    {
        std::string parse_error = "Not json object";
        LOG_ERROR("%s while parsing:\n%s\n", parse_error.c_str(), msg);
        return -3;
    }

    return 1;
}


int NakedJsonConnection::really_process_msg(const Json::Value&  root )
{ 
    Json::StyledWriter  writer;
    std::string jstr = writer.write(root);

    LOG_DEBUG("Let's process:\n%s\n", jstr.c_str());

    return 0 ;
}

void  NakedJsonConnection::send_msg(  const Json::Value& jdata  )
{
    Json::StyledWriter  writer;
    std::string jstr = writer.write(jdata);

    int data_size = jstr.length();

#if TRACE_JMSG
    LOG_DEBUG(" Outgoing:%s\n", jstr.c_str());
#endif 

    queue_to_send( jstr.c_str() , (size_t)data_size);
}


