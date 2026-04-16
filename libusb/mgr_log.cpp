#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include "mgr_log.h"

log_msg::log_msg()
{

}

log_msg::log_msg(int loglevel,const std::string& logmsg,unsigned long thread_id,time_t time_stamp,long usec) :
m_iloglevel(loglevel),m_log_msg(logmsg),m_thread_id(thread_id),m_timestap(time_stamp),m_usec(usec)
{

}

log_msg::~log_msg()
{

}

log_msg::log_msg(const log_msg& other)
{

}

log_msg& log_msg::operator=(const log_msg& other)
{
    if(this != &other) {
        this->m_iloglevel = other.m_iloglevel;
        this->m_log_msg = other.m_log_msg;
        this->m_thread_id = other.m_thread_id;
        this->m_timestap = other.m_timestap;
        this->m_usec = other.m_usec;
    }
    return *this;
}

log_msg::log_msg(log_msg&& other)
{
    if(this != & other) {
        std::swap(other.m_iloglevel,this->m_iloglevel);
        m_log_msg=std::move(other.m_log_msg);
        std::swap(other.m_thread_id,this->m_thread_id);
        std::swap(other.m_timestap,this->m_timestap);
        std::swap(other.m_usec,this->m_usec);
    }
}

log_msg::log_msg(const log_msg&& other)
{
    if(this != & other) {
        this->m_iloglevel = other.m_iloglevel;
        m_log_msg=std::move(other.m_log_msg);
        this->m_thread_id = other.m_thread_id;
        this->m_timestap = other.m_timestap;
        this->m_usec = other.m_usec;
    }
}

log_msg& log_msg::operator=(log_msg&& other)
{
    if(this != &other) {
        std::swap(other.m_iloglevel,this->m_iloglevel);
        m_log_msg = std::move(other.m_log_msg);
        std::swap(other.m_thread_id,this->m_thread_id);
        std::swap(other.m_timestap,this->m_timestap);
        std::swap(other.m_usec,this->m_usec);
    }
    return *this;
}

log_msg& log_msg::operator=(const log_msg&& other)
{
    if(this != &other) {
        this->m_iloglevel = other.m_iloglevel;
        m_log_msg = std::move(other.m_log_msg);
        this->m_thread_id = other.m_thread_id;
        this->m_timestap = other.m_timestap;
        this->m_usec = other.m_usec;
    }
    return *this;
}

mgr_log::mgr_log(): m_log_queue(0)
{
    m_log_file = NULL;
    m_log_level = LOGLEVEL_NONE;
    m_log_mode = LOG_MODE_NORMAL;
    m_log_num = 0;
    m_max_log_num = 0;
    m_isFirst = 1;
    memset(m_log_filename,'\0',sizeof(m_log_filename));
    memset(m_log_prefix,'\0',sizeof(m_log_prefix));
}

mgr_log::~mgr_log()
{
    if(m_log_file != NULL) {
        fclose(m_log_file);
        m_log_file = NULL;
    }
    m_log_queue.Exit();
}

void mgr_log::init()
{

}

void mgr_log::work(unsigned long ticket)
{
    log_msg msg;
    bool ret = m_log_queue.Pop_Wait(&msg,500);
    if(!ret) {
        //printf("failed to fetch message from log message queue len(%d)\n", m_log_queue.Size());
        return ;
    }

    log_to_file(msg);
    return ;
}

int mgr_log::log_config(const char *logfilename,const char* logprefix,int loglevel,int logmode,int max_line)
{
    if(logfilename == NULL) {
        printf("mgr_log::log_config() logfilename is NULL\n");
        return -1;
    }
    if(logprefix == NULL) {
        return -1;
    }
    set_file_name(logfilename);
    set_file_prefix(logprefix);
    set_log_level(loglevel);
    set_log_mode(logmode);
    set_max_line(max_line);

    return 0;
}

void mgr_log::write_log(int loglevel, const char* mgsfmt, ...)
{
    //检查程序启动时,检查是否需要备份旧的log
    if(m_isFirst) {
        //检查是否存在旧的log
        m_log_file = NULL;
        m_log_file = fopen(m_log_filename,"r");
        if(m_log_file != NULL) {
            fclose(m_log_file);
            m_log_file = NULL;
            log_backup();
        }
        m_isFirst = 0;
    }

    //检查log级别
    if(m_log_level < loglevel) {
        return ;
    }

    char cMsgBuffer[MAX_LOG_MSG_LEN + 1];
    char temp[MAX_LOG_MSG_LEN + 1];

    va_list vl;
    va_start(vl, mgsfmt);
    vsnprintf(temp,MAX_LOG_MSG_LEN-10, mgsfmt, vl );
    va_end(vl);

    formatmsg_byloglevel(loglevel,temp,cMsgBuffer);

    log_msg msgq;
    msgq.m_iloglevel = loglevel;
    msgq.m_log_msg = std::string(cMsgBuffer);
    msgq.m_thread_id = pthread_self();
    // msgq.m_timestap = time(NULL);

    struct timeval tv;
    gettimeofday(&tv,NULL);
    msgq.m_timestap = tv.tv_sec;
    msgq.m_usec = tv.tv_usec;

    if(!m_log_queue.Push(msgq)) {
        printf("mgr_log::write_log() push log message failed!\n");
        return ;
    }

    return ;
}

