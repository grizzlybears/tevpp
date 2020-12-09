#ifndef  __INNER_PIPE_H__
#define  __INNER_PIPE_H__
#include <event2/thread.h>

#include "EventLoop.h"
#include "listeners.h"
#include "connections.h"
#include "thread_utils.h"

//#include "mysql_wrapper/mysql_wrapper.h"


class JobMessage
{
public:
    virtual ~JobMessage()
    {
    }

    virtual void do_in_main_thread()
    {
        // msg 自己负责释放
        delete this;
    }

    virtual void do_in_worker_thread()
    {
        // msg 自己负责释放
        delete this;
    }

    virtual CString dump_2_str() const
    {
        return "JobMessageBase";
    }
};

typedef SharedList< JobMessage*> JobQueue;

class InnerPipe
    :public BaseConnection
{
public:
     InnerPipe(SimpleEventLoop* loop)
         : BaseConnection(loop)
    { 
    }

    InnerPipe(SimpleEventLoop* loop, struct bufferevent  * the_bev )
         : BaseConnection(loop)
    {
        take_bev( the_bev  , EV_READ );

        int i = evbuffer_enable_locking( bufferevent_get_output( get_bev()) , NULL);
        if (i)
        {
            LOG_ERROR("Couldn't enable lock for command pipe.\n");
        }
    }

    virtual void on_readable();
    virtual void on_writable(){}

    
    //如果有外界容器管理this，或者this是栈分配的，则需要重载本函数。缺省实现是 delete this;
    virtual void post_disconnected()
    {
    }

    virtual JobMessage * fetch_msg(); // 从buf中获得一个完整的msg，否则不动buf
    
    
    virtual void post_msg(  JobMessage* msg );


};


class WorkerThread
    :public BaseThread
{
public:
     WorkerThread(SimpleEventLoop  * loop)
        :  main_loop(loop)
     {
     }
     
    virtual void* thread_main(); 
    JobQueue job_q;

    // MyConnection& get_db_conn()
    // {
    //     return my_conn;
    // }

protected:
    SimpleEventLoop  * main_loop;

    //int conn_to_db();
    ////数据库的操作有可能block，故不能放在主线程或者Keygoe回调线程，一律在worker处理
    //MyConnection my_conn;
 
};

/*
 * MsgSwith comes with a worker thread, a pair of 'inner pipe', and a 'job message q' .
 
 * Main thread append 'job message' to the Q, then worker thread gets it and exec the job on worker thread.
 
 * Worker thread post special message to 'wr head' of the innerpipe, 
 * then main thread fetches it via 'rd head' of inner pipe and does further processing.
 *
*/
class MsgSwitch
{
public: 
    MsgSwitch(SimpleEventLoop  * loop)
        : read_head(loop), write_head(loop), main_loop(loop), worker(loop)
    {
    }

    int  create_inner_pipe();
    void queue_to_main_thread( JobMessage* msg )
    {
        write_head.post_msg(msg);
    }
 
    void queue_to_worker_thread( JobMessage* msg )
    {
        worker.job_q.shared_append_one( msg);
    }

    void start_worker();
    void stop_worker();

protected:
    InnerPipe  read_head;
    InnerPipe  write_head;
    SimpleEventLoop  * main_loop;

    struct event_base * get_event_base()
    {
        return main_loop->get_event_base();
    }

public:
    WorkerThread  worker;

};

//////////////// thread pool , and MsgSwitch with thread poll /////
class WorkerThreadPool;

class PooledWorkerThread
    :public BaseThread
{
public:
      PooledWorkerThread( WorkerThreadPool * pool)
        :my_pool (pool)
     {
     }
     
    virtual void* thread_main(); 

protected:
    WorkerThreadPool * my_pool; 

    void register_to_pool();
    void unregister_from_poll();
};

class WorkerThreadPool
    :public SharedPtrMan< pthread_t ,PooledWorkerThread> 
{
public:
     WorkerThreadPool(SimpleEventLoop  * loop)
        :  main_loop(loop)
     {
     }
     
    JobQueue job_q; // all worker threads are equal :)
   
    void create_new_worker(int howmany);
    void dismiss_1_worker();
    void dismiss_all_workers();
    void wait_until_all_workers_gone();

protected:
    SimpleEventLoop  * main_loop;
};

/*
 * PooledMsgSwith comes with a worker thread pool, a pair of 'inner pipe', and a 'job message q' .
 
 * Main thread append 'job message' to the Q, then worker thread gets it and exec the job on worker thread.
 
 * Worker thread post special message to 'wr head' of the innerpipe, 
 * then main thread fetches it via 'rd head' of inner pipe and does further processing.
 *
*/
class PooledMsgSwitch
{
public: 
    PooledMsgSwitch(SimpleEventLoop  * loop)
        :main_loop(loop),read_head(loop), write_head(loop), workers(loop)
    {
    }

    virtual ~PooledMsgSwitch()
    {
    }

    int  create_inner_pipe(); // FIXME: if thers was no virtual func, dtor of 'read_head'/'write_head' won't get called.
                              //        if 'main_loop' was not 1st member var, 'create_inner_pipe()' in wt_demo2.cpp would crash
                              //        Why? @_@
                              //
    void queue_to_main_thread( JobMessage* msg )
    {
        write_head.post_msg(msg);
    }
 
    void queue_to_worker_thread( JobMessage* msg )
    {
        workers.job_q.shared_append_one( msg);
    }

protected:
    SimpleEventLoop  * main_loop;
    InnerPipe  read_head;
    InnerPipe  write_head;

    struct event_base * get_event_base()
    {
        return main_loop->get_event_base();
    }

public:
    WorkerThreadPool  workers;
};
#endif

