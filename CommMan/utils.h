﻿#ifndef UTILS_H_
#define UTILS_H_

#include "CStringWrapper.h"
#include <assert.h>

#include <memory>
#include <vector>
#include <list>
#include <map>

#ifdef _MSC_VER
#define LIKE_PRINTF23 
#define localtime_r(a,b) localtime_s(b,a)
#include <malloc.h>
#else
#define LIKE_PRINTF23 __attribute__((format(printf,2,3)))	    
#endif


#define LOG( format, ...) do { \
        time_t tnow ; \
        struct tm  __now ; \
        tnow = time(NULL); \
        localtime_r(&tnow, &__now ); \
        CAtlStringA the_message; \
		the_message.Format(format, ## __VA_ARGS__); \
        fprintf(stderr,"[%d-%d-%d %02d:%02d:%02d] %s" \
                , __now.tm_year+1900 \
                , __now.tm_mon +1    \
                , __now.tm_mday      \
                , __now.tm_hour     \
                , __now.tm_min      \
                , __now.tm_sec      \
                , the_message.GetString() \
               );   \
	    } while(0)

#define LOG_DEBUG(format, ...) LOG( "Debug|%s:(%d)|" format, __FUNCTION__ , __LINE__, ## __VA_ARGS__)
#define LOG_INFO(format, ...)  LOG( "Info |%s:(%d)|" format, __FUNCTION__ , __LINE__, ## __VA_ARGS__)
#define LOG_WARN(format, ...)  LOG( "Warn |%s:(%s:%d)|" format, __FUNCTION__ , __FILE__, __LINE__, ## __VA_ARGS__)
#define LOG_ERROR(format, ...) LOG( "Error|%s:(%s:%d)|" format, __FUNCTION__ , __FILE__, __LINE__, ## __VA_ARGS__)


#define SIMPLE_LOG_LIBC_ERROR( where, what ) \
   do {   \
	    int code = what; \
		char* msg = strerror(code ); \
	    LOG_ERROR( "Failed  while %s, errno = %d, %s\n", where , code, msg ); \
   } while (0)


class SimpleException: public std::exception 
{
public:
    SimpleException(const CAtlStringA& str):_mess(str) {}
    SimpleException(const std::string& str):_mess(str) {}
    
    explicit SimpleException(const char * fmt, ...) /* __attribute__((format(printf,2,3))) */
    {
        va_list  args;
        va_start(args, fmt);

        _mess.FormatV(fmt, args);
        va_end(args);
    }

    virtual ~SimpleException() throw () {}
    virtual const char* what() const throw () {return _mess.c_str();}

protected:
    CAtlStringA  _mess;
};

