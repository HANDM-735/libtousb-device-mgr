#include "xsession.h"
#include "mgr_network.h"

// 信号量构造函数
xlibsemaphore::xlibsemaphore(int id) : m_id(id), m_errcode(0), m_sync_semaph(0)
{

}

void xlibsemaphore::post()
{
    m_sync_semaph.post(); //触发信号量
}

void xlibsemaphore::wait()
{
    m_sync_semaph.wait(); //阻塞等待信号量
}

bool xlibsemaphore::try_wait()
{
    return m_sync_semaph.try_wait(); //非阻塞判断信号量是否触发
}

bool xlibsemaphore::timed_wait(int milliseconds) //等待信号量或超时
{
    boost::posix_time::ptime wait_until(boost::posix_time::microsec_clock::universal_time());
    wait_until += boost::posix_time::milliseconds(milliseconds);
    return m_sync_semaph.timed_wait(wait_until); //等待到信号量
}

// 请求消息构造函数
xrequest::xrequest(const char *port_name, xusbadapter_package *pack) : m_port_name(port_name), m_pack(dynamic_cast<xusbadapter_package*>(pack->clone()))
{

}

// 会话构造
xsession::xsession(int session_id) : xtransmitter("<session>")
{
    m_sess_id = session_id;
}

xsession::~xsession()
{

}

std::string xsession::get_port_name()
{
    return m_port_name;
}

int xsession::get_session_id()
{
    return m_sess_id;
}

// 消息接收分发入口
int xsession::on_recv(const char *port_name, xusbadapter_package *pack) //消息通知
{
    // xbasic::debug_output("Enter into xsession::on_recv().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::on_recv()");

    if(pack->m_msg_type == xusbadapter_package::MT_RESPOND) //是应答(下面库主动调用后的应答)
    {
        switch(pack->m_msg_cmd)
        {
            case xusbadapter_package::MC_START:
                handle_start_response(pack);
                break;
            case xusbadapter_package::MC_QUERY:
                handle_query_response(pack);
                break;
            case xusbadapter_package::MC_CANCEL:
                handle_cancle_response(pack);
                break;
            case xusbadapter_package::MC_DATA:
                handle_data_response(pack);
                break;
            case xusbadapter_package::MC_USB_SERIALS_ID:
                handle_usbserialids_response(pack);
                break;
            case xusbadapter_package::MC_ASIC_JUNCTION_TEMP:
                handle_asicjunctions_response(pack);
                break;
            case xusbadapter_package::MC_GET_VALUE:
                handle_getvalue_response(pack);
                break;
            case xusbadapter_package::MC_COMMAND:
                handle_powercontrol_response(pack);
                break;
            case xusbadapter_package::MC_FREQUENCY:
                handle_frequencyset_response(pack);
                break;
            case xusbadapter_package::MC_CAL_READ:
                handle_cal_read_response(pack);
                break;
            case xusbadapter_package::MC_GET_UTP_REGISTER:
                handle_get_utp_register_response(pack);
                break;
            case xusbadapter_package::MC_GET_UTP102_REGISTER:
                handle_get_utp102_register_response(pack);
                break;
        }
    }
    // xbasic::debug_output("Exited xsession::on_recv().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::on_recv()");
    return xtransmitter::on_recv(port_name,pack);
}

// OTA启动响应处理
void xsession::handle_start_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_start_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_start_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        // xbasic::debug_output("xsession::handle_start_response() err_info=%s.\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_start_response() err_info=%s",err_info.c_str());
        semaphore->post();
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_start_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_start_response()");
    return ;
}

// OTA进度查询响应
void xsession::handle_query_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_query_response().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_query_response()");
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        int progress_len = pack->m_upgrade_progress;
        int total_len = pack->m_total_len;
        float progress = 0.0;
        if(total_len != 0)
        {
            progress = static_cast<float>(progress_len)/static_cast<unsigned int>(total_len);
        }
        LOG_MSG(MSG_LOG, "xsession::handle_query_response() progress_len=%u total_len=%u progress=%f", progress_len, total_len, progress);
        std::string progress_str = std::to_string(progress);
        semaphore->m_att_data = progress_str;
        semaphore->post();
    }
    lock.unlock();
    // xbasic::debug_output("Enter into xsession::handle_query_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_query_response()");
    return ;
}

// OTA取消响应
void xsession::handle_cancle_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_cancle_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_cancle_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_cancle_response() err_info=%s.\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_cancle_response() err_info=%s",err_info.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_cancle_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_cancle_response()");
    return ;
}

