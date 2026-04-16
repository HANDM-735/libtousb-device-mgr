#ifndef X_MGR_BASIC_H
#define X_MGR_BASIC_H

#include <set>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include "xbasic.hpp"
#include "xpackage.hpp"

// 网络消息监听器基类
class xlistener
{
public:
    // 网络消息类型枚举
    enum NET_MSG { NET_CONNECTED = 1, NET_DISCONNECTED, NET_DATA };

public:
    xlistener() {}

    // 添加消息过滤标签
    void add_filter(std::string filt_flag)
    {
        m_set_filter.insert(filt_flag);
    }

    // 判断标签是否在过滤白名单内
    bool judge_filter(std::string flag)
    {
        return (m_set_filter.find(flag) != m_set_filter.end());
    }

public:
    // 网络状态变化回调（禁止在回调内增删监听器）
    virtual int on_network(NET_MSG msg_type, const char *port_name, xpacket *packet) { return 0; }
    // 业务报文接收回调（禁止在回调内增删监听器）
    virtual int on_message(const char *msg_type, const char *channel, void *msg_data) { return 0; }

protected:
    std::set<std::string> m_set_filter;    // 消息过滤器白名单
};

// 报文传输管理器（发布-订阅+确认重传缓存）
class xtransmitter
{
public:
    // 报文发送代理函数类型
    typedef std::function<int(const char *, xpacket *)> SEND_DATA_FN;

public:
    xtransmitter(std::string obj_name) : m_obj_name(obj_name) {}

    // 设置底层发送代理接口
    void set_proxy_send(SEND_DATA_FN fn) { m_send_fn = fn; }

public:
    // 报文发送接口
    virtual int send(const char *port_name, xpacket *pack)
    {
        int ret = -1;
        if (m_send_fn) ret = m_send_fn(port_name, pack);

        // 开启应答报文缓存重传机制
        /*
        if(pack->need_confirm())
        {
            boost::unique_lock<boost::shared_mutex> lock(m_mux_packet); // 写锁
            if(m_lst_packet.size() > 512) m_lst_packet.pop_front();       // 限制最大缓存512个待确认报文
            m_lst_packet.push_back(boost::shared_ptr<xpacket>(pack->clone()));
        }
        */
        return ret;
    }

    // 缓存队列补发报文
    virtual int send_cache(const char *port_name)
    {
        if(m_lst_packet.size() <= 0) return 0;

        boost::shared_lock<boost::shared_mutex> lock(m_mux_packet); // 读锁
        boost::shared_ptr<xpacket> packet = m_lst_packet.front();
        lock.unlock();

        if(m_send_fn) return m_send_fn(port_name, packet.get());
        return -1;
    }

    // 接收报文回调：匹配确认包，清理重传缓存 + 广播消息给所有监听器
    virtual int on_recv(const char *port_name, xpacket *pack)
    {
        // 匹配确认报文，删除对应待重传数据包
        if(pack->type_confirm())
        {
            boost::unique_lock<boost::shared_mutex> lock(m_mux_packet); // 写锁
            for(std::list<boost::shared_ptr<xpacket>>::iterator iter=m_lst_packet.begin(); iter!=m_lst_packet.end(); iter++)
            {
                boost::shared_ptr<xpacket> packet = *iter;
                if(pack->is_confirm(packet.get()))
                {
                    m_lst_packet.erase(iter);
                    break;
                }
            }
        }

        // 遍历所有监听器，分发接收消息
        boost::shared_lock<boost::shared_mutex> lock(m_mux_listener); // 读锁
        for(std::list<xlistener *>::iterator iter=m_lst_listener.begin(); iter!=m_lst_listener.end(); iter++)
        {
            xlistener *listener = *iter;
            listener->on_message(m_obj_name.c_str(), port_name, pack);
        }
        return m_lst_listener.size();
    }

    // 添加消息监听器
    virtual int add_listener(xlistener *new_listener)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mux_listener); // 写锁
        for(std::list<xlistener *>::iterator iter=m_lst_listener.begin(); iter!=m_lst_listener.end(); iter++)
        {
            xlistener *listener = *iter;
            if(listener == new_listener) return 0; // 重复添加，直接返回
        }
        m_lst_listener.push_back(new_listener);
        return 1;
    }

    // 移除消息监听器
    virtual void del_listener(xlistener *del_listener)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mux_listener); // 写锁
        for(std::list<xlistener *>::iterator iter=m_lst_listener.begin(); iter!=m_lst_listener.end();)
        {
            if((xlistener *)(*iter) != del_listener) { iter++; continue; }
            else
            {
                m_lst_listener.erase(iter++);
                return;
            }
        }
    }

protected:
    std::string                          m_obj_name;        // 传输模块名称
    SEND_DATA_FN                         m_send_fn;         // 底层发送代理函数
    std::list<xlistener *>               m_lst_listener;    // 注册监听器链表
    boost::shared_mutex                   m_mux_listener;    // 监听器读写锁
    std::list<boost::shared_ptr<xpacket>> m_lst_packet;      // 待应答重传报文缓存
    boost::shared_mutex                   m_mux_packet;      // 报文缓存读写锁
};

// 通用管理器模板基类（定时循环工作线程 + 线程安全单例）
template<typename T>
class xmgr_basic : public xlistener
{
public:
    // C++11 静态局部变量 线程安全单例
    static T *get_instance()
    {
        static T s_instance;
        return &s_instance;
    }

    virtual ~xmgr_basic() {}

protected:
    xmgr_basic()
    {
        m_work_sign = false;
        m_work_cycle = 1000;
    }

public:
    // 启动定时工作线程
    virtual int start_work(int work_cycle = 1000)
    {
        if(m_work_sign) return 0; // 已运行，重复启动无效

        this->init();
        m_work_sign = true;
        m_work_cycle = work_cycle;

        // 绑定并启动后台工作线程
        boost::thread tmp_thread(boost::bind(&xmgr_basic::work_thread, this));
        boost::this_thread::yield();
        tmp_thread.swap(m_work_thread);
        return 1;
    }

    // 停止工作线程
    virtual void stop_work()
    {
        m_work_sign = false;
        m_work_thread.join();
    }

    // 查询线程运行状态
    virtual bool is_working() { return m_work_sign; }

    // 初始化虚函数（子类重写）
    virtual void init() {}
    // 周期业务处理虚函数（子类重写）
    virtual void work(unsigned long ticket) {}

private:
    // 后台定时工作线程入口
    void work_thread()
    {
        unsigned long ticket = 0;
        while(m_work_sign)
        {
            // 周期休眠
            boost::this_thread::sleep(boost::posix_time::milliseconds(m_work_cycle));
            this->work(ticket++);
        }
    }

protected:
    boost::thread    m_work_thread;    // 后台工作线程
    bool             m_work_sign;       // 线程运行标志
    unsigned int     m_work_cycle;      // 定时周期(ms)
};

#endif