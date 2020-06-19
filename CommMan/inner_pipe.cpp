#include "inner_pipe.h"
//#include "config.h"
//#include "mysql_wrapper/mysql_wrapper.h"

JobMessage *  InnerPipe::fetch_msg()
{
    struct evbuffer *input = bufferevent_get_input(get_bev());
    size_t buffer_len = evbuffer_get_length( input );

    if (buffer_len < sizeof( JobMessage*))
    {
        return NULL;
    } 
    
    JobMessage * msg;
    evbuffer_remove(input , &msg, sizeof( JobMessage*));

    return msg;
}



void InnerPipe::post_msg(JobMessage* msg  )
{ 
    if (!bev)
    {
        return ;
    }

    //debug_printf("post a job  msg %p\n", msg);
    queue_to_send( &msg , sizeof(msg));
    //bufferevent_flush(bev, EV_WRITE ,BEV_FLUSH);
}


void  InnerPipe::on_readable()
{ 
    try
    { 
        JobMessage * msg;
        while ( (msg = fetch_msg() ) )
        {
            //debug_printf("got a job  msg %p\n", msg);
            try 
            {
                msg-> do_in_main_thread();
            } 
            catch (std::exception& ex)
            {
                LOG_ERROR("Exception while handle pipe command '%s': %s\n",msg->dump_2_str().c_str(),  ex.what());
            }

        }
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



int  MsgSwitch::create_inner_pipe()
{
    int i;
    struct bufferevent *pair[2];

    i =  bufferevent_pair_new( get_event_base() , BEV_OPT_THREADSAFE , pair);
    if (i)
    {
        LOG_ERROR("Failed ti create pipe pair.\n");
        return 1;
    }  
    
    read_head.take_bev( pair[0] );
    write_head.take_bev( pair[1] );

    return 0;
}

//此异常特殊，直达顶层，中间层的catch ( std::exception& ) 不能抓住
class QuitWorker 
{
public:
    QuitWorker(){}
    QuitWorker(const std::string& str):_mess(str) {} 
    
    virtual ~QuitWorker() throw () {}
    virtual const char* what() const throw () {return _mess.c_str();}

protected:
    CString  _mess;
};

// int WorkerThread::conn_to_db()
// {
//     if (my_conn.is_connected())
//     {
//         return 0;
//     }
//     
//     MonitorCenterApp * app = checked_cast(main_loop, (MonitorCenterApp *)NULL );
//  
//     if (app->app_config->my_host.empty())
//     {
//         LOG_WARN("Skip connecting to  db.\n");
//         return 1;
//     }
// 
//     LOG_DEBUG("Goto connect db.\n");
//     my_conn.real_connect(
//           app->app_config->my_host.c_str()
//         , app->app_config->my_uid.c_str()
//         , app->app_config->my_passwd.c_str()
//         , app->app_config->my_db.c_str()
//         , app->app_config->my_port
//         );
// 
//     LOG_INFO("DB conncected.\n");
//     return 0;
// }


void* WorkerThread::thread_main( )
{
    try
    {
        //MyThreadHelper __dummy();

        JobMessage* msg;
        while (1)
        {
            // try
            // {
            //     // 空闲时不断尝试重连DB
            //     conn_to_db();
            // }
            // catch (std::exception& ex)
            // {
            //     //底层已经记了log
            //     //continue;
            // }

            try
            {
                job_q.shared_fetch_head( msg);
                msg->do_in_worker_thread();
            }
            catch (std::exception& ex)
            {
                LOG_ERROR("Exception in worker while processing msg '%s' : %s\n", msg->dump_2_str().c_str(), ex.what());
            }

        }

    }
    catch(QuitWorker& qw )
    {
        debug_printf("Worker normally ended.\n");
    }
    catch (std::exception& ex)
	{
		LOG_ERROR("Exception in worker: %s\n", ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception in worker\n");
	}
    return NULL;
} 

class QuitWorkerMsg
    :public JobMessage
{
public:
    virtual void  do_in_worker_thread()
    {
        // msg 自己负责释放 
        AutoReleasePtr<  QuitWorkerMsg>  _dont_leak(this);
        throw QuitWorker();
    }

    virtual CString dump_2_str() const
    {
        return "QuitWorkerMsg";
    }
};

void MsgSwitch::start_worker()
{
    worker.create_thread();
}

void MsgSwitch::stop_worker()
{
    queue_to_worker_thread( new  QuitWorkerMsg());
    worker.wait_thread_quit();
}