// OTA数据包响应
void xsession::handle_data_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_data_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_data_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        semaphore->m_rw_data = pack->m_rw_data;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_data_response() err_info=%s\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_data_response() err_info=%s",err_info.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_data_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_data_response()");
    return ;
}

// 校准文件读取响应
void xsession::handle_cal_read_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_cal_read_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_cal_read_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        // semaphore->m_rw_data = pack->m_rw_data;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_cal_read_response() err_info=%s.\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_cal_read_response() err_info=%s",err_info.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_cal_read_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_cal_read_response()");
    return ;
}

// USB序列号查询响应
void xsession::handle_usbserialids_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_usbserialids_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_usbserialids_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;

        int sz = pack->m_usbdst_ids.size();
        for(int i = 0 ; i < sz; i++)
        {
            semaphore->m_ids.insert(pack->m_usbdst_ids[i]);
        }
        semaphore->post();
        // xbasic::debug_output("xsession::handle_usbserialids_response() err_info=%s.\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_usbserialids_response() err_info=%s",err_info.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_usbserialids_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_usbserialids_response()");
    return ;
}

// ASIC结温查询响应
void xsession::handle_asicjunctions_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_asicjunctions_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_asicjunctions_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;

        int sz = pack->m_junctions.size();
        for(int i = 0 ; i < sz; i++)
        {
            semaphore->m_juncts.push_back(pack->m_junctions[i]);
        }
        semaphore->post();
        // xbasic::debug_output("xsession::handle_asicjunctions_response() err_info=%s.\n",err_info.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_asicjunctions_response() err_info=%s",err_info.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_asicjunctions_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_asicjunctions_response()");
    return ;
}

// 设备参数读取响应
void xsession::handle_getvalue_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_getvalue_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_getvalue_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        semaphore->m_att_data = pack->m_value_dev;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_getvalue_response() err_code=%d, data_size=%d, data=%s.\n",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_getvalue_response() err_code=%d, data_size=%d, data=%s",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_getvalue_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_getvalue_response()");
    return ;
}

// 电源控制响应
void xsession::handle_powercontrol_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_powercontrol_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_powercontrol_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_powercontrol_response() err_code=%d\n",pack->m_err_code);
        LOG_MSG(WRN_LOG, "xsession::handle_powercontrol_response() err_code=%d",pack->m_err_code);
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_powercontrol_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_powercontrol_response()");
    return ;
}

// 频点设置响应
void xsession::handle_frequencyset_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_frequencyset_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_frequencyset_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        std::string err_info = (pack->m_err_code == 0) ? "Success" : "Failed";
        semaphore->m_att_data = err_info;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_frequencyset_response() err_code=%d\n",pack->m_err_code);
        LOG_MSG(WRN_LOG, "xsession::handle_frequencyset_response() err_code=%d",pack->m_err_code);
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_frequencyset_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_frequencyset_response()");
    return ;
}

// UTP寄存器读取响应
void xsession::handle_get_utp_register_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_getvalue_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_get_utp_register_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        semaphore->m_att_data = pack->m_value_dev;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_get_utp_register_response() err_code=%d, data_size=%d, data=%s.\n",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_get_utp_register_response() err_code=%d, data_size=%d, data=%s",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_get_utp_register_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_get_utp_register_response()");
    return ;
}

// UTP102寄存器读取响应
void xsession::handle_get_utp102_register_response(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into xsession::handle_getvalue_response() session_id=%d\n",pack->m_session);
    LOG_MSG(MSG_LOG, "Enter into xsession::handle_get_utp102_register_response() session_id=%d",pack->m_session);
    boost::shared_lock<boost::shared_mutex> lock(m_mux_semaphore); //读锁
    boost::unordered_map<int, boost::shared_ptr<xlibsemaphore> >::iterator iter = m_map_semaphore.find(pack->m_session);
    if (iter != m_map_semaphore.end()) //找到指定信号量
    {
        boost::shared_ptr<xlibsemaphore> semaphore = iter->second;
        semaphore->m_errcode = pack->m_err_code;
        semaphore->m_att_data = pack->m_value_dev;
        semaphore->post();
        // xbasic::debug_output("xsession::handle_get_utp102_register_response() err_code=%d, data_size=%d, data=%s.\n",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
        LOG_MSG(WRN_LOG, "xsession::handle_get_utp102_register_response() err_code=%d, data_size=%d, data=%s",pack->m_err_code,pack->m_value_dev.size(),pack->m_value_dev.c_str());
    }
    lock.unlock();
    // xbasic::debug_output("Exited xsession::handle_get_utp102_register_response().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::handle_get_utp102_register_response()");
    return ;
}

