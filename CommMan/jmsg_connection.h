#ifndef  __JMSG_CONNECTION_H__
#define  __JMSG_CONNECTION_H__

#include "connetion_common.h"
#include "json/json.h"

struct PlainMessage;
class BaseJMessage;

// 报文负载是JSON的会话
class JMsgConnection
    :public BaseConnection
{
public:
    JMsgConnection(SimpleEventLoop* loop)
         : BaseConnection(loop)
    { 
        disconnect_if_all_sent = 0;
    }

    JMsgConnection(SimpleEventLoop* loop, evutil_socket_t fd  )
         : BaseConnection(loop)
    {
        disconnect_if_all_sent = 0;
        take_socket(fd ,EV_WRITE |  EV_READ, BEV_OPT_CLOSE_ON_FREE );
        //take_socket(fd );
        //

        int i = evbuffer_enable_locking( bufferevent_get_output( get_bev()) , NULL);
        if (i)
        {
            LOG_ERROR("Couldn't enable lock for command pipe.\n");
        }
    }

    virtual void on_readable();
    virtual void on_writable();
    
    //如果有外界容器管理this，或者this是栈分配的，则需要重载本函数
    virtual void safe_release()
    {
        delete this;
    }

    virtual void post_disconnected()
    {
        safe_release();
        // In case of  'this' is managed by outer, override this member
    }


    virtual PlainMessage * fetch_msg(); // 从buf中获得一个完整的msg，否则不动buf
    
    //如果想在其他线程处理消息，需要重载本函数
    virtual int process_then_free_msg( PlainMessage * msg); 

    //通常需要重载本函数
    virtual int really_process_msg(BaseJMessage* msg); 
    

    virtual void send_msg( int32_t seq, const Json::Value& jdata  );
    virtual void send_error_msg( int32_t seq, const char * error_msg );
    

    // 只用于libevent在conn上的回调
    virtual void send_error_msg_then_bye( int32_t seq, const char * error_msg );
    
    // 用于main thread
    virtual void send_error_msg_then_disconn( int32_t seq,  const char * error_msg );
    
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



#endif