#define LOG_THEN_THROW(format, ...) \
    do {\
        LOG_ERROR(format, ## __VA_ARGS__ );\
        throw SimpleException(format, ## __VA_ARGS__ ); \
    } while(0)


template <typename Drived, typename Base>
Drived * checked_cast(Base * b, Drived* d)
{
	Drived * p = dynamic_cast<Drived *> (b);
	if (!p)
	{
		throw SimpleException("Bad cast!");
	}

	return p;
}

template<typename K, typename V>
class Map2
	: public std::map<K, V>
{
public:
    typedef std::map<K, V> MyBase;

	bool has_key(const K& which)const
	{
        typename MyBase::const_iterator it= MyBase::find(which);
		return ( MyBase::end() != it);
	}

	bool has_key2(const K& which,typename MyBase::iterator& the_it)
	{
		the_it= MyBase::find(which);
		return ( MyBase::end() != the_it);
	}

    V* just_get_ptr(const K& which)
    {
        typename MyBase::iterator it= MyBase::find(which);
        if (MyBase::end() == it)
        {
            return NULL;
        }

        return & it->second;

    }
};


//使用 insert_if_not_present 来避免插入重复元素
template<typename K, typename V>
class Multimap2
	: public std::multimap<K, V>
{
public:
	typedef std::multimap<K, V> MyBase;

	typename MyBase::const_iterator find_pair( const typename MyBase::value_type& pair) const
	{
		std::pair <typename MyBase::const_iterator, typename MyBase::const_iterator> gocha;
		gocha = equal_range(pair.first);
		for (typename MyBase::const_iterator it = gocha.first; it != gocha.second; ++it)
		{
			if (it->second == pair.second)
			    return it;
		}

		return typename MyBase::end();
	}

	bool insert_if_not_present( const typename MyBase::value_type& pair)
	{
		if (find_pair( pair) == typename MyBase::end()) {
			insert(pair);
			return true;
		}

		return false;
	}

	void erase_by_value(const V& v)
	{

		while (1)
		{
			typename MyBase::iterator it;
			for (it = MyBase::begin(); it != MyBase::end() ; it++)
			{
				if ( it->second == v )
				{
					break;
				}
			}

			if (MyBase::end() == it)
			{
				// not found
				return;
			}

			erase(it);
		}
		
	}


};
#define safe_strcpy(d , s) strncpy( (d) , (s) , sizeof(d) - 1 )

template <typename T>
class AutoReleasePtr
{
public:
	T * me;

	AutoReleasePtr( T * p = NULL)
	{
		me =p;
	}

private:
	AutoReleasePtr (const AutoReleasePtr& a )
	{
		throw "Hey! AutoReleasePtr is NOT boost::shared_ptr!\n ";
	}
public:
	AutoReleasePtr&  operator= ( T* p)
	//void assign (T* p)
	{
		if (p == me)
		{
			return *this;
		}

		release();
		me = p;

		return *this;
	}
	

	operator T* () const
	{
		return me;
	}

	T* operator->() const 
	{
		return me;
	}

	virtual ~AutoReleasePtr()
	{
		release();	
	}

	virtual void release()
	{
		if (!me)
			return;

		delete me;
		me =NULL;
	}
	
};

class AutoCloseFD
{
public:   
	int  me;
    AutoCloseFD( int fd)
	{
		me = fd;
	}

private:
	AutoCloseFD (const AutoCloseFD& a )
	{
		throw "Hey! AutoCloseFD is NOT boost::shared_ptr!\n ";
	}
public:
    ~AutoCloseFD()
	{
		close(me);
	}
};

class AutoFree
{
public:   
	void*  me;
     AutoFree( void* p)
	{
		me = p;
	}

    ~AutoFree()
	{
		if (me)
        {
            free(me);
        }
	}
private:
    AutoFree (const AutoFree& a )
	{
		throw "Hey! AutoFree is NOT boost::shared_ptr!\n ";
	}

};

class AutoFClose
{
public:   
	FILE*  me;
    AutoFClose( FILE* fp)
	{
		me = fp;
	}

private:
	AutoFClose (const AutoFClose& a )
	{
		throw "Hey! AutoFClose is NOT boost::shared_ptr!\n ";
	}
public:
    ~AutoFClose()
	{
		if( NULL != me )
			fclose(me);
	}
};

template <typename T>
const T* is_null( const T* x , const T* y )  
{
	return x ?  x : y ;
}


////////////////////////////////////////////////////////////////////////
/////    debug util setcion
////////////////////////////////////////////////////////////////////////

#define _CONSOLE_RESET_COLOR "\e[m"
#define _CONSOLE_MAKE_RED "\e[31m"
#define _CONSOLE_MAKE_GREEN "\e[32m"
#define _CONSOLE_MAKE_YELLOW "\e[33m"
#define _CONSOLE_BLACK_BOLD  "\e[30;1m"
#define _CONSOLE_BLUE        "\e[34m"
#define _CONSOLE_MAGENTA     "\e[35m"
#define _CONSOLE_CYAN        "\e[36m"
#define _CONSOLE_WHITE_BOLD  "\e[37;1m"

// to add more color effect,
//ref:
//  http://stackoverflow.com/questions/3506504/c-code-changes-terminal-text-color-how-to-restore-defaults-linux
//  http://www.linuxselfhelp.com/howtos/Bash-Prompt/Bash-Prompt-HOWTO-6.html
//  http://bluesock.org/~willg/dev/ansi.html

#ifndef NDEBUG
	#define debug_printf(format, ... )  \
    do { \
        int colorful_fp = isatty ( fileno(stderr));\
        fprintf( stderr, "%s%s(%d) %s (t=%lu):%s " format \
                , colorful_fp? _CONSOLE_MAKE_GREEN:""\
                ,  __FILE__, __LINE__, __func__, (unsigned long)pthread_self()  \
                ,  colorful_fp? _CONSOLE_RESET_COLOR:"" \
                ,  ##__VA_ARGS__ );\
    } while(0)

    #define debug_printf_yellow(format, ... )  \
    do { \
        int colorful_fp = isatty ( fileno(stderr));\
        fprintf( stderr, "%s(%d) %s%s: " format \
                ,  __FILE__, __LINE__, __func__ \
                , colorful_fp? _CONSOLE_MAKE_YELLOW:""\
                ,  ##__VA_ARGS__ );\
         if (colorful_fp) { fprintf( stderr, "%s", _CONSOLE_RESET_COLOR ); } \
    } while(0) 

class _PlainFuncTracer
{
public:
    std::string _func;
    _PlainFuncTracer(const char* func)
    {
        _func = func;
        debug_printf("======= enter %s =======\n", _func.c_str() );
    }
    
    ~_PlainFuncTracer()
    {
        debug_printf("======= leave %s ========\n\n", _func.c_str() );
    }
    
};
    #define PlainFuncTracePlease _PlainFuncTracer __dont_use_this_name( __FUNCTION__ );

#else 
	#define debug_printf(format, ...)
    #define debug_printf_yellow(format, ...)
    #define PlainFuncTracePlease 
#endif // DEBUG_TRACE


#define MakeInt64(a,b) ( (((uint64_t)(uint32_t)(a)) <<32)  | (uint32_t)b  )
#define LowInt32(a)    (  uint32_t ( (a) & 0x00000000FFFFFFFFUL)  )
#define HighInt32(a)   (  uint32_t ( ((a) & 0xFFFFFFFF00000000UL) >> 32  )  )

#define MakeInt32(a,b) ( (((uint32_t)(uint16_t)(a)) << 16)  | (uint16_t)b  )
#define LowInt16(a)    (  (uint32_t)  ( (a) & 0x0000FFFFU)  )
#define HighInt16(a)   (  (uint32_t)  ( ((a) & 0xFFFF0000U) >> 16 ))

#define TurnOnBitHigh16(who, which_bit) ( (who) |=    ( (uint32_t)( which_bit)) << 16 )
#define TurnOffBitHigh16(who, which_bit) ( (who) &=  ~( ((uint32_t)( which_bit)) << 16 ))


int32_t inline  parse_int(const char* buf, size_t buf_size)
{
    char buf2[buf_size + 1];
    buf2[buf_size]= 0;

    memcpy((void*)buf2,(void*)buf, buf_size);

    return atoi(buf2);
}
#define PARSE_INT(a) parse_int( a, sizeof(a))

#endif
