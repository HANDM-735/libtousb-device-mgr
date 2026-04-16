#ifndef MGR_LOG_H
#define MGR_LOG_H

#include <stdio.h>
#include <time.h>
#include <string>
#include "xbasic.hpp"
#include "xbasicmgr.hpp"
#include "queue.h"

#define MAX_FILE_NAME_LEN   255
#define MAX_LOG_MSG_LEN     4096
#define MAX_THREAD_WAIT_MS  100

#define LOGLEVEL_SYS        -1
#define LOGLEVEL_NONE       0
#define LOGLEVEL_ERROR      1
#define LOGLEVEL_WARN       2
#define LOGLEVEL_DEBUG      3

#define LOG_MODE_NORMAL     0
#define LOG_MODE_DEBUG      1
#define LOG_MODE_VERBOSE    2

#define SYS_LOG     LOGLEVEL_SYS
#define ERR_LOG     LOGLEVEL_ERROR
#define WRN_LOG     LOGLEVEL_WARN
#define MSG_LOG     LOGLEVEL_DEBUG

class mgr_log;

#define LOG_MSG(level, fmt, ...) \
do { \
    mgr_log* log_mgr = mgr_log::get_instance(); \
    if(log_mgr != NULL) { \
        log_mgr->write_log(level,fmt,##__VA_ARGS__); \
    } \
} while(0)

class log_msg
{
public:
    log_msg();
    log_msg(int loglevel,const std::string& logmsg,unsigned long thread_id,time_t time_stamp,long usec);
    log_msg(const log_msg& other);
    log_msg(log_msg&& other);
    log_msg& operator=(const log_msg& other);
    log_msg& operator=(const log_msg&& other);
    ~log_msg();

public:
    int             m_iloglevel;
    std::string     m_log_msg;
    unsigned long   m_thread_id;
    time_t          m_timestap; // 秒
    long            m_usec;     // 微秒
};

class mgr_log : public xmgr_basic<mgr_log>
{
public:
    mgr_log();
    virtual ~mgr_log();

public:
    int log_config(const char *logfilename,const char* logprefix,int loglevel,int logmode,int max_line);
    void write_log(int loglevel, const char* mgsfmt, ...);

    virtual void init();                    //初始化
    virtual void work(unsigned long ticket); //工作函数

private:
    void log_to_file(log_msg& msg);
    void log_backup();
    void formatmsg_byloglevel(int nLogLevel,const char * pszInput,char * szOutput);

    void set_log_level(int loglevel);
    void set_log_mode(int logmode);
    void set_max_line(int max);
    void set_file_name(const char* filename);
    void set_file_prefix(const char* prefix);

private:
    FILE*                       m_log_file;
    SafeQueue<log_msg>          m_log_queue;
    int                         m_log_num;
    int                         m_max_log_num;
    int                         m_log_level;
    int                         m_log_mode;
    int                         m_isFirst;
    char                        m_log_filename[MAX_FILE_NAME_LEN+1];
    char                        m_log_prefix[MAX_FILE_NAME_LEN+1];
};

#endif