// OTA类型校验
bool xsession::is_ota_caltype(int type)
{
    bool ret = false;
    if((type >= OTA_TYPE_CPPE_MUTP40_W_CAL) && (type <= OTA_TYPE_FTFEB_PE_DC_CAL))
    {
        ret = true;
    }
    return ret;
}

// OTA升级开始同步接口
int xsession::call_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name)
{
    // xbasic::debug_output("Enter into xsession::call_ota_start_upgrade().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_ota_start_upgrade(), version_num=%s, board_id=0x%x, file_name=%s", ota_type, version_num, usb_device_addr_id, file_name.c_str());
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_START;
    pack->m_ota_type = ota_type;
    std::string version = std::string(version_num);
    if(version.find(".") != std::string::npos)
    {
        pack->m_ota_version = xbasic::version_to_int(version_num);
    }
    else
    {
        pack->m_ota_version = std::stoll(version_num);
    }
    pack->m_usb_dst = usb_device_addr_id;
    pack->m_calfilename = file_name;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    // xbasic::debug_output("xsession::call_ota_start_upgrade() send start message ota_type=%d session_id=%d usb_dst=%d\n",ota_type,pack->m_session,pack->m_usb_dst);
    LOG_MSG(MSG_LOG, "xsession::call_ota_start_upgrade() send start message ota_type=%d session_id=%d usb_dst=%d",ota_type,pack->m_session,pack->m_usb_dst);
    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        result = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_ota_start_upgrade() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_ota_start_upgrade() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_ota_start_upgrade().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_ota_start_upgrade()");
    return (waited ? 0 : -1);
}

// OTA升级取消同步接口
int xsession::call_ota_cancel_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into xsession::call_ota_cancel_upgrade().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_ota_cancel_upgrade()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_CANCEL;
    pack->m_ota_type = ota_type;
    pack->m_usb_dst = usb_device_addr_id;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    // xbasic::debug_output("xsession::call_ota_cancel_upgrade() send start message ota_type=%d session_id=%d usb_dst=%d\n",ota_type,pack->m_session,pack->m_usb_dst);
    LOG_MSG(MSG_LOG, "xsession::call_ota_cancel_upgrade() send start message ota_type=%d session_id=%d usb_dst=%d",ota_type,pack->m_session,pack->m_usb_dst);
    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        result = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_ota_cancel_upgrade() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_ota_cancel_upgrade() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_ota_cancel_upgrade().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_ota_cancel_upgrade()");
    return (waited ? 0 : -1);
}

// OTA进度查询同步接口
int xsession::call_ota_query_upgrade(int ota_type,int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into xsession::call_ota_query_upgrade().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_ota_query_upgrade() ota_type=%d, board_id=0x%x",ota_type,usb_device_addr_id);
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_QUERY;
    pack->m_ota_type = ota_type;
    pack->m_usb_dst = usb_device_addr_id;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        result = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_ota_query_upgrade() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_ota_query_upgrade() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_ota_query_upgrade().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_ota_query_upgrade()");
    return (waited ? 0 : -1);
}

// OTA完成通知接口（无同步等待）
int xsession::call_ota_complete_upgrade(int ota_type, int usb_device_addr_id,int &errcode,std::string &result)
{
    // xbasic::debug_output("Enter into xsession::call_ota_complete_upgrade().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_ota_complete_upgrade()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_NOTIFY));
    pack->m_msg_cmd = xusbadapter_package::MC_COMPLETE;
    pack->m_ota_type = ota_type;
    pack->m_usb_dst = usb_device_addr_id;

    int call_ret = send(PORT_ADAPTER, pack.get());
    LOG_MSG(MSG_LOG, "xsession::call_ota_complete_upgrade() send complete notify message ota_type=%d session_id=%d usb_dst=%d",ota_type,pack->m_session,pack->m_usb_dst);

    // 设备管理并没有对这个 notify 消息进行响应，所以把下面的等待响应代码暂时注释掉
    /*
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        result = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_ota_complete_upgrade() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_ota_complete_upgrade() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    */

    // xbasic::debug_output("Exited xsession::call_ota_complete_upgrade().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_ota_complete_upgrade()");
    return call_ret;
}

// 下发获取OTA数据包
int xsession::usb_fetch_real_data(int usb_device_addr_id,int &errcode,boost::shared_ptr<xusbadapter_package::rw_data>& data)
{
    // xbasic::debug_output("Enter into xsession::usb_fetch_real_data().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::usb_fetch_real_data()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_DATA;
    pack->m_usb_dst = usb_device_addr_id;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        boost::shared_ptr<xusbadapter_package::rw_data> new_data(new xusbadapter_package::rw_data(new_semaphore->m_rw_data));
        data = new_data;
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::usb_fetch_real_data().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::usb_fetch_real_data()");
    return (waited ? 0 : -1);
}

