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

#ifdef _MSC_VER
#define DEFAULT_BEV_OPTION (BEV_OPT_CLOSE_ON_FREE)
#else
#define DEFAULT_BEV_OPTION (BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE)
#endif

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

    SimpleEventLoop  * get_app() const
    {
        return my_app;
    }

    // attach 'this' to an existing fd
    virtual void take_socket(evutil_socket_t fd
            , short event_mask = EV_WRITE |  EV_READ
            , int   options = DEFAULT_BEV_OPTION);

    virtual int pre_take_socket(evutil_socket_t fd)
    {
        return 0;
    }

    // attach 'this' to an existing bufferevent
    virtual void take_bev(struct bufferevent    *bev
            , short event_mask = EV_WRITE |  EV_READ
            , int   options =  BEV_OPT_THREADSAFE );


    // connect 'this' to tcp addr:port
    virtual void connect_tcp(const char *hostname, int port, int   options = DEFAULT_BEV_OPTION);
    virtual void connect_tcp2(const char* host_port, int   options = DEFAULT_BEV_OPTION);

#ifdef __GNUC__ 
    // connect 'this' to unix domain socket at 'path' 
    virtual void connect_unix(const char *path, int   options =  BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE); 

    // connect 'this' to generic filesystem path, i.e. '/dev/ttyS0'
    virtual int connect_generic(const char* path, int   options = DEFAULT_BEV_OPTION);

#endif

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

class BaseDiagram
{
public:
     BaseDiagram(SimpleEventLoop  * loop)
        :my_app(loop), the_event(NULL),sock_fd(-1)
    {
    }
 
    struct event_base * get_event_base()
    {
        return my_app->get_event_base();
    }

    SimpleEventLoop  * get_app()
    {
        return my_app;
    }


    virtual ~BaseDiagram()
    {
        //debug_printf("yes, dtor \n");
        release_ev();
    }

    // bind 'this' to udp addr:port
    virtual void bind_udp(const char *addr, int port);
    virtual void bind_udp2(const char* addr_port);

    void release_ev();
 
    static void trampoline_event_cb(evutil_socket_t fd, short events, void * arg);
 
    virtual void on_readable() 
    {
    }

protected:
    SimpleEventLoop       *my_app;  // just ref, dont touch its life cycle.
    struct event * the_event;
    evutil_socket_t  sock_fd;

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

    void load_from_addr(const struct sockaddr_in* peer_addr );

    CString to_str() const;

};


#define MAKE_SURE_OUTGOING_DIAGRAM_CONTINUE  EvBufAutoLocker __make_sure_msg_conti(bufferevent_get_output(get_bev()))

struct PipePara{
    unsigned long pipe_id;
    int32_t       para1;  // usually here can be 'S1'  , ref following section
    int64_t       para2; 
    CString       para3;
    // 此处再放一个shared_ptr可以任意扩展，直接把指针塞在para不好，释放的时机不易把握，但一般没这么复杂的需求
 
    PipePara()
        :pipe_id(0), para1(0),para2(0)
    {
    }

    PipePara(unsigned long id)
        :pipe_id(id), para1(0),para2(0)
    {
    }

};

// 收到'sesion token'的报文，转交'PipePara'的管道处理
// 适用场景:
//      管道A 收到 问询报文x,  seq_no =  S1 (由问询方发号)
//      管道A 向 管道B 转发问询报文x ,seq_no =  S2 (由本进程发号)
//      管道B 上收到问询报文x的回信y
//      管道B 需要把y转交管道A 处理
typedef Map2<int32_t /*S2*/,  PipePara>        PipeLineTable;   // 上例中 B 需要的转发跳转表
typedef Map2<int32_t /*S2*/, int /*dummy*/ >   PendingTokens;   // 上例中 A 需要的'等待中token'列表


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

    PipeLineTable pipe_line;
    PendingTokens pending_tokens;

    void reg_pipe_line(int32_t token, unsigned long pipe_id)
    { 
        PipePara pipe(pipe_id);
        pipe_line[token] = pipe;
    }
    
    void reg_pipe_line(int32_t token, const PipePara& pipe)
    { 
        pipe_line[token] = pipe;
    }
    
    void unreg_pipe_line(int32_t token)
    { 
        pipe_line.safe_remove(token);
    }

    // 返回 nonzero，表示找到了接力的管道id。
    // 返回 0 , 表述无接力管道。
    int find_dest_pipe(int32_t token, PipePara& pipe_para)
    {        
        PipeLineTable::iterator  it;
        bool b = pipe_line.has_key2( token, it);
        if (!b)
        {
           return -1;
        }

        pipe_para = it->second;

        pipe_line.erase(it);
        return 1;
    }

    void wait_4_response(int32_t token)
    { 
        pending_tokens[token] = 0;
    }

    // 返回non-zero  表示确实在等这个token的回信
    // 返回0 表示没有在等
    int unwait_if_waiting(int32_t token)
    {
        PendingTokens::iterator it;
	    bool b =  pending_tokens.has_key2(token, it);
        if (b)
        {
            pending_tokens.erase(it);
            return 1;
        }
        return 0;
    }

   
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
    virtual int  register_connection(OuterPipe* client  );
    virtual void unregister_connection(int pipe_id);
 
    
    CString dump_2_str() const;
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
