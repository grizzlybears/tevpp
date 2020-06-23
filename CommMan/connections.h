#ifndef  __CONNECTIONS_H__
#define  __CONNECTIONS_H__

#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>

#include "utils.h"
#include "EventLoop.h"
#include "thread_utils.h"
#include "timers.h"
#include <deque>

class BaseConnection
{
public:
    BaseConnection(SimpleEventLoop  * loop)
        :my_app(loop), bev(NULL)
    {
    }

    virtual ~BaseConnection()
    {
        //debug_printf("yes, dtor \n");
        release_bev();
    }

    struct event_base * get_event_base()
    {
        return my_app->get_event_base();
    }

    SimpleEventLoop  * get_app()
    {
        return my_app;
    }

    // attach 'this' to an existing fd
    virtual void take_socket(evutil_socket_t fd
            , short event_mask = EV_WRITE |  EV_READ
            , int   options =  BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE);

    // attach 'this' to an existing bufferevent
    virtual void take_bev(struct bufferevent    *bev
            , short event_mask = EV_WRITE |  EV_READ
            , int   options =  BEV_OPT_THREADSAFE );


    // connect 'this' to tcp addr:port
    virtual void connect_tcp(const char *hostname, int port, int   options =  BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE);

    virtual void post_disconnected()
    {
        delete this;
        // In case of  'this' is managed by outer, override this member
    }

    virtual void release_bev();

    virtual int queue_to_send( const void *data, size_t size);

    void send_str(const CString& s)
    {
        queue_to_send( (const void *)s.c_str(),  s.size());
    }

    static  void trampoline_readable(struct bufferevent *bev,  void *ctx);
    static  void trampoline_writable(struct bufferevent *bev,  void *ctx);
    static  void trampoline_event(struct bufferevent *bev, short what, void *ctx);

    virtual void on_readable() 
    {
    }

    virtual void on_writable()
    {
    }

    virtual void on_conn_event(short what);

    struct bufferevent * get_bev()
    {
        return bev;
    }

protected:
    SimpleEventLoop       *my_app;  // just ref, dont touch its life cycle.
    struct bufferevent    *bev;
};

class EvBufAutoLocker
{
public:
    struct evbuffer *buf; 
    EvBufAutoLocker(struct evbuffer * to_lock)
    {
        buf = to_lock;
        
        if (!buf)
        {
            return;
        }
        evbuffer_lock(buf);
    }

    virtual ~EvBufAutoLocker()
    { 
        if (!buf)
        {
            return;
        }
        evbuffer_unlock(buf);
        buf = NULL;
    }

};

class AddrInfo
{
public:
    char peer_ipstr[INET6_ADDRSTRLEN];
    int  peer_port;

    AddrInfo()
    {
        peer_port     = 0;
        peer_ipstr[0] = 0; 
    }

    void get_peer_info(int s);

    CString to_str() const;

};


#define MAKE_SURE_OUTGOING_DIAGRAM_CONTINUE  EvBufAutoLocker __make_sure_msg_conti(bufferevent_get_output(get_bev()))

class OuterPipeMan;
// 
class OuterPipe
    :public BaseConnection 
{
public:
    typedef BaseConnection  MyBase;

    OuterPipe(SimpleEventLoop  * loop)
        :BaseConnection(loop)
    { 
        managed = 0;
        pipe_id = 0;
        pipe_type = 0;
  
    }

    virtual ~OuterPipe()
    {
    } 
    
    int managed;     // 是否被CommandPan管理
    unsigned long  pipe_id;  // 在OuterPipeMan中唯一识别每个command pipe

    int  pipe_type;  // 一个app可以有多种外界通信管道，留个口子区分一下
                     // 这里没法做 enum，搞成摸版参数又太矫情了，留待导出类去#dfine常量吧
   
    int is_managed() const
    {
        return managed;
    }

    unsigned long get_id() const
    {
        return pipe_id;
    }
    
    // 接受外界管理
    void take_it(unsigned long new_id)
    {
        pipe_id = new_id;
        managed = 1;
    }

    
    virtual void release_self();
   
    virtual void post_disconnected() 
    {
        release_self();
    }


    virtual int is_alive()
    {
        return 1;
    }


    virtual CString dump_2_str() const = 0 ;

    // 获得管理所有管道的外界容器
    virtual OuterPipeMan * get_pipe_man() = 0;
};

class OuterPipeMan
    :public  SharedPtrMan<unsigned long /*ID*/ , OuterPipe>
     ,public TimerHandler
{
public:
    typedef SharedPtrMan<unsigned long /*ID*/ , OuterPipe>  MyBase;

    static  OuterPipeMan * singleton;
    OuterPipeMan(SimpleEventLoop  * loop);
    virtual ~OuterPipeMan();


    virtual void timer_cb();


    // 交由OuterPipeMan来管理
    int  register_connection(OuterPipe* client  );
    void unregister_connection(int pipe_id);
 
    
    CString dump_2_str();
    SimpleEventLoop* event_loop;

    unsigned long allocate_new_id();

    unsigned long next_pipe_id ;

};

// 此异常特殊，直达顶层，中间层的catch ( std::exception& ) 不能抓住
// 在处理报文过程中抛出本异常，就代表‘断开连接，不干了’ 
class ConnectionGoneException 
{
public:
    ConnectionGoneException(const std::string& str):_mess(str) {} 
    
    virtual ~ConnectionGoneException() throw () {}
    virtual const char* what() const throw () {return _mess.c_str();}

protected:
    CString  _mess;
};


#endif
