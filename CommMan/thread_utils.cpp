#include "thread_utils.h"

void BaseThread::create_thread()
{
    int ret;
    ret = pthread_create(&_thread_handle, NULL,  trampoline, this); 
	if ( ret )
	{
		throw SimpleException(" pthread_create failed, errno = %d ", errno);
	}
}

void* BaseThread::trampoline( void *arg)
{
    BaseThread * real_one = ( BaseThread*)arg;
    return real_one->thread_main();
}

void* BaseThread::thread_main( )
{
    debug_printf("nothing to do.\n");
    return NULL;
}

int BaseThread::wait_thread_quit()
{
	if (!_thread_handle)
	{
		return NOTHING_TO_WAIT;
	}

	int i = pthread_join(_thread_handle, NULL);
    if (i)
    {
        SIMPLE_LOG_LIBC_ERROR( "join worder thread",   errno);
    }
    _thread_handle = 0;

    return i;
}