void mgr_log::log_to_file(log_msg& msg)
{
    //检查log级别
    if(m_log_level < msg.m_iloglevel) {
        return;
    }
    // if(LOG_MODE_DEBUG == m_log_mode || LOG_MODE_VERBOSE == m_log_mode) {
    //     printf("%s\n",msg.m_log_msg.c_str());
    // }
    struct tm mytime;
    localtime_r(&msg.m_timestap,&mytime);
    // struct timeval tv;
    // gettimeofday(&tv,NULL);

    char occur_time[30];
    sprintf(occur_time,"%4d-%02d-%02d %02d:%02d:%02d,%d",
    mytime.tm_year+1900,mytime.tm_mon+1,mytime.tm_mday,
    mytime.tm_hour,mytime.tm_min,mytime.tm_sec, msg.m_usec/1000);

    if( LOG_MODE_VERBOSE == m_log_mode ) {
        printf("At %s: %s.\n",occur_time,msg.m_log_msg.c_str());
        return ;
    }
    if(LOG_MODE_DEBUG == m_log_mode) {
        printf("At %s: %s.\n",occur_time,msg.m_log_msg.c_str());
    }

    //检查log文件行数,超过最大行数，备份log文件
    if(m_log_num > m_max_log_num) {
        log_backup();
    }
    //检查文件是否存在
    if(m_log_file != NULL && access(m_log_filename,F_OK) != 0) {
        //文件不存在
        fclose(m_log_file);
        m_log_file = NULL;
    }

    //打开log文件
    if(m_log_file == NULL) {
        m_log_file = fopen(m_log_filename,"a+");
        if(m_log_file == NULL) {
            return;
        }
    }

    //记录行号
    m_log_num++;

    //向log文件中写入log内容
    if(fprintf(m_log_file,"%d: At %s:(0x%lx) %s.\n",m_log_num,occur_time,msg.m_thread_id,msg.m_log_msg.c_str())<0)
    {
        printf("Error: Write log file failed,maybe disk is full!\n");
        fclose(m_log_file);
        m_log_file=NULL;
        return;
    }

    //将log内容刷新到磁盘
    if(fflush(m_log_file)!=0)
    {
        fclose(m_log_file);
        m_log_file=NULL;
    }
}

void mgr_log::log_backup()
{
    //得到当前时间戳
    time_t currt;
    struct tm curr_tm;

    time(&currt);
    localtime_r(&currt, &curr_tm);

    if(m_log_file != NULL) {
        fclose(m_log_file);
        m_log_file = NULL;
    }

    char occur_time[30];
    sprintf(occur_time,"%4d%02d%02d%02d%02d%02d",
        curr_tm.tm_year+1900,curr_tm.tm_mon+1,curr_tm.tm_mday,
        curr_tm.tm_hour,curr_tm.tm_min,curr_tm.tm_sec);

    //重命名旧的文件名
    char szNewName[MAX_FILE_NAME_LEN+1];
    sprintf(szNewName,"%s%s.log",m_log_prefix,occur_time);
    rename(m_log_filename,szNewName);
    //重置log num
    m_log_num = 0;

    return ;
}

void mgr_log::formatmsg_byloglevel(int nLogLevel,const char * pszInput,char * szOutput)
{
    szOutput[0] = '\0';
    switch(nLogLevel){
        case SYS_LOG:
            strcpy(szOutput,"Sys: ");
            break;
        case ERR_LOG:
            strcpy(szOutput,"Err: ");
            break;
        case WRN_LOG:
            strcpy(szOutput,"Wrn: ");
            break;
        default:
            strcpy(szOutput,"Msg: ");
            break;
    }
    strcat(szOutput,pszInput);
}

void mgr_log::set_log_level(int loglevel)
{
    m_log_level = loglevel;
    return ;
}

void mgr_log::set_log_mode(int logmode)
{
    m_log_mode = logmode;
    return ;
}

void mgr_log::set_max_line(int max)
{
    m_max_log_num = max;
    return ;
}

void mgr_log::set_file_name(const char* filename)
{
    if(filename == NULL){
        return ;
    }
    strncpy(m_log_filename,filename,strlen(filename));
    return ;
}

void mgr_log::set_file_prefix(const char* prefix)
{
    if(prefix == NULL){
        return ;
    }
    strncpy(m_log_prefix,prefix,strlen(prefix));
    return ;
}