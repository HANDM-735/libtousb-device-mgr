#include "mgr_session.h"
#include <math.h>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/thread.hpp>

mgr_session::mgr_session()
{

}

mgr_session::~mgr_session()
{

}

void mgr_session::init() //初始化
{
    const char *c_filt_port[] = {PORT_ADAPTER};
    for(int i=0; i<sizeof(c_filt_port)/sizeof(char *); i++) this->add_filter(c_filt_port[i]); //设置网络消息筛选端口
    m_sys_config = xconfig::get_instance();
    m_network_mgr = mgr_network::get_instance();

    m_lib_session.Reset(new xsession()); //创建本体会话
    m_network_mgr->set_proxy_send(std::bind(&mgr_session::on_network_send, this, std::placeholders::_1, std::placeholders::_2)); //设置代理发送函数
    m_network_mgr->set_callback(std::bind(&mgr_session::on_network_recv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)); //设置设备管理器的回调函数
}

void mgr_session::work(unsigned long ticket) //工作函数
{
    //to do
}

int mgr_session::on_network_recv(mgr_network *network_mgr,mgr_network::CBKDEVMSG msg_type,const char *port_name,xpacket *packet) //网络接收通知
{
    // xbasic::debug_output("Enter into mgr_session::on_network_recv().\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::on_network_recv()");
    xusbadapter_package *pack = dynamic_cast<xusbadapter_package *>(packet);
    if (msg_type == mgr_network::DEV_DATA) //网络拿来数据
    {
        m_lib_session->on_recv(port_name, pack); //回调到指定会话
    }
    else if (msg_type == mgr_network::DEV_CONNECTED) //网络连接连接
    {
        //
    }
    // xbasic::debug_output("Exited mgr_session::on_network_recv().\n");
    LOG_MSG(MSG_LOG, "Exited mgr_session::on_network_recv()");
    return 0;
}

int mgr_session::on_network_send(const char *port_name,xpacket *packet) //代理消息发送接口
{
    // xbasic::debug_output("Enter into mgr_session::on_network_send()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::on_network_send()");
    xusbadapter_package *pack = dynamic_cast<xusbadapter_package *>(packet);
    int ret = m_network_mgr->send_to_adapter(pack);

    // xbasic::debug_output("Exited mgr_session::on_network_send() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::on_network_send() ret=%d",ret);
    return ret;
}

int mgr_session::call_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name)
{
    // xbasic::debug_output("Enter into mgr_session::call_ota_start_upgrade()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_ota_start_upgrade() ota_type:%d, version_num:%s, board_id:0x%x, file_name:%s",ota_type, version_num, usb_device_addr_id, file_name.c_str());
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_ota_start_upgrade(ota_type,version_num,usb_device_addr_id,errcode,result,file_name);
    // xbasic::debug_output("Exited mgr_session::call_ota_start_upgrade() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_ota_start_upgrade() ret=%d",ret);
    return ret;
}

int mgr_session::call_ota_cancel_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into mgr_session::call_ota_cancel_upgrade()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_ota_cancel_upgrade()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_ota_cancel_upgrade(ota_type,usb_device_addr_id,errcode,result);
    // xbasic::debug_output("Exited mgr_session::call_ota_cancel_upgrade() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_ota_cancel_upgrade() ret=%d",ret);
    return ret;
}

int mgr_session::call_ota_query_upgrade(int ota_type,int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into mgr_session::call_ota_query_upgrade()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_ota_query_upgrade()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_ota_query_upgrade(ota_type,usb_device_addr_id,errcode,result);
    // xbasic::debug_output("Exited mgr_session::call_ota_query_upgrade() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_ota_query_upgrade() ret=%d",ret);
    return ret;
}

int mgr_session::call_ota_complete_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into mgr_session::call_ota_complete_upgrade()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_ota_complete_upgrade()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_ota_complete_upgrade(ota_type,usb_device_addr_id,errcode,result);
    // xbasic::debug_output("Exited mgr_session::call_ota_complete_upgrade() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_ota_complete_upgrade() ret=%d",ret);
    return ret;
}

int mgr_session::usb_fetch_real_data(int usb_device_addr_id,int &errcode,boost::shared_ptr<xusbadapter_package::rw_data>& data)
{
    // xbasic::debug_output("Enter into mgr_session::usb_fetch_real_data()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::usb_fetch_real_data()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->usb_fetch_real_data(usb_device_addr_id,errcode,data);
    // xbasic::debug_output("Exited mgr_session::usb_fetch_real_data() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::usb_fetch_real_data() ret=%d",ret);
    return ret;
}

