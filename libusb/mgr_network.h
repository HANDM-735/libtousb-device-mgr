#ifndef MGR_NETWORK_H
#define MGR_NETWORK_H
#include <string>
#include <list>
#include <boost/array.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include "xbasic.hpp"
#include "xbasicmgr.hpp"
#include "xbasicasio.hpp"
#include "xpackage.hpp"
#include "usbadapter_package.h"
#include "mgr_log.h"

#define PORT_ADAPTER  "<adapter>"   //单板通讯端口名称定义

class mgr_network : public xmgr_basic<mgr_network> //网络管理器
{
public:
    enum CBKDEVMSG { DEV_CONNECTED = 1, DEV_DISCONNECTED, DEV_DATA };
    typedef std::function<void(mgr_network *, CBKDEVMSG, const char *, xusbadapter_package *)> CALLBK_FN; //回调函数定义

    mgr_network();
    ~mgr_network();

public:
    void set_callback(CALLBK_FN fn);
    bool is_init_ok();
    void set_adapter_addr(std::string adapter_addr); //设置平台的服务器地址
    int send_to_adapter(xusbadapter_package *pack);

public:
    virtual void init();                    //初始化
    virtual void work(unsigned long ticket); //工作函数
    virtual int start_work(int work_cycle = 1000); //开始工作
    virtual void stop_work(); //停止工作

protected:
    void on_tcp_cli_msg(xtcp_client *cli,xtcp_client::CBKTCPMSG tcp_msg, char *pdata, int data_len); //TCP客户端回调函数
    int send_heartbeat();

private:
    CALLBK_FN                                   m_fn_callbk;        //向上层的回调函数
    boost::shared_ptr<xtcp_client>              m_adapter_cli;      //适配层客户端
    boost::interprocess::interprocess_semaphore m_init_semph;       //初始化信号量
    bool                                        m_init_ok;          //初始化是否成功标志
};

#endif