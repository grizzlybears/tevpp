#ifndef  __JMSG_CONNECTION_H__
#define  __JMSG_CONNECTION_H__

#include "json/json.h"
#include "connections.h"

/* 
   Inspired from spice-protocol/spice/vd_agent.h
 
 */


// {{{ start_packed.h
#ifdef __GNUC__

  #define SPICE_ATTR_PACKED __attribute__ ((__packed__))

  #ifdef __MINGW32__
  #pragma pack(push,1)
  #endif

#else

  #pragma pack(push)
  #pragma pack(1)
  #define SPICE_ATTR_PACKED
  #pragma warning(disable:4200)
  #pragma warning(disable:4103)

#endif
// }}}


struct SPICE_ATTR_PACKED  ChunkHeader{
    char sequence[10];  // 整数，左补空格
                       // 会话中的消息流水号，发起方自行负责采号，服务端给回信的时候，回填对应的流水号。
                       // 如果无须回信，或者不形成会话，此处为负数。
                       
    char space1[1];    // 空格

    char data_size[8]; // 整数，左补空格 
                       //不包括 ChuckHeader自身的size

    char lf[1];        // 换行
public:
    ChunkHeader()
    {
        memset((void*)this, 0, sizeof(*this));
        lf[0] = '\n';
    }
};

struct SPICE_ATTR_PACKED  PlainMessage{
    ChunkHeader header;
    char        data[0];   // 必须为json，格式见下
};

#define MAX_DATASIZE (1024*1024)

/*
1.客户端消息"Query"
    {
        "Command": "QUERY",     <== '命令字' 必须
        "BoxID": "asdqwe123"         其余视'命令字'而定
    }

    应答
    {
        "Result": "1",          
        "BoxID": "asdqwe123",
        "RemainNum": 2,
        "Slots":[
            {
                "Slot":1,
                "TerminalID": "oopp009988",
                "Level":4 
            }

            ,{
                "Slot":2,
                "TerminalID": "oopp009989",
                "Level":3
            }
        ]
    }

 * */


// {{{  end_packed.h
#undef SPICE_ATTR_PACKED                                                                                                                   
#if defined(__MINGW32__) || !defined(__GNUC__)                                                                                             
  #pragma pack(pop)                                                                                                                          
#endif
// }}}

class BaseJMessage;

// 报文负载是JSON的会话
class JMsgConnection
    :public  OuterPipe
{
public:
    typedef OuterPipe MyBase;

    JMsgConnection(SimpleEventLoop* loop)
         : MyBase(loop)
    { 
        disconnect_if_all_sent = 0;
    }

    JMsgConnection(SimpleEventLoop* loop, evutil_socket_t fd  )
         :MyBase(loop)
    {
        disconnect_if_all_sent = 0;
        take_socket(fd );

        // TODO: sometimes it fails here!
        //int i = evbuffer_enable_locking( bufferevent_get_output( get_bev()) , NULL);
        //if (i)
        //{
        //    LOG_ERROR("Couldn't enable lock for command pipe.\n");
        //}
    }

    virtual void on_readable();
    virtual void on_writable();
    

    virtual PlainMessage * fetch_msg(); // 从buf中获得一个完整的msg，否则不动buf
    
    //如果想在其他线程处理消息，需要重载本函数
    virtual int process_then_free_msg( PlainMessage * msg); 

    //通常需要重载本函数
    virtual int really_process_msg(BaseJMessage* msg); 
    

    virtual void send_msg( int32_t seq, const Json::Value& jdata  );
    virtual void send_error_msg( int32_t seq, int32_t code, const char * error_msg );
    

    // 只用于libevent在conn上的回调
    virtual void send_error_msg_then_bye( int32_t seq,  int32_t code, const char * error_msg );
    
    // 用于worker threads
    virtual void send_error_msg_then_disconn( int32_t seq,  int32_t code, const char * error_msg );
    
    //virtual void send_normal_ack( int32_t seq);

    //通常需要重载本函数
    virtual CString dump_2_str() const
    {
        return "[JMsgConnection]";
    }

    void mark_exiting()
    { 
        disconnect_if_all_sent = 1;
    }

    int disconnect_if_all_sent; // 数据发送光就断开

};

// 解析之后的PlainMessage
class BaseJMessage
{
public:
    int32_t     sequence;  
    Json::Value jdata;

    std::string Command; // 冗余一点

    bool parse_from_plain_msg(PlainMessage * plain_msg, std::string& parse_error);
    CString dump_2_str()const ;
};

// 报文是裸JSON的连接
class NakedJsonConnection
    :public  OuterPipe
{
public:
    typedef OuterPipe MyBase;

     NakedJsonConnection(SimpleEventLoop* loop)
         : MyBase(loop)
    { 
        disconnect_if_all_sent = 0;
    }

    NakedJsonConnection(SimpleEventLoop* loop, evutil_socket_t fd  )
         :MyBase(loop)
    {
        disconnect_if_all_sent = 0;
        take_socket(fd );
    }

    virtual void on_readable();
    virtual void on_writable();


    //返回: >  0  成功获得json
    //      == 0  没获得json
    //      <  0  其他失败
    int fetch_msg(Json::Value&  root );

    //通常需要重载本函数
    virtual int really_process_msg(const Json::Value&  root ); 

    virtual void send_msg( const Json::Value& jdata  );

   
    //virtual void send_normal_ack( int32_t seq);

    //通常需要重载本函数
    virtual CString dump_2_str() const
    {
        return "[NakedJsonConnection]";
    }

    void mark_exiting()
    { 
        disconnect_if_all_sent = 1;
    }

    int disconnect_if_all_sent; // 数据发送光就断开

};


#endif

