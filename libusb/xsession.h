#ifndef X_SESSION_H
#define X_SESSION_H

#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <list>
#include <map>
#include <functional>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "xbasic.hpp"
#include "xbasicmgr.hpp"
#include "xpackage.hpp"
#include "xconfig.hpp"
#include "usbadapter_package.h"
#include "mgr_log.h"

// 库信号量封装
class xlibsemaphore
{
public:
    xlibsemaphore(int id);
    void post();                     // 触发信号量
    void wait();                     // 阻塞等待信号量
    bool try_wait();                 // 非阻塞判断信号量是否触发
    bool timed_wait(int milliseconds); // 等待信号量或超时

public:
    int                                     m_id;           // 信号量ID
    int                                     m_errcode;      // 错误码
    std::string                             m_att_data;     // 附带数据
    xusbadapter_package::rw_data             m_rw_data;      // 读写数据
    std::set<int>                           m_ids;          // 多点数据
    std::vector<xusbadapter_package::junction_temp> m_juncts; // 附带数据

protected:
    boost::interprocess::interprocess_semaphore m_sync_semaph; // 同步信号量
};

// 请求消息封装
class xrequest
{
public:
    xrequest(const char *port_name, xusbadapter_package *pack);

public:
    std::string                         m_port_name;    // 请求的端口
    boost::shared_ptr<xusbadapter_package> m_pack;     // 请求的包内容
};

// 会话管理类，继承自消息发送基类
class xsession : public xtransmitter
{
public:
    xsession(int session_id);
    ~xsession();

public:
    // 消息通知虚接口
    virtual int on_recv(const char *port_name, xusbadapter_package *pack);

public:
    std::string get_port_name();
    int get_session_id();

    // OTA升级相关接口
    int call_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name="");
    int call_ota_query_upgrade(int ota_type,int usb_device_addr_id,int &errcode,std::string &result);
    int call_ota_cancel_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result);
    int call_ota_complete_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result);

    // 文件读取接口
    int call_cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name);

    // USB设备相关接口
    int call_usb_serials_ids(std::set<int>& ids,int &errcode);
    int call_asic_junctions_temp(std::vector<xusbadapter_package::junction_temp>& juncts_temp,int &errcode);

    // 设备参数读写接口
    int call_get_value_from_dev(int dstid, int type, int& errcode, std::string& value);
    int call_powercontrol_req(int dstid,int type,int sw_val,int& errcode,int utp40_id=0);
    int call_set_frequency(int dstid,int type,float fre_val,int& errcode,int channel=0);

    // UTP寄存器读取接口
    int call_get_one_utp_register(int dstid,int chip_id,int& errcode,std::string& value);
    int call_get_one_utp102_register(int dstid,int chip_id,int& errcode,std::string& value);

protected:
    // 各类响应消息处理函数
    void handle_start_response(xusbadapter_package *pack);
    void handle_query_response(xusbadapter_package *pack);
    void handle_cancle_response(xusbadapter_package *pack);
    void handle_data_response(xusbadapter_package *pack);
    void handle_cal_read_response(xusbadapter_package *pack);
    void handle_usbserialids_response(xusbadapter_package *pack);
    void handle_asicjunctions_response(xusbadapter_package *pack);
    void handle_getvalue_response(xusbadapter_package *pack);
    void handle_powercontrol_response(xusbadapter_package *pack);
    void handle_frequencyset_response(xusbadapter_package *pack);
    void handle_get_utp_register_response(xusbadapter_package *pack);
    void handle_get_utp102_register_response(xusbadapter_package *pack);

private:
    bool is_ota_caltype(int type);

private:
    int                 m_sess_id;      // 会话id
    std::string         m_port_name;    // 会话端口

protected:
    std::list<boost::shared_ptr<xrequest>>                   m_lst_request;    // 请求消息集合
    boost::shared_mutex                                     m_mux_request;    // 请求消息集合读写锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore>> m_map_semaphore; // 信号量集合
    boost::shared_mutex                                     m_mux_semaphore;  // 信号量集合读写锁
};

#endif