// 校准文件读取同步接口
int xsession::call_cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name)
{
    // xbasic::debug_output("Enter into xsession::call_cal_read_file().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_cal_read_file() ota_type=%d, version_num=%s, board_id=0x%x, file_name=%s",ota_type,version_num,usb_device_addr_id,file_name.c_str());
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_CAL_READ;
    pack->m_ota_type = ota_type;
    pack->m_ota_version = xbasic::version_to_int(version_num);
    pack->m_usb_dst = usb_device_addr_id;
    pack->m_calfilename = file_name;

    int call_ret = send(PORT_ADAPTER, pack.get());
    // 最多等待一个小时
    int wait_time = 3600000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    // xbasic::debug_output("xsession::call_cal_read_file() send start message ota_type=%d session_id=%d usb_dst=%d\n",ota_type,pack->m_session,pack->m_usb_dst);
    LOG_MSG(MSG_LOG, "xsession::call_cal_read_file() send start message ota_type=%d session_id=%d usb_dst=%d",ota_type,pack->m_session,pack->m_usb_dst);
    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited)
    {
        // 到这里说明等待到了信号量
        errcode = new_semaphore->m_errcode;
        result = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_cal_read_file() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_cal_read_file() get semaphore data");
    }
    else
    {
        // 获取信号量超时
        LOG_MSG(WRN_LOG, "xsession::call_cal_read_file() get semaphore data time out");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_cal_read_file().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_cal_read_file()");
    return (waited ? 0 : -1);
}

// USB设备序列号查询接口
int xsession::call_usb_serials_ids(std::set<int>& usbserials,int &errcode)
{
    // xbasic::debug_output("Enter into xsession::call_usb_serials_ids().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_usb_serials_ids()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_USB_SERIALS_ID;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        usbserials = new_semaphore->m_ids;
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_usb_serials_ids().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_usb_serials_ids()");
    return (waited ? 0 : -1);
}

int xsession::call_asic_junctions_temp(std::vector<xusbadapter_package::junction_temp>& juncts_temp,int &errcode)
{
    // xbasic::debug_output("Enter into xsession::call_asic_junctions_temp().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_asic_junctions_temp()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_ASIC_JUNCTION_TEMP;

    int call_ret = send(PORT_ADAPTER, pack.get());
    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        int sz = new_semaphore->m_juncts.size();
        for(int i = 0 ; i < sz; i++)
        {
            juncts_temp.push_back(new_semaphore->m_juncts[i]);
        }
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_asic_junctions_temp().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_asic_junctions_temp()");
    return (waited ? 0 : -1);
}

int xsession::call_get_value_from_dev(int dstid, int type, int& errcode, std::string& value)
{
    // xbasic::debug_output("Enter into xsession::call_get_value_from_dev().\n");
    LOG_MSG(MSG_LOG, "Enter into xsession::call_get_value_from_dev()");
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_GET_VALUE;
    pack->m_value_type = type;
    pack->m_usb_dst = dstid;

    int call_ret = send(PORT_ADAPTER, pack.get());
    // xbasic::debug_output("xsession::call_get_value_from_dev() pack->m_value_type=%d pack->m_usb_dst=0x%x pack->m_session=%d\n",pack->m_value_type,pack->m_usb_dst,pack->m_session);
    LOG_MSG(MSG_LOG, "xsession::call_get_value_from_dev() pack->m_value_type=%d pack->m_usb_dst=0x%x pack->m_session=%d",pack->m_value_type,pack->m_usb_dst,pack->m_session);

    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    int size = 0;
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        value = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_get_value_from_dev() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_get_value_from_dev() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_get_value_from_dev().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_get_value_from_dev()");
    return (waited ? 0 : -1);
}

