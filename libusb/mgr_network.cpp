#include "mgr_network.h"
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "xpackage.hpp"

mgr_network::mgr_network(): m_init_semph(0), m_init_ok(false)
{
    m_adapter_cli.reset(new xtcp_client());
    m_adapter_cli->set_callback(std::bind(&mgr_network::on_tcp_cli_msg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)); //设置回调函数
    m_adapter_cli->set_packer_unpacker(new usbadapter_bin_packer, new usbadapter_bin_unpacker); //重定义编解码器
}

mgr_network::~mgr_network()
{
    m_adapter_cli->set_callback(nullptr);
    m_adapter_cli->close_socket();
}

void mgr_network::init() //初始化
{
    if (!m_adapter_cli->is_connected()) //平台客户端还没有连接
    {
        m_adapter_cli->start_connect();
    }
}

void mgr_network::work(unsigned long ticket) //工作函数
{
    if (!m_adapter_cli->is_connected()) //平台客户端还没有连接
    {
        m_adapter_cli->start_connect();
    }
    else //平台客户端已经连接
    {
        long current_time = time(NULL);
        long last_time = m_adapter_cli->get_last_recv_tm();
        long tm_rec_interval = (long)abs(current_time - last_time); //最后接收数据时间与当前间隔秒数
        if (tm_rec_interval > 15) //心跳超时
        {
            m_adapter_cli->disconnect();
            xbasic::debug_output("mgr_network::work() heartbeat timeout,current_time=%ld,last_time=%ld\n",current_time,last_time);
        }
        else if (tm_rec_interval > 5) //心跳即将超时
        {
            //发送心跳包到adapter server端
            send_heartbeat();
        }
    }
}

void mgr_network::on_tcp_cli_msg(xtcp_client *cli, xtcp_client::CBKTCPMSG tcp_msg, char *pdata, int data_len) //TCP客户端回调函数
{
    // xbasic::debug_output("Enter into mgr_network::on_tcp_cli_msg().\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_network::on_tcp_cli_msg()");
    if (tcp_msg == xtcp_client::TCP_DATA)
    {
        xusbadapter_package pack;
        if (!pack.parse_from_bin((unsigned char*)pdata,data_len)) {
            //解析bin失败
            // xbasic::debug_output("mgr_network::on_tcp_cli_msg() parse bin failed!!\n");
            LOG_MSG(ERR_LOG, "mgr_network::on_tcp_cli_msg() parse bin failed!!");
            return;
        }

        if (!m_init_ok) {
            m_init_ok = true;
            m_init_semph.post();
        }

        if (m_fn_callbk) {
            m_fn_callbk(this, DEV_DATA, PORT_ADAPTER, &pack); //回调
        }
    }
    else if (tcp_msg == xtcp_client::TCP_CONNECTED) //连接建立
    {
        if (m_fn_callbk) {
            m_fn_callbk(this, DEV_CONNECTED, PORT_ADAPTER, NULL); //回调
        }
    }
    else if (tcp_msg == xtcp_client::TCP_DISCONNECTED) //连接断开
    {
        if (m_fn_callbk) {
            m_fn_callbk(this, DEV_DISCONNECTED, PORT_ADAPTER, NULL); //回调
        }
    }
    // xbasic::debug_output("Exited mgr_network::on_tcp_cli_msg().\n");
    LOG_MSG(MSG_LOG, "Exited mgr_network::on_tcp_cli_msg()");
}

int mgr_network::start_work(int work_cycle) //开始工作
{
    int ret_start = xmgr_basic::start_work(work_cycle);
    boost::posix_time::ptime wait_until = boost::posix_time::microsec_clock::universal_time();
    wait_until += boost::posix_time::milliseconds(60000);
    m_init_ok = m_init_semph.timed_wait(wait_until); //等待到信号量
    return ret_start;
}

void mgr_network::stop_work() //停止工作
{
    xmgr_basic::stop_work();
    //xtcp client应该在线程停止后，再关闭socket更合理
    //否则如果出先关闭socket，在work函数中再次发生重连现象
    m_adapter_cli->disconnect();
}

void mgr_network::set_callback(CALLBK_FN fn)
{
    m_fn_callbk = fn; //设置回调函数
}

bool mgr_network::is_init_ok()
{
    return m_init_ok; //初始化是否成功
}

void mgr_network::set_adapter_addr(std::string adapter_addr) //设置平台的服务器地址
{
    // xbasic::debug_output("Enter into mgr_network::set_adapter_addr() adapter_addr=%s.\n",adapter_addr.c_str());
    LOG_MSG(MSG_LOG, "Enter into mgr_network::set_adapter_addr() adapter_addr=%s",adapter_addr.c_str());
    std::vector<std::string> vct_addr;
    if (xbasic::split_string(adapter_addr, ":", &vct_addr) < 2)
    {
        // xbasic::debug_output("mgr_network::set_adapter_addr() addr is invalid not ip:port format.\n");
        LOG_MSG(WRN_LOG, "mgr_network::set_adapter_addr() addr is invalid not ip:port format");
        return; //不是地址格式
    }
    m_adapter_cli->set_server_addr(vct_addr[0].c_str(), atoi(vct_addr[1].c_str()));
    // xbasic::debug_output("Exited mgr_network::set_adapter_addr().\n");
    LOG_MSG(MSG_LOG, "Exited mgr_network::set_adapter_addr()");
}

int mgr_network::send_to_adapter(xusbadapter_package *pack)
{
    // xbasic::debug_output("Enter into mgr_network::send_to_adapter().\n");
    LOG_MSG(MSG_LOG, "Enter into mgr_network::send_to_adapter()");
    std::string pack_data = pack->serial_to_bin(); //将包串行化
    int ret = -1;
    if(pack_data.empty()) {
        // xbasic::debug_output("mgr_network::send_to_adapter() seriral_to_bin failed,pack_data is empty.\n",pack->m_msg_type,pack->m_msg_cmd);
        LOG_MSG(WRN_LOG, "mgr_network::send_to_adapter() seriral_to_bin failed,pack_data is empty",pack->m_msg_type,pack->m_msg_cmd);
        return ret;
    }

    ret = m_adapter_cli->send_data(pack_data.data(), pack_data.length());
    if(ret == -1) {
        // xbasic::debug_output("mgr_network::send_to_adapter() send data(msg type=%d,cmd=%d) failed!!\n",pack->m_msg_type,pack->m_msg_cmd);
        LOG_MSG(ERR_LOG, "mgr_network::send_to_adapter() send data(msg type=%d,cmd=%d) failed!",pack->m_msg_type,pack->m_msg_cmd);
    }
    // xbasic::debug_output("Exited mgr_network::send_to_adapter().\n");
    LOG_MSG(MSG_LOG, "Exited mgr_network::send_to_adapter()");
    return ret;
}

int mgr_network::send_heartbeat()
{
    xusbadapter_package pack;
    pack.m_msg_type = xusbadapter_package::MT_REQUEST;
    pack.m_msg_cmd = xusbadapter_package::MC_HEARTBEAT;

    int ret = send_to_adapter(&pack);
    return ret;
}