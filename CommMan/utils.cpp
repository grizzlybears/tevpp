#include "utils.h"
#include <sys/types.h>

#ifdef __GNUC__
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#endif

#ifdef __GNUC__
CString get_exe_path()
{
    pid_t my_pid = getpid();

    CString path("/proc/%d/exe", (int)my_pid);

    char buf[PATH_MAX];

    ssize_t len;
    len = readlink( path.c_str(), buf, sizeof(buf)-1);
    if (-1 == len)
    {  
        throw SimpleException("readlink %s got code %d, %s"
                , path.c_str()
                , errno
                , strerror(errno)
                );
    }

    buf[len] = '\0';

    return buf;
}
#else
CString get_exe_path()
{
    char result[MAX_PATH];
    GetModuleFileNameA(NULL, result, MAX_PATH);
        
    return result;
}
#endif

CString get_exe_dir()
{
    return dir_from_file_path(get_exe_path().c_str());
}

//不以‘/’结尾
CString dir_from_file_path(const char * file_path)
{ 
    CString path;

    path = file_path;

    CString::size_type pos = path.find_last_of('/');
    path.erase( pos , path.size() - pos  );
    return path;
}

#ifdef __GNUC__
CString real_path(const char * path )
{
    char buf[PATH_MAX], *p;

    p= realpath(path, buf);
    if (!p)
    {  
        throw SimpleException("realpath '%s' got code %d, %s"
                , path
                , errno
                , strerror(errno)
                );
    }

    return buf;
}

// aka.  /bin/bash -c ${cmd_line} 
int shell_cmd_no_wait(const char * cmd_line)
{ 
    pid_t child = fork();
    if (child > 0 )
    {
        // I am parent.
        int status;
        waitpid( child, & status, 0);
        return 0;
    }
    else if (child < 0)
    {
        SIMPLE_LOG_LIBC_ERROR("First time spawn"  , errno);
        LOG_ERROR("cmd: %s\n", cmd_line);
        return 1;
    }

    // level 1 child

    pid_t child2 = fork();
    if (child2 > 0 )
    {
        // I am parent.
        exit(0);
        return 0;
    }
    else if (child < 0)
    {
        SIMPLE_LOG_LIBC_ERROR("2nd time  spawn"  , errno);
        LOG_ERROR("cmd: %s\n", cmd_line);
        return 1;
    }

    // level 2 child
    setsid();
    
    //debug_printf("%s\n", cmd_line);
    execlp("/bin/bash"
            , "/bin/bash"
            , "-c"
            , cmd_line
            , NULL
            );

    SIMPLE_LOG_LIBC_ERROR( "exec"  , errno);
    LOG_ERROR("cmd: %s\n", cmd_line);
    exit(-1);
}

int shell_cmd(const char * cmd_line, CString& output )
{
    FILE* f;
    if( ( f = popen( cmd_line, "r" ) ) == NULL ) {
        SIMPLE_LOG_LIBC_ERROR( cmd_line, errno );
        return -1;
    }

    output.clear();

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, f)) != -1) 
    {
        output += line;
    }

    if (line)
    {
        free(line);
        line = NULL;
    }

    int status = pclose(f);
    if (status == -1) {
        SIMPLE_LOG_LIBC_ERROR( "pclose", errno );
        return -2;
    }

    if (WIFEXITED(status))
    {
        int exit_code WEXITSTATUS(status);
        if ( 0 == exit_code)
        {
            return 0;
        }
        
        LOG_WARN("\"%s\" got exit_code %d\n%s\n", cmd_line, exit_code, output.c_str());
        return 1;
    }

    if (WIFSIGNALED(status))
    {
        int exit_signal  = WTERMSIG(status);
        LOG_WARN("\"%s\" got signal %d\n%s\n", cmd_line, exit_signal, output.c_str());
        return 2;
    }

    LOG_WARN("\"%s\" got status %d\n%s\n", cmd_line, status, output.c_str());
    return 3;
}

CString get_primary_mac()
{
    CString output;
    int r;
    r = shell_cmd("cat /sys/class/net/`ip -o -4 route show to default | awk '{print $5}'`/address 2>&1" , output );
    if (r)
    {
        return "";
    }

    std::vector<CString> lines;
    str_split(lines, output,  '\n');

    if (0 == lines.size())
    {
        LOG_ERROR("got null mac?output:\n%s\n", output.c_str());
        return "";
    }

    return  lines[0];
}


#endif

struct tm* date_add_ym(struct tm* date, int y, int m)
{
    date->tm_year += y;
    
    date->tm_year +=  m /12;

    date->tm_mon = m %12;

    if (date->tm_mon < 0 )
    {
        date->tm_mon += 12;
        date->tm_year --;
    }
    else if (date->tm_mon > 11 )
    { 
        date->tm_mon -= 12;
        date->tm_year ++;
    }

    int d = get_ym_days(date->tm_year , date->tm_mon);

    if (date->tm_mday > d)   // tm_mday   The day of the month, in the range 1 to 31.
    {
        date->tm_mday = d ;  // 4/30 - '2 monthes' => 2/28
    }

    // TODO: do we need tm_wday and tm_yday?

    return date;
}


int get_ym_days(int y, int m)
{
    int is_leap_year(int y);

    static int days_of_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    int r = days_of_month[m];
    if (1== m &&  is_leap_year(y))
    {
        r ++;
    }

    return r;
}

int is_leap_year(int y)
{
    if (y%4)
    {
        return 0;
    }

    if (! (y%100) )
    {
        return 1;
    }

    if (y%400)
    {
        return 1;
    }

    return 0;
}

CString hex_dump(const unsigned char * buf, int buf_len, int pad_lf )
{
    CString s;
    for (int i = 0; i < buf_len; i++)
    {
        if ( 0 == i )
        {
            s.format_append("%02x", (int)buf[i] );
        }
        else
        {
            s.format_append(" %02x", (int)buf[i] );
        }
    }

    if (pad_lf)
    {
        s += "\n";
    }

    return s;
}