int xsession::call_powercontrol_req(int dstid,int type,int sw_val,int& errcode,int utp40_id)
{
    // xbasic::debug_output("Enter into xsession::call_powercontrol_req() dstid=0x%x type=%d sw_val=%d utp40_id=%d\n",dstid,type,sw_val,utp40_id);
    LOG_MSG(MSG_LOG, "Enter into xsession::call_powercontrol_req() dstid=0x%x type=%d sw_val=%d utp40_id=%d",dstid,type,sw_val,utp40_id);
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_COMMAND;
    pack->m_usb_dst = dstid;
    pack->m_command_type.value = type;
    pack->m_command_value.m_utp40_id = utp40_id;
    pack->m_command_value.m_switch = sw_val;

    int call_ret = send(PORT_ADAPTER, pack.get());

    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    int size = 0;
    if(waited) //如果等到了返回则从信号量中获取返回数据
    {
        errcode = new_semaphore->m_errcode;
        // xbasic::debug_output("xsession::call_powercontrol_req() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_powercontrol_req() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_powercontrol_req().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_powercontrol_req()");
    return (waited ? 0 : -1);
}

int xsession::call_set_frequency(int dstid,int type, float fre_val,int& errcode,int channel)
{
    // xbasic::debug_output("Enter into xsession::call_set_frequency() dstid=0x%x type=%d fre_val=%f channel=%d\n",dstid,type,fre_val,channel);
    LOG_MSG(MSG_LOG, "Enter into xsession::call_set_frequency() dstid=0x%x type=%d fre_val=%f channel=%d",dstid,type,fre_val,channel);
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_FREQUENCY;
    pack->m_usb_dst = dstid;
    pack->m_frequency_type = type;
    pack->m_frequency_value.m_frequency = fre_val;
    pack->m_frequency_value.m_channel = channel;

    int call_ret = send(PORT_ADAPTER, pack.get());

    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    int size = 0;
    if(waited)
    {
        errcode = new_semaphore->m_errcode;
        // xbasic::debug_output("xsession::call_set_frequency() get semaphore data.\n");
        LOG_MSG(MSG_LOG, "xsession::call_set_frequency() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_set_frequency().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_set_frequency()");
    return (waited ? 0 : -1);
}

int xsession::call_get_one_utp_register(int dstid, int chip_id,int& errcode,std::string& value)
{
    // xbasic::debug_output("Enter into xsession::call_powercontrol_req() dstid=0x%x type=%d sw_val=%d utp40_id=%d\n",dstid,type,sw_val,utp40_id);
    LOG_MSG(MSG_LOG, "Enter into xsession::call_get_one_utp_register() dstid=0x%x chip_id=%d",dstid,chip_id);
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_GET_UTP_REGISTER;
    pack->m_usb_dst = dstid;
    pack->m_utp_chip_id = chip_id;

    int call_ret = send(PORT_ADAPTER, pack.get());

    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    int size = 0;
    if(waited)
    {
        errcode = new_semaphore->m_errcode;
        value = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_powercontrol_req() err_code=%d\n",pack->m_err_code);
        LOG_MSG(MSG_LOG, "xsession::call_get_one_utp_register() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_powercontrol_req().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_get_one_utp_register()");
    return (waited ? 0 : -1);
}

int xsession::call_get_one_utp102_register(int dstid, int chip_id,int& errcode,std::string& value)
{
    // xbasic::debug_output("Enter into xsession::call_powercontrol_req() dstid=0x%x type=%d sw_val=%d utp40_id=%d\n",dstid,type,sw_val,utp40_id);
    LOG_MSG(MSG_LOG, "Enter into xsession::call_get_one_utp102_register() dstid=0x%x chip_id=%d",dstid,chip_id);
    boost::shared_ptr<xusbadapter_package> pack(new xusbadapter_package(xusbadapter_package::MT_REQUEST));
    pack->m_msg_cmd = xusbadapter_package::MC_GET_UTP102_REGISTER;
    pack->m_usb_dst = dstid;
    pack->m_utp_chip_id = chip_id;

    int call_ret = send(PORT_ADAPTER, pack.get());

    int wait_time = 50000;
    boost::unique_lock<boost::shared_mutex> lock_1(m_mux_semaphore); //写锁
    boost::shared_ptr<xlibsemaphore> new_semaphore(new xlibsemaphore(pack->m_session));
    m_map_semaphore.insert(std::make_pair(pack->m_session, new_semaphore)); //将新信号量加入到集合中
    lock_1.unlock();

    bool waited = new_semaphore->timed_wait(wait_time); //等待得到信号量，由 2000 改为 30000，多了一次转发
    int size = 0;
    if(waited)
    {
        errcode = new_semaphore->m_errcode;
        value = new_semaphore->m_att_data;
        // xbasic::debug_output("xsession::call_powercontrol_req() err_code=%d\n",pack->m_err_code);
        LOG_MSG(MSG_LOG, "xsession::call_get_one_utp102_register() get semaphore data");
    }

    boost::unique_lock<boost::shared_mutex> lock_2(m_mux_semaphore); //写锁
    m_map_semaphore.erase(pack->m_session); //删除信号量
    lock_2.unlock();
    // xbasic::debug_output("Exited xsession::call_powercontrol_req().\n");
    LOG_MSG(MSG_LOG, "Exited xsession::call_get_one_utp102_register()");
    return (waited ? 0 : -1);
}