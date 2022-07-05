Thinking libevent in C++
============================

Brief
----

'libevent' のお陰で、通信系の開発は大分らくになりました。
ところが、まだ多くの '*callbacks*' や '*void opaque*　*' などは存在し、そう素直になっていはいません。

通信系でも、「OO」(オブジェクト指向)で考えていたい！
例えば、どこかにtcp接続したい場合、このようなものがあれば幸いです：

```c++
class BaseConnection
{
public:
	void connect_tcp(const char *hostname, int port); 
	
	// 'void* opaque'付きのcallback functionsは要らない , C++ virual functionでいきます。
	virtual void on_conn_event(short what);  // 「繋いだ」あるいは「切断された」ときに、この'CB'は呼び出されます。
	virtual void on_readable();              // 受信データが到着したら、この'CB'は呼び出されます。
	
    virtual int queue_to_send( const void *data, size_t size);  //名前通り、「非同期送信」を行います。 
};
```

また、サーバ側なら、こういうのは欲しくなりますね：
```c++
class BaseListener
{
public:
    void start_listen_on_tcp(const char* addr);
    
    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen) = 0;  // implement this 'CB' to hadle incoming connections
};
```

よって、最低限のサーバの実現は百行以内で済みます。
```c++
#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 9995;

class HelloServerConnection  // our 'connection', when connected it says a hello msg to peer then disconnect.
    :public BaseConnection
{
public:
    HelloServerConnection(SimpleEventLoop* loop, evutil_socket_t fd )
         : BaseConnection(loop) {
        take_socket(fd, EV_WRITE );
    }

    virtual void on_writable()
    {
        struct evbuffer *output = bufferevent_get_output(bev);
        if (evbuffer_get_length(output) == 0) {
            printf("flushed answer\n");
            delete this;
        }
    }
};

class HelloWorldListener   // our real 'listener', creates our 'connection' instances to handle incoming connecting request.
    : public BaseListener 
{
public: 
     HelloWorldListener(SimpleEventLoop  * loop) 
        :BaseListener(loop ) {
    }

    virtual void listener_cb( evutil_socket_t fd, struct sockaddr *sa, int socklen) {
        HelloServerConnection* conn = new HelloServerConnection( my_app, fd );
        conn->queue_to_send( MESSAGE, strlen(MESSAGE));
    }
};

int main(int argc, char **argv){
    try	{ 
        SimpleEventLoop loop;
        //1. TCP listen on PORT
        HelloWorldListener tcp_listener( &loop);
        tcp_listener.start_listen_on_tcp( CString("0.0.0.0:%d", PORT).c_str() );
     
        //2. also we quit gracefully when ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        //3. the main loop
        loop.run();
    
        printf("done\n");
    }
    catch (std::exception& ex) {
    	LOG_ERROR("Exception in main(): %s\n", ex.what());
        return 1;
    }
    
    return 0;
}
```

もちろん、通常の通信系のフレームワークに、まだたくさんの‘道具’は必要です。

上記の 'BaseConnection' は、 'unix domain socket'、  'serial port'、 'generic file' そして 'stdio'まで、同じ方式で取り扱うべきですし、 'udp'  のラッパーと、  'timer'、 'signal handler'、  'http server wrapper'などいろいろもあるべきです。

もっと大事なのは、 実際の通信サーバには 'connection manager'が必要です。また、非同期方式でやっているため, 「ブッロク可能」な操作（例えば：DBオペレーション）は、event loop thread (’on_readable’のようなCBも含む)にはい一切使えません、よってworker-thread / thread-poolの仕組みも現実的に必須となります。それを応じて、’ジョブ’を ’loop thread’　と ’worker thread’の間に、自由に'marshal' する仕組みも必要となります。

上記のそれぞれを全部そろったら、なんと、「次世代通信系サーバのフレームワーク」は見えています！

Build dependences
-----------------
    (On Fedora) yum install gcc-c++ libstdc++-devel libevent-devel socat.

Build and run
-------------
    $ make             #build all samples
    
    $ make test_hello  #run the 'libevent hello-world sample' C++ ported.
    
    $ make test_cat    #run the 'cat' sample, which implement 'cat' in libevent style.
    
    $ make test_wt     #run the 'wt_demo' sample, which does some stuff in worker thread.
    
    $ make test_wt2    #run the 'wt_demo2' sample, which does some stuff in worker thread pool.
    
    $ make test_echo   #run the 'echo to all' sample  -- a chat server echoes any income msg to all cliets.
    
    $ make test_http_server  #run sample of httpserver wrapper. 