int call_cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name)
{
    // xbasic::debug_output("Enter into mgr_session::call_cal_read_file()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_cal_read_file() ota_type:%d, version_num:%s, board_id:0x%x, file_name:%s",ota_type, version_num, usb_device_addr_id, file_name.c_str());
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_cal_read_file(ota_type,version_num,usb_device_addr_id,errcode,result,file_name);
    // xbasic::debug_output("Exited mgr_session::call_cal_read_file() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_cal_read_file() ret=%d",ret);
    return ret;
}

int mgr_session::call_usb_getserials_ids(std::set<int>& usbdstids,int &errcode)
{
    // xbasic::debug_output("Enter into mgr_session::call_usb_getserials_ids()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_usb_getserials_ids()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_usb_getserials_ids(usbdstids,errcode);
    // xbasic::debug_output("Exited mgr_session::call_usb_getserials_ids() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_usb_getserials_ids() ret=%d",ret);
    return ret;
}

int mgr_session::call_asic_juictions_temp(std::vector<xusbadapter_package::junction_temp> &juncts_temp,int &errcode)
{
    // xbasic::debug_output("Enter into mgr_session::call_asic_juictions_temp()\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_session::call_asic_juictions_temp()");
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_asic_juiction_temp(juncts_temp,errcode);
    // xbasic::debug_output("Exited mgr_session::call_asic_juictions_temp() ret=%d.\n",ret);
    LOG_MSG(MSG_LOG, "Exited mgr_session::call_asic_juictions_temp() ret=%d",ret);
    return ret;
}

int mgr_session::call_get_value_from_dev(int dstid, int type, int& errcode, std::string& value)
{
    // xbasic::debug_output("mgr_session::call_get_value_from_dev: pack_len:%d \n", pack_len);
    LOG_MSG(MSG_LOG, "mgr_session::call_get_value_from_dev()");
    if (!m_lib_session) {
        // xbasic::debug_output("mgr_session::call_get_value_from_dev() m_lib_session is NULL.\n");
        LOG_MSG(ERR_LOG, "mgr_session::call_get_value_from_dev() m_lib_session is NULL");
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_get_value_from_dev(dstid, type, errcode, value);
    // xbasic::debug_output("mgr_session::call_get_value_from_dev: m_usb_dst=0x%x m_value_type:%d\n",m_usb_dst, m_value_type);
    LOG_MSG(MSG_LOG, "mgr_session::call_get_value_from_dev: dstid=%d type=%d",dstid, type);
    return ret;
}

int mgr_session::call_powercontrol_req(int dstid,int type,int sw_val,int& errcode,int utp40_id)
{
    LOG_MSG(MSG_LOG, "mgr_session::call_powercontrol_req() dstid:%d type:%d sw_val:%d utp40_id:%d",dstid,type,sw_val,utp40_id);
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_powercontrol_req(dstid, type, sw_val, errcode, utp40_id);
    LOG_MSG(MSG_LOG, "mgr_session::call_powercontrol_req() ret=%d",ret);
    return ret;
}

int mgr_session::call_set_frequency(int dstid,int type,float fre_val,int& errcode,int channel)
{
    LOG_MSG(MSG_LOG, "mgr_session::call_set_frequency() dstid:%d type:%d fre_val:%f channel:%d",dstid,type,fre_val,channel);
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_set_frequency(dstid, type, fre_val, errcode, channel);
    LOG_MSG(MSG_LOG, "mgr_session::call_set_frequency() ret=%d",ret);
    return ret;
}

int mgr_session::call_get_utp_register(int dstid,int chip_id,int& errcode,std::string& value)
{
    // xbasic::debug_output("xusbadapter_package::parse_get_value_response_body: pack_len:%d \n", pack_len);
    LOG_MSG(MSG_LOG, "mgr_session::call_get_utp_register() dstid:%d chip_id:%d",dstid,chip_id);
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_get_utp_register(dstid, chip_id, errcode, value);
    LOG_MSG(MSG_LOG, "mgr_session::call_get_utp_register() ret=%d",ret);
    return ret;
}

int mgr_session::call_get_one_utp102_register(int dstid,int chip_id,int& errcode,std::string& value)
{
    LOG_MSG(MSG_LOG, "mgr_session::call_get_one_utp102_register() dstid:%d chip_id:%d",dstid,chip_id);
    if (!m_lib_session) {
        return -1;
    }
    int ret = 0;
    ret = m_lib_session->call_get_one_utp102_register(dstid, chip_id, errcode, value);
    LOG_MSG(MSG_LOG, "mgr_session::call_get_one_utp102_register() ret=%d",ret);
    return ret;
}