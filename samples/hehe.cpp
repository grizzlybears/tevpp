/*
    this 'client part' of the sample 'hello'.

    It behaves like 'socat READLINE tcp:127.0.0.1:9995'.
*/
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <termios.h>

#include "EventLoop.h"
#include "listeners.h"
#include "signals.h"
#include "connections.h"

static const int PORT = 9995;

class HelloClientConnection
    :public BaseConnection
{
public:
    int serial;

    HelloClientConnection(SimpleEventLoop* loop, const char* host, int port )
         : BaseConnection(loop)
    { 
        serial = 0;
        connect_tcp( host, port);
    }

    HelloClientConnection(SimpleEventLoop* loop, const char* path)
         : BaseConnection(loop)
    {
        serial = 0;
        connect_generic( path);
    }
 
    virtual int pre_take_socket(evutil_socket_t fd)
    {
        if (!serial)
        {
            return 0;
        }

        LOG_INFO("configureing serail port...\n");
    
        struct termios tty;
        if(tcgetattr(fd, &tty) != 0)
        {
            LOG_WARN("Error %i from tcgetattr: %s\n", errno, strerror(errno));
            return 0;
        }

        // control mode
        tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity
        tty.c_cflag &= ~CSTOPB; // Clear stop field, aka. one stop bit

        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;     // 8 bits per byte

        tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
        tty.c_cflag |= CREAD | CLOCAL; // Turn on READ,  ignore ctrl lines (CLOCAL = 1) which adapts 'modems'

        // local mode
        tty.c_lflag &= ~ICANON;  // disable canonical mode

        tty.c_lflag &= ~ECHO; // Disable echo
        tty.c_lflag &= ~ECHOE; // Disable erasure
        tty.c_lflag &= ~ECHONL; // Disable new-line echo

        tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

        // input mode
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received

        // output modes
        tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
        tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

        // blocking and  timeout
        //
        //  When VMIN is 0, VTIME specifies a time-out from the start of the read() call. 
        //  When VMIN is > 0, VTIME specifies the time-out from the start of the first received character.

        tty.c_cc[VTIME] = tty.c_cc[VMIN] = 0; // No blocking

        // Baud Rate
        cfsetspeed(&tty, B9600);

        // Save tty settings
        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            LOG_WARN("Error %i from tcsetattr: %s\n", errno, strerror(errno));
            return 0;
        }
    
        serial = 1;
        return 0;
    }


    virtual void on_readable()
    {
        unsigned char buf[1024];
        int n;
        struct evbuffer *input = bufferevent_get_input(bev);
        while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {

            if (!serial)
            {
                fwrite(buf, 1, n, stdout);
            }
            else
            {
                CString line;
                int i;
                for (i = 0 ; i < n ; i++)
                {
                    if (0 == i)
                    {
                        line += CString("%02x", (int)buf[i]);
                    }
                    else
                    {
                        line += CString(" %02x", (int)buf[i]);
                    }
                }
                
                printf("%s\n", line.c_str());
            }

        }
    }
    
    virtual void post_disconnected()
    {
        event_base_loopexit( get_event_base(), NULL); 
        delete this;
    }
};

int main(int argc, char **argv)
{
    try
	{ 
        SimpleEventLoop loop;
        
        if (2==argc)
        { 
            LOG_DEBUG("We take %s\n", argv[1]);
            HelloClientConnection *hehe =  new HelloClientConnection (&loop, argv[1]);
            (void)hehe; 
        }
        else
        {
            HelloClientConnection *hehe =  new HelloClientConnection (&loop, "127.0.0.1", PORT);
           (void)hehe; 
        }

        //2. also we handle ctrl-C
        QuitSignalHandler control_c_handler(&loop);

        // the main loop
        loop.run();
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Exception in main(): %s\n", ex.what());
        return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception in main()\n");
        return 1;
	}
    return 0;
}

