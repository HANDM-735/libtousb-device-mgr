#ifndef MGR_SESSION_H
#define MGR_SESSION_H
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include "xbasic.hpp"
#include "xbasicmgr.hpp"
#include "xconfig.hpp"
#include "xsession.h"
#include "mgr_network.h"
#include "mgr_log.h"

class mgr_session : public xmgr_basic<mgr_session> //会话管理器
{
public:
    mgr_session() ;
    ~mgr_session();

public:
    virtual void init();                    //初始化
    virtual void work(unsigned long ticket); //工作函数

public:
    int call_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name="");
    int call_ota_cancel_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result);
    int call_ota_query_upgrade(int ota_type,int usb_device_addr_id,int &errcode,std::string &result);
    int call_ota_complete_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result);
    int usb_fetch_real_data(int usb_device_addr_id,int &errcode,boost::shared_ptr<xusbadapter_package::rw_data>& data);
    int call_cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name="");
    int call_usb_getserials_ids(std::set<int>& usbdstids,int &errcode);
    int call_asic_juictions_temp(std::vector<xusbadapter_package::junction_temp> &juncts_temp,int &errcode);
    int call_get_value_from_dev(int dstid, int type, int& errcode, std::string& value);
    int call_powercontrol_req(int dstid,int type,int sw_val,int& errcode,int utp40_id=0);
    int call_set_frequency(int dstid,int type,float fre_val,int& errcode,int channel=0);
    int call_get_one_utp_register(int dstid,int chip_id,int& errcode,std::string& value);
    int call_get_one_utp102_register(int dstid,int chip_id,int& errcode,std::string& value);

protected:
    virtual int on_network_recv(mgr_network *network_mgr,mgr_network::CBKDEVMSG msg_type,const char *port_name,xpacket *packet); //网络接收通知
    virtual int on_network_send(const char *port_name,xpacket *packet); //代理消息发送接口

protected:
    xconfig                         *m_sys_config;      //系统配置
    mgr_network                     *m_network_mgr;    //网络管理器

private:
    boost::shared_ptr<xsession>      m_lib_session;      //库会话
};

#endif