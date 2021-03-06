﻿#ifndef THREAD_UTILS_H_
#define THREAD_UTILS_H_

#include <stdio.h>
#include <assert.h>
#include <memory>
#include <vector>
#include <list>
#include <map>

#ifdef _WIN32 
	#include <windows.h>
    #include <winnt.h>    
    #include <process.h>
	#define STDCALL __stdcall
	typedef unsigned ThreadRetType;
#else
    #include <stdint.h>
    #include <pthread.h>
//    #define DWORD uint32_t
    #define __stdcall  
    #define INFINITE (0xFFFFFFFF)
	#include "utils.h"
	#define STDCALL __stdcall
	typedef void* ThreadRetType;
#endif


#define NOTHING_TO_WAIT (1)
class BaseThread
{
public: 
    BaseThread()
    { 
        _thread_handle = 0;
    }

    virtual ~BaseThread()
    {
    }

    static  ThreadRetType STDCALL trampoline( void *arg);
    virtual ThreadRetType thread_main();
	
	void create_thread();
	int  wait_thread_quit();
#ifdef __GNUC__
    pthread_t _thread_handle;
#else
	HANDLE _thread_handle;
	unsigned _thread_id;
#endif

};

typedef enum {
	DONT_WAKE_ANYONE  = 0,
	WAKE_ONE  ,
	WAKE_ALL  ,
} HOW_TO_WAKE;

#ifdef _WIN32
class AutoLocker
{
public:
	AutoLocker(CRITICAL_SECTION&  lock)
	{
		pLock = &lock;
		EnterCriticalSection(pLock);
	}

	AutoLocker(CRITICAL_SECTION*  lock)
	{
		pLock = lock;
		EnterCriticalSection(pLock);
	}

	~AutoLocker()
	{
		LeaveCriticalSection(pLock);
	}

protected:
	CRITICAL_SECTION* pLock;
};

class SimpleMutex
{
public:
	CRITICAL_SECTION _cs;
	SimpleMutex()
	{
		InitializeCriticalSection(& _cs);
	}

	virtual ~SimpleMutex()
	{
		DeleteCriticalSection(&_cs);
	}

	operator CRITICAL_SECTION * ()
	{
		return & _cs;
	}

	CRITICAL_SECTION&  get_lock()
	{
		return  _cs;
	}

	void lock()
	{
		EnterCriticalSection(& _cs);
	}

	void unlock()
	{
		LeaveCriticalSection(& _cs);
	}
};

class SharedFile
	:public SimpleMutex
{
public:
	FILE* _file;
	int   _auto_close;

	SharedFile(FILE* file = NULL, int auto_close = 1)
	{
		_file = file;
		_auto_close = auto_close;
	}

	virtual ~SharedFile()
	{
		if (_auto_close && _file)
		{
			fclose(_file);
			_file = NULL;
		}
	}

};

class SimpleConditionVar
	:public SimpleMutex
{
public:
	CONDITION_VARIABLE _cond;

	SimpleConditionVar()
	{
		InitializeConditionVariable (&_cond);
	}

	void wait()
	{
		SleepConditionVariableCS (&_cond, &_cs, INFINITE);
	}

	// return 0        -- wait success
	// return nonzero  -- wait timeout
	int timed_wait(unsigned int seconds)
	{
		BOOL b = SleepConditionVariableCS(&_cond, &_cs, seconds* 1000);
		if (b)
		{
			return 0;
		}
		assert( ERROR_TIMEOUT == GetLastError());
		return 1;
	}

	void wake( int how = WAKE_ONE)
	{
		if (WAKE_ONE == how)
		{
			WakeConditionVariable(&_cond);
		}
		else if (WAKE_ALL == how)
		{
			WakeAllConditionVariable(&_cond);
		}
	}


	
};

/*
An SRW lock is the size of a pointer. 
The advantage is that it is fast to update the lock state. 
The disadvantage is that very little state information can be stored, 
so SRW locks cannot be acquired recursively. 
In addition, a thread that owns an SRW lock in shared mode cannot upgrade its ownership of the lock to exclusive mode.
*/

class SimpleRwLock
{
public:
	SRWLOCK _lock;

	SimpleRwLock()
	{
		InitializeSRWLock(&_lock);
	}

	PSRWLOCK get_lock()
	{
		return &_lock;
	}
};

class AutoLockSrwForRead
{
public:
	PSRWLOCK _lock;

	AutoLockSrwForRead(PSRWLOCK lock)
	{
		_lock = lock;
		AcquireSRWLockShared (_lock);
	}

	~AutoLockSrwForRead()
	{
		ReleaseSRWLockShared (_lock);
	}
};

class AutoLockSrwForWrite
{
public:
	PSRWLOCK _lock;

	AutoLockSrwForWrite(PSRWLOCK lock)
	{
		_lock = lock;
		AcquireSRWLockExclusive (_lock);
	}

	~AutoLockSrwForWrite()
	{
		ReleaseSRWLockExclusive (_lock);
	}
};


#else
class AutoLocker
{
public:
	AutoLocker(pthread_mutex_t&  lock)
	{
		pLock = &lock;
        pthread_mutex_lock( pLock);

	}

	AutoLocker(pthread_mutex_t*  lock)
	{
		pLock = lock;
        pthread_mutex_lock( pLock);
	}

	~AutoLocker()
	{
        pthread_mutex_unlock(pLock);
	}

protected:
	pthread_mutex_t* pLock;
};


class SimpleMutex 
{
protected:
	pthread_mutex_t m_pthr_mutex;
public:
    SimpleMutex(int recur = 1)
    {
        if (!recur)
        {
            m_pthr_mutex =PTHREAD_MUTEX_INITIALIZER;
        }
        else
        {
            pthread_mutexattr_t Attr;
            pthread_mutexattr_init(&Attr);
            pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
           
            pthread_mutex_init(&m_pthr_mutex , &Attr);
        }
    
    }

	virtual ~SimpleMutex() 
    { 
        pthread_mutex_destroy(&m_pthr_mutex);
    }

    operator pthread_mutex_t * ()
    {
        return & m_pthr_mutex;
    }
    
    pthread_mutex_t& get_lock()
    {
        return  m_pthr_mutex;
    }

	void lock() 
    { 
         pthread_mutex_lock(&m_pthr_mutex);
    }
	
    void unlock() 
    { 
        pthread_mutex_unlock(&m_pthr_mutex);
    }
};

class SimpleConditionVar
	:public SimpleMutex
{
public:
    pthread_cond_t     _cond;

	SimpleConditionVar()
        :_cond(PTHREAD_COND_INITIALIZER)
	{
	}

    virtual ~SimpleConditionVar()
    {
        pthread_cond_destroy(& _cond); 
    }


	void wait()
	{
        int i = pthread_cond_wait(&_cond, &m_pthr_mutex); 
        if (i)
        {
            SIMPLE_LOG_LIBC_ERROR( "wait_cond", i );
        }
	}

    // return 0        -- wait success
    // return nonzero  -- wait timeout
    int timed_wait(unsigned int seconds)
	{
        struct timespec time_to_wait = {0, 0};
        time_to_wait.tv_sec = time(NULL) + seconds; 

        int i = pthread_cond_timedwait(&_cond, &m_pthr_mutex, &time_to_wait ); 
        if (ETIMEDOUT ==i)
        {
            return 1;
        }
        else if (i)
        {
            SIMPLE_LOG_LIBC_ERROR( "wait_cond", i );
            return 1;
        }

        return 0;
	}

	void wake( int how = WAKE_ONE)
	{
        int i;
		if (WAKE_ONE == how)
		{
            i = pthread_cond_signal(&_cond);
		}
		else if (WAKE_ALL == how)
		{
            i = pthread_cond_broadcast(&_cond);
		}
        
        if (i)
        {
            SIMPLE_LOG_LIBC_ERROR( "wake_cond", i );
        }

	}
	
};


#endif

template <typename T>
class SharedList
	: public std::list<T>
{
public:
	typedef std::list<T> MyBase;
	typedef typename MyBase::iterator  iterator;
	typedef typename MyBase::const_iterator  const_iterator;
	//typedef MyBase::reference  reference;
	//typedef MyBase::value_type  value_type;

	SimpleConditionVar  _condition_var;
	

	void shared_append_one(const T& v , int wake_type = WAKE_ONE)
	{
		AutoLocker _yes_locked(_condition_var);
		this->push_back(v);
		_condition_var.wake(wake_type);
	}

	void shared_append_many(MyBase& from , int wake_type = WAKE_ALL)
	{
		AutoLocker _yes_locked(_condition_var);
		slice( MyBase::end(),from);
		_condition_var.wake();
	}

	void shared_fetch_head( T& v )
	{
		AutoLocker _yes_locked(_condition_var);

		while( MyBase::empty())
		{
			_condition_var.wait();
		}

		v = MyBase::front();

		MyBase::pop_front();
	}

	void shared_fetch_all( MyBase& get_to_here )
	{
		AutoLocker _yes_locked(_condition_var);

		while( MyBase::empty())
		{
			_condition_var.wait();
		}

		get_to_here.splice(get_to_here.end(), *this);
	}
};


class OnOffSwitch
{
public:
	bool on_off;
	SimpleConditionVar _condition_var;

	OnOffSwitch(bool init_state= false)
	{
		on_off = init_state;
	}

	bool is_turned_on()
	{
		AutoLocker _yes_locked(_condition_var);
		return on_off ;
	}

	void turn_on()
	{
		AutoLocker _yes_locked(_condition_var);
		on_off = true;
		_condition_var.wake(WAKE_ALL);
	}
   

	void turn_off()
	{
		AutoLocker _yes_locked(_condition_var);
		on_off = false;
		_condition_var.wake(WAKE_ONE);
	}
	
	void wait_until_on()
	{
		AutoLocker _yes_locked(_condition_var);

		while( !on_off)
		{
			_condition_var.wait();
		}
	}
    

    // return 0        -- wait success
    // return nonzero  -- wait timeout
    int timed_wait_until_on(unsigned int seconds)
	{
		AutoLocker _yes_locked(_condition_var);

		while( !on_off)
		{
            int i = _condition_var.timed_wait(seconds);
            if (i)
            {
                return i;
            }
		}

        return 0;
	}

	void wait_then_turn_off()
	{
		AutoLocker _yes_locked(_condition_var);

		while( !on_off)
		{
			_condition_var.wait();
		}

		on_off = false;

	}

};

template<typename K, typename V>
class SharedPtrMan
	: public std::map<K, V*>
{
public:
    typedef std::map<K, V*> MyBase;
    SimpleConditionVar  lock;

    virtual ~SharedPtrMan()
    { 
    
        AutoLocker _yes_locked( lock );

        typename MyBase::iterator it;
        for (it = this->begin() ; it != this->end(); it++)
        {
            delete it->second;
        }
    }

	V* get_item(const K& which)
	{
		AutoLocker _yes_locked( lock );

        typename MyBase::iterator the_it;
		the_it= MyBase::find(which);

        if (MyBase::end() == the_it)
        {
            return NULL;
        }

        return the_it->second;
	}

    void remove_item(const K& which)
    {
        AutoLocker _yes_locked( lock );

        typename MyBase::iterator the_it;
		the_it= MyBase::find(which);

        if (MyBase::end() == the_it)
        {
            return;
        }

        delete the_it->second;

        this->erase(the_it);
    }
 
    int add_new_item(const K& which, V* what)
    {
        AutoLocker _yes_locked( lock ); 
        
        typename MyBase::iterator the_it;
		the_it= MyBase::find(which);

        if (MyBase::end() == the_it)
        {
            (*this)[which] = what;
            return 0 ;
        }
        
        // 同key的item已经存在
        return 1;
    }

    void add_or_replace_item(const K& which, V* what)
    {
        AutoLocker _yes_locked( lock ); 
        
        typename MyBase::iterator the_it;
		the_it= MyBase::find(which);

        if (MyBase::end() == the_it)
        {
            (*this)[which] = what;
            return;
        }
        
        delete the_it->second;
        the_it->second = what;
    }
};


#endif

