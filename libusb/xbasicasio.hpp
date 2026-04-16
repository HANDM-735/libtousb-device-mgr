#ifndef X_ASIO_BASIC_H
#define X_ASIO_BASIC_H

#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/array.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/list.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include "xbasic.hpp"

#define MAX_MSG_LEN     16384   // 接收缓冲区最大长度
#define MAX_MSG_NUM     128     // 收发缓冲区最多缓存报文个数
#define MAXIOTHREADNUM  64      // xioservice最大线程数量
#define POST_ACCEPT_NUM 32     // 预先投递的ACCEPT连接数量

// Asio IO服务池（多线程异步IO调度中心）
class xioservice : public boost::asio::io_service
{
public:
    xioservice() : m_use_count(0) {}
    virtual ~xioservice() {}

    // 启动IO服务线程池
    void start_serve()
    {
        if (++m_use_count > 1) return; // 对象被多处引用，不重复启动
        reset();

        boost::thread tmp_thread(boost::bind(&xioservice::run_threads, this));
        boost::this_thread::yield();
        tmp_thread.swap(m_main_thread);
    }

    // 停止IO服务
    void stop_serve()
    {
        if (--m_use_count > 0) return; // 还有端口在用，不关闭服务
        stop();
        m_main_thread.timed_join(boost::posix_time::seconds(6));
        m_threads_id.clear();
    }

    // 根据线程ID获取线程索引编号
    int get_thread_index(boost::thread::id thread_id)
    {
        boost::unordered_map<boost::thread::id, int>::iterator iter = m_threads_id.find(thread_id);
        return (iter == m_threads_id.end()) ? -1 : iter->second;
    }

protected:
    // 启动异步IO线程池
    inline void run_threads()
    {
        m_work.reset(new boost::asio::io_service::work(this));
        boost::thread_group thread_group;

        // 默认线程数 = CPU核心数 * 2
        int cpu_num = boost::thread::hardware_concurrency();
        int threads_num = (cpu_num > 1) ? cpu_num * 2 : 1;

        for (int i = 0; i < threads_num; ++i)
        {
            boost::thread *ptr_thread = thread_group.create_thread(boost::bind(&boost::asio::io_service::run, this, boost::system::error_code()));
            m_threads_id.insert(boost::unordered_map<boost::thread::id, int>::value_type(ptr_thread->get_id(), i));
        }

        boost::this_thread::yield();
        thread_group.join_all();
    }

private:
    boost::atomic<int>                              m_use_count;    // 对象引用计数器
    boost::thread                                   m_main_thread;  // IO主线程
    boost::shared_ptr<boost::asio::io_service::work> m_work;         // 保活对象，防止Run退出
    boost::unordered_map<boost::thread::id, int>   m_threads_id;   // 线程ID与索引映射
};

// 通用报文打包器
class xpacker
{
public:
    enum PACKWAY { PACK_NATIVE = 0, PACK_BASIC, PACK_USER };

public:
    virtual ~xpacker() {}

    // 原始数据打包成标准报文格式
    virtual boost::shared_ptr<const std::string> pack_data(const char *pdata, size_t datalen, PACKWAY pack_way)
    {
        boost::shared_ptr<const std::string> ppack;
        if (NULL == pdata || 0 == datalen) return ppack;

        std::string *pstring = new std::string();
        ppack.reset(pstring);

        pstring->reserve(datalen);
        pstring->append(pdata, datalen);
        return ppack;
    }
};

// 通用报文解包器
class xunpacker
{
public:
    xunpacker() { m_signed_len = (size_t)-1; m_data_len = 0; }
    virtual ~xunpacker() {}

    // 重置解包状态
    virtual void reset_data() { m_signed_len = (size_t)-1; m_data_len = 0; }

    // 二进制数据流解包拆分完整报文
    virtual bool unpack_data(size_t bytes_data, boost::container::list<boost::shared_ptr<const std::string>> &list_pack)
    {
        m_data_len = bytes_data;
        char *pbegin = m_raw_buff.begin();
        list_pack.push_back(boost::shared_ptr<std::string>(new std::string(pbegin, m_data_len)));
        return true;
    }

    // 准备接收缓冲区
    virtual boost::asio::mutable_buffers_1 prepare_buff(size_t &min_recv_len)
    {
        return boost::asio::buffer(m_raw_buff);
    }

protected:
    boost::array<char, MAX_MSG_LEN> m_raw_buff;     // 原始接收环形缓冲区
    size_t                          m_signed_len;  // 报文头部标识长度，-1=未收到包头
    size_t                          m_data_len;    // 已接收完整报文长度
};

// TCP套接字基类（异步收发+队列缓存+分包重组）
class xtcp_socket : public boost::asio::ip::tcp::socket
{
public:
    xtcp_socket(boost::asio::io_service &io_service)
        : boost::asio::ip::tcp::socket(io_service)
        , m_packer(new xpacker())
        , m_unpacker(new xunpacker())
    {}

    virtual ~xtcp_socket() {}

    // 重置套接字所有缓冲区状态
    void reset_buff()
    {
        boost::mutex::scoped_lock locks(m_send_mutex);
        m_send_buff.clear();
        m_is_sending = false;

        boost::mutex::scoped_lock lockr(m_recv_mutex);
        m_recv_buff.clear();
        m_is_dispatching = false;
    }

    // 关闭TCP套接字
    void close_socket()
    {
        reset_buff();
        boost::system::error_code err_code;
        shutdown(boost::asio::ip::tcp::socket::shutdown_both, err_code);
        close(err_code);
    }

    void recv_data() { do_recv_data(); }

    // 打包并异步发送报文
    int send_data(const char *pdata, const size_t length, xpacker::PACKWAY pack_type = xpacker::PACK_BASIC)
    {
        if (!is_open() || length <= 0) return -1;
        boost::shared_ptr<const std::string> pack = m_packer->pack_data(pdata, length, pack_type);
        send_pack_in_buff(pack);
        return (int)pack->length();
    }

    // 将报文压入发送缓存队列
    void send_pack_in_buff(boost::shared_ptr<const std::string> pack)
    {
        boost::mutex::scoped_lock lock(m_send_mutex);
        do_send_data();
    }

    // 获取待发送报文总数
    size_t get_pending_pack_num()
    {
        boost::mutex::scoped_lock lock(m_send_mutex);
        return m_send_buff.size();
    }

    // 查看队列首个待发送报文
    const boost::shared_ptr<const std::string> peek_first_pending_pack()
    {
        boost::mutex::scoped_lock lock(m_send_mutex);
        return m_send_buff.empty() ? boost::shared_ptr<const std::string>() : m_send_buff.front();
    }

    // 弹出队列首个待发送报文
    boost::shared_ptr<const std::string> pop_first_pending_pack()
    {
        boost::mutex::scoped_lock lock(m_send_mutex);
        if (m_send_buff.empty()) return boost::shared_ptr<const std::string>();

        boost::shared_ptr<std::string> str_ptr = m_send_buff.front();
        m_send_buff.pop_front();
        return str_ptr;
    }

    // 清空全部待发送报文
    void pop_all_pending_pack(boost::container::list<boost::shared_ptr<const std::string>> &pack_list)
    {
        boost::mutex::scoped_lock lock(m_send_mutex);
        pack_list.splice(pack_list.end(), m_send_buff);
    }

    // 运行时动态替换打包/解包器
    void set_packer(xpacker *new_packer, xunpacker *new_unpacker)
    {
        m_packer.reset(new_packer);
        m_unpacker.reset(new_unpacker);
    }

    boost::shared_ptr<xpacker>    get_packer()     { return m_packer; }
    boost::shared_ptr<xunpacker>  get_unpacker()   { return m_unpacker; }
    time_t                         get_last_recv_tm() { return m_tm_last_recv; }
    time_t                         get_last_send_tm() { return m_tm_last_send; }

public:
    virtual bool start_work() = 0;

protected:
    // 套接字业务是否正常可用
    virtual bool is_ok() = 0;
    // 报文解包错误回调
    virtual void on_unpack_error() = 0;
    // 底层IO接收异常回调
    virtual void on_recv_error(const boost::system::error_code &err_code) = 0;
    // 底层IO发送异常回调
    virtual void on_send_error(const boost::system::error_code &err_code) = 0;
    // 同步收到完整业务报文
    virtual bool on_data_recv(boost::shared_ptr<const std::string> &pack) = 0;
    // 异步分发报文通知
    virtual void on_data_send(boost::shared_ptr<const std::string> &pack) = 0;
    // 接收缓冲区溢出回调
    virtual void on_recv_buff_overflow(boost::shared_ptr<const std::string> &pack) = 0;

private:
    // 底层原生异步发送报文
    int send_native_data(boost::shared_ptr<const std::string> &str_data)
    {
        if (str_data == NULL || str_data->empty()) return -1;

        unsigned int msg_num = m_send_buff.size();
        if (msg_num > MAX_MSG_NUM) return -1;

        boost::mutex::scoped_lock lock(m_send_mutex);
        m_send_buff.push_back(str_data);
        do_send_data();
        return (int)str_data->length();
    }

    // 同步批量分发接收报文
    void sync_dispatch_data(boost::container::list<boost::shared_ptr<const std::string>> &pack_list)
    {
        if (pack_list.empty()) return;

        m_tm_last_recv = time(NULL);
        boost::mutex::scoped_lock lock(m_recv_mutex);
        unsigned int pack_num = m_recv_buff.size();

        BOOST_FOREACH(boost::shared_ptr<const std::string> &item, pack_list)
        {
            // 上层同步回调返回true，报文存入待异步分发队列
            if (on_data_recv(item))
            {
                if (pack_num < MAX_MSG_NUM)
                {
                    m_recv_buff.push_back(item);
                    pack_num++;
                }
                else
                {
                    on_recv_buff_overflow(item);
                }
            }
        }

        do_async_dispatch_data();
        lock.unlock();
    }

    // Asio异步读回调
    void recv_handler(const boost::system::error_code &err_code, size_t bytes_transferred)
    {
        if (!err_code && bytes_transferred > 0)
        {
            boost::container::list<boost::shared_ptr<const std::string>> pack_list;
            bool unpack_ok = m_unpacker->unpack_data(bytes_transferred, pack_list);

            if (!pack_list.empty()) sync_dispatch_data(pack_list);
            if (!unpack_ok) on_unpack_error();

            do_recv_data(); // 继续投递下一次异步接收
        }
        else
        {
            boost::system::error_code err_code_copy(err_code);
            on_recv_error(err_code_copy);
        }
    }

    // Asio异步写回调
    void send_handler(const boost::system::error_code &err_code, size_t bytes_transferred, boost::shared_ptr<const std::string> pack)
    {
        if (!err_code && bytes_transferred > 0)
        {
            on_data_send(pack);
        }
        else
        {
            boost::system::error_code err_code_copy(err_code);
            on_send_error(err_code_copy);
        }

        boost::mutex::scoped_lock lock(m_send_mutex);
        m_is_sending = false;
        do_send_data(); // 继续发送队列下一个报文
    }

    // 执行发送队列异步写入
    void do_send_data()
    {
        if (!is_ok() || !is_open()) return;

        if (!m_is_sending && !m_send_buff.empty())
        {
            m_is_sending = true;
            boost::shared_ptr<const std::string> pack = m_send_buff.front();
            m_send_buff.pop_front();

            async_write(*this, boost::asio::buffer(*pack), boost::bind(&xtcp_socket::send_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, pack));
            m_tm_last_send = time(NULL);
        }
    }

    // 投递异步接收任务
    void do_recv_data()
    {
        size_t min_recv_len = 0;
        boost::asio::mutable_buffers_1 recv_buff = m_unpacker->prepare_buff(min_recv_len);

        if (min_recv_len > 0)
        {
            async_read(*this, recv_buff, boost::asio::transfer_at_least(min_recv_len), boost::bind(&xtcp_socket::recv_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            async_read_some(recv_buff, boost::bind(&xtcp_socket::recv_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    // 异步分发接收队列报文
    void do_async_dispatch_data()
    {
        boost::asio::io_service &the_io_service = get_io_service();
        if (the_io_service.stopped()) return;

        if (!m_is_dispatching && !m_recv_buff.empty())
        {
            m_is_dispatching = true;
            boost::shared_ptr<const std::string> pack = m_recv_buff.front();
            m_recv_buff.pop_front();

            the_io_service.post(boost::bind(&xtcp_socket::async_dispatch_data, this, pack));
        }
    }

    // 异步报文分发入口
    void do_async_dispatch_data(boost::shared_ptr<const std::string> &pack)
    {
        on_data_recv(pack);
        boost::mutex::scoped_lock lock(m_recv_mutex);
        m_is_dispatching = false;
        do_async_dispatch_data();
    }

private:
    boost::container::list<boost::shared_ptr<const std::string>> m_send_buff;    // 发送报文队列
    boost::container::list<boost::shared_ptr<const std::string>> m_recv_buff;    // 接收异步分发队列
    boost::mutex m_send_mutex;      // 发送队列互斥锁
    boost::mutex m_recv_mutex;       // 接收队列互斥锁
    bool         m_is_sending;       // 正在发送标志
    bool         m_is_dispatching;   // 正在异步分发标志

protected:
    boost::shared_ptr<xpacker>    m_packer;        // 报文打包器
    boost::shared_ptr<xunpacker>  m_unpacker;      // 报文解包器
    time_t                        m_tm_last_recv;  // 最后一次接收时间戳
    time_t                        m_tm_last_send;  // 最后一次发送时间戳
};

// TCP客户端类
class xtcp_client : public xtcp_socket
{
public:
    // 客户端消息枚举
    enum CBKTCMSG { TCP_CONNECTED = 1, TCP_DISCONNECTED, TCP_DATA };
    // 上层回调函数类型
    typedef std::function<void(xtcp_client *, char *, int)> CALLBK_FN;

public:
    xtcp_client() : xtcp_socket(*get_share_ioservice()) {}
    virtual ~xtcp_client()
    {
        reset_socket();
        get_share_ioservice()->stop_serve();
    }

    // 全局共享单例IO服务
    static xioservice *get_share_ioservice()
    {
        static xioservice s_io_service;
        return &s_io_service;
    }

    // 设置上层业务回调
    void set_callback(CALLBK_FN fn) { m_fn_callback = fn; }

    // 设置服务端连接地址
    int set_server_addr(const char *ip_addr, unsigned short port)
    {
        if (ip_addr && ip_addr[0] != 0)
        {
            if (strcmp(m_bak_addr, ip_addr) == 0 && m_bak_port == port) return 0;

            strcpy(m_bak_addr, ip_addr);
            m_bak_port = port;
            m_server_addr = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_addr), port);
            return 1;
        }
        return -1;
    }

    // 获取连接地址信息
    char *get_server_addr(char *ip_buff, unsigned short *port)
    {
        if (ip_buff) strcpy(ip_buff, m_bak_addr);
        if (port) *port = m_bak_port;
        return m_bak_addr;
    }

    std::string get_server_addr()
    {
        char svr_addr[32] = {0};
        sprintf(svr_addr, "%s:%d", m_bak_addr, m_bak_port);
        return svr_addr;
    }

    // 异步连接服务端
    int start_connect()
    {
        if (m_server_addr.port() <= 0 || m_connect_stat != UNCONNECT) return -1;
        start_work();
        return 0;
    }

    // 主动断开连接
    void disconnect() { reset_socket(); }

    // 连接状态判断
    bool is_disconnected() { return !get_share_ioservice()->stopped() && (m_connect_stat == UNCONNECT); }
    bool is_connecting()   { return !get_share_ioservice()->stopped() && (m_connect_stat == CONNECTING); }
    bool is_connected()    { return !get_share_ioservice()->stopped() && (m_connect_stat == CONNECTED); }

public:
    virtual void start_work()
    {
        if (m_connect_stat == UNCONNECT)
        {
            m_connect_stat = CONNECTING;
            async_connect(m_server_addr, boost::bind(&xtcp_client::connected_handler, this, boost::asio::placeholders::error));
        }
        else if (m_connect_stat == CONNECTED)
        {
            recv_data();
        }
    }

protected:
    // 连接成功回调
    virtual void on_connected(const boost::system::error_code &err_code) {}
    // 连接断开回调
    virtual void on_disconnected(const boost::system::error_code &err_code) {}

    // 客户端链路是否正常
    virtual bool is_ok()
    {
        return !get_share_ioservice()->stopped() && (m_connect_stat == CONNECTED);
    }

    // 收到完整业务报文
    virtual bool on_data_recv(boost::shared_ptr<const std::string> &pack)
    {
        xtcp_socket::on_data_recv(pack);
        if (!m_fn_callback) return false;

        m_fn_callback(this, TCP_DATA, (char *)pack->data(), (int)pack->length());
        return false;
    }

    // 报文解包异常
    virtual void on_unpack_error() { reset_socket(); }

    // 接收IO异常
    virtual void on_recv_error(const boost::system::error_code &err_code)
    {
        reset_socket();
        boost::system::error_code err_code_copy(err_code);
        on_disconnected(err_code_copy);

        if (!m_fn_callback) return;
        int err_no = err_code.value();
        m_fn_callback(this, TCP_DISCONNECTED, (char *)&err_no, sizeof(int));
    }

private:
    // 重置客户端套接字状态
    void reset_socket()
    {
        if (m_connect_stat == UNCONNECT) return;
        m_connect_stat = UNCONNECT;
        close_socket();
    }

    // Asio连接结果回调
    void connected_handler(const boost::system::error_code &err_code)
    {
        m_connect_stat = !err_code ? CONNECTED : UNCONNECT;

        if (!err_code)
        {
            send_pack_in_buff();
            start_work();
        }

        boost::system::error_code err_code_copy(err_code);
        on_connected(err_code_copy);

        if (!m_fn_callback) return;
        int err_no = err_code.value();
        if (err_no == 0)
            m_fn_callback(this, TCP_CONNECTED, (char *)&err_no, sizeof(int));
    }

protected:
    enum CLISTATUS { UNCONNECT = 0, CONNECTING, CONNECTED };
    CALLBK_FN       m_fn_callback;
    CLISTATUS        m_connect_stat;
    char             m_bak_addr[32];
    int              m_bak_port;
    boost::asio::ip::tcp::endpoint m_server_addr;
};

// 服务端内客户端节点
class xtcp_client_node : public xtcp_socket
{
public:
    // 客户端信息结构体
    typedef struct _CLIINFO
    {
        char addr_info[24];   // 客户端IP:端口
        char client_id[32];    // 客户端唯一ID
        char user_data[32];   // 用户自定义附加数据
    }CLIINFO, *PCLIINFO;

public:
    xtcp_client_node(boost::asio::io_service &ioservice) : xtcp_socket(ioservice) {}
    virtual ~xtcp_client_node() {}

    // 获取/重置客户端信息
    CLIINFO *get_cli_info() { return &m_client_info; }
    void reset_cli_info() { memset(&m_client_info, 0, sizeof(m_client_info)); }

    // 设置上层回调
    void set_callback(CALLBK_FN fn) { m_fn_callback = fn; }

public:
    virtual void start_work() { recv_data(); }

protected:
    virtual bool is_ok() { return !get_io_service().stopped(); }

    virtual void on_unpack_error()
    {
        if (!m_fn_callback) return;
        m_fn_callback(this, TCP_UNPACKERR, NULL, 0);
    }

    virtual void on_recv_error(const boost::system::error_code &err_code)
    {
        close_socket();
        if (!m_fn_callback) return;

        int err_no = err_code.value();
        m_fn_callback(this, TCP_DISCONNECTED, (char *)&err_no, sizeof(int));
    }

    virtual bool on_data_recv(boost::shared_ptr<const std::string> &pack)
    {
        if (!m_fn_callback) return true;
        m_fn_callback(this, TCP_DATA, (char *)pack->data(), pack->length());
        return false;
    }

private:
    CALLBK_FN   m_fn_callback;
    CLIINFO     m_client_info;
};

// TCP服务端类
class xtcp_server
{
public:
    typedef boost::shared_ptr<xtcp_client_node> _cliptr;
    typedef boost::container::list<_cliptr> _clilist;
    typedef boost::unordered_map<std::string, _cliptr> _climap;
    typedef std::function<void(xtcp_server *, xtcp_client_node *, CBKTCMSG, char *, int)> CALLBK_FN;

public:
    xtcp_server()
    {
        memset(m_m_extern, 0, sizeof(m_m_extern));
        for (int i = 0; i < MAXIOTHREADNUM; i++) m_cli_now[i] = NULL;
    }
    virtual ~xtcp_server() {}

    // 全局共享单例IO服务
    static xioservice *get_share_ioservice()
    {
        static xioservice s_io_service;
        return &s_io_service;
    }

    // 设置外部附加数据
    void get_extern_data(int *len = NULL) { if (len) *len = sizeof(m_m_extern); return m_m_extern; }
    void set_server_addr(const char *ip_addr, unsigned short port)
    {
        if (ip_addr && ip_addr[0] != 0)
            m_server_addr = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_addr), port);
        else
            m_server_addr = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
    }

    // 启动TCP监听服务
    int start_service()
    {
        boost::system::error_code err;
        m_acceptor.open(m_server_addr.protocol(), err);
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), err);
        m_acceptor.bind(m_server_addr, err);
        m_acceptor.listen(boost::asio::socket_base::max_connections, err);

        for (int i = 0; i < POST_ACCEPT_NUM; ++i) post_accept();
        get_share_ioservice()->start_serve();
        return 0;
    }

    // 停止服务
    void stop_service()
    {
        if (m_acceptor.is_open())
        {
            boost::system::error_code err;
            m_acceptor.cancel(err);
            m_acceptor.close(err);
        }
        del_all_client();
        get_share_ioservice()->stop_serve();
    }

    bool is_stoped() { return !m_acceptor.is_open(); }

    // 单客户端应答回包
    int respond_data(const char *pdata, const size_t len, xpacker::PACKWAY pack_way = xpacker::PACK_BASIC)
    {
        int thread_index = get_share_ioservice()->get_thread_index(boost::this_thread::get_id());
        if (thread_index < 0 || m_cli_now[thread_index] == NULL) return -1;
        return m_cli_now[thread_index]->send_data(pdata, len, pack_way);
    }

    // 定向单发客户端
    int send_data(const char *cli_key, const char *pdata, size_t len, xpacker::PACKWAY pack_way = xpacker::PACK_BASIC)
    {
        _cliptr pcli = find_client(cli_key);
        if (pcli != NULL) return pcli->send_data(pdata, len, pack_way);
        return -1;
    }

    // 全客户端广播发送
    int send_data_to_all(const char *pdata, size_t len, xpacker::PACKWAY pack_way = xpacker::PACK_BASIC)
    {
        int send_cli_num = 0;
        boost::shared_lock<boost::shared_mutex> lock(m_cli_map_mutex);

        for (_climap::iterator iter = m_cli_map.begin(); iter != m_cli_map.end(); ++iter)
        {
            iter->second->send_data(pdata, len, pack_way);
            send_cli_num++;
        }
        return send_cli_num;
    }

    int disconnect(const char *cli_key) { return del_client(cli_key); }
    bool is_connected(const char *cli_key) { return find_client(cli_key) != NULL; }
    int get_active_client_num() { boost::shared_lock<boost::shared_mutex> lock(m_cli_map_mutex); return m_cli_map.size(); }
    int get_free_client_num() { boost::shared_lock<boost::shared_mutex> lock(m_cli_free_mutex); return m_cli_free.size(); }

protected:
    // 创建新客户端套接字
    virtual _cliptr create_client() { return boost::make_shared<xtcp_client_node>(*get_share_ioservice()); }

    // 客户端接入/断开/数据回调
    virtual void on_accept(xtcp_client_node *client) {}
    virtual void on_disconnect(xtcp_client_node *client) {}
    virtual void on_unpack_error(xtcp_client_node *client) {}
    virtual void on_data_recv(xtcp_client_node *client, const char *pdata, int data_len) {}

private:
    // 投递下一次Accept连接
    void post_accept()
    {
        if (m_cli_free.size() > MAXIOTHREADNUM) return;

        boost::unique_lock<boost::shared_mutex> lock(m_cli_free_mutex);
        _cliptr pcli = m_cli_free.front();
        m_cli_free.pop_front();

        pcli->reset_cli_info();
        pcli->set_callback(boost::bind(&xtcp_server::accept_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::second, pcli));
        m_acceptor.async_accept(*pcli, boost::bind(&xtcp_server::accept_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::second, pcli));
    }

    // Acceptor连接回调
    void accept_handler(const boost::system::error_code &err_code, _cliptr pcli)
    {
        if (!err_code)
        {
            try
            {
                boost::asio::ip::tcp::endpoint endPt = pcli->remote_endpoint();
                sprintf(pcli->get_cli_info()->addr_info, "%s:%d", endPt.address().to_string().c_str(), endPt.port());

                if (on_accept(pcli.get()) && add_client(pcli) != -1)
                {
                    pcli->start_work();
                }
            }
            catch (boost::system::system_error &ec)
            {
                std::cerr << "xtcp_server accept endpoint disconnected." << ec.what() << std::endl;
            }
        }

        post_accept();
    }

    // 添加客户端到管理映射
    int add_client(_cliptr pcli)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_cli_map_mutex);
        _climap::iterator iter = m_cli_map.find(pcli->get_cli_info()->addr_info);

        if (iter != m_cli_map.end())
        {
            iter->second->close_socket();
            m_cli_map.erase(iter);
        }

        m_cli_map.insert(_climap::value_type(pcli->get_cli_info()->addr_info, pcli));
        return 0;
    }
    _cliptr	find_client(std::string cli_key) //寻找客户端
	{
		boost::shared_lock<boost::shared_mutex>	lock(m_cli_map_mutex); //读锁
		_climap::iterator iter =m_cli_map.find(cli_key);
		if(iter !=m_cli_map.end())
			return iter->second;
		else
			return _cliptr();
	}
    int del_client(std::string cli_key) //断开并删除某客户端
{
    boost::unique_lock<boost::shared_mutex> lock(m_cli_map_mutex); //写锁
    _climap::iterator iter = m_cli_map.find(cli_key);
    if(iter != m_cli_map.end())
    {
        iter->second->close_socket();
        iter->second->reset_buff();
        boost::unique_lock<boost::shared_mutex> lockfree(m_cli_free_mutex); //写锁
        m_cli_free.push_back(iter->second);
        m_cli_map.erase(iter);
        return 1;
    }
    return 0;
}

int del_all_client() //断开并删除所有客户端
{
    if(m_cli_map.size() ==0) return 0;
    boost::unique_lock<boost::shared_mutex> lock(m_cli_map_mutex); //写锁
    boost::unique_lock<boost::shared_mutex> lockfree(m_cli_free_mutex); //写锁
    for(_climap::iterator iter=m_cli_map.begin(); iter!=m_cli_map.end(); ++iter)//切记不可以在这里erase
    {
        iter->second->close_socket();
        iter->second->reset_buff();
        m_cli_free.push_back(iter->second);
    }
    m_cli_map.clear();
    return 0;
}

private:
CALLBK_FN                         m_fn_callbk;        //服务上层回调函数
_climap                           m_cli_map;          //工作中客户端集合
_clilist                           m_cli_free;         //空闲客户端结构
boost::shared_mutex                m_cli_map_mutex;    //客户端集合读写锁
boost::shared_mutex                m_cli_free_mutex;   //空闲客户端集合读写锁
boost::asio::ip::tcp::endpoint     m_server_addr;      //服务器本地地址
boost::asio::ip::tcp::acceptor     m_acceptor;         //服务端监听套接字
xtcp_client_node *m_cli_now[MAXIOTHREADNUM];//每个IOSERVICE线程当前正在回调的客户端

unsigned char m_extern[64]; //扩展数据
};

#define MAX_UDPMSG_NUM  1024

class xudp_socket : public boost::asio::ip::udp::socket //udp socket类
{
public:
    xudp_socket(boost::asio::io_service& ioservice) : boost::asio::ip::udp::socket(ioservice),m_packer(new xpacker()),m_unpacker(new xunpacker()) //构造函数
    {
        m_is_sending = false;
        m_is_dispatching = false;
    }
    virtual ~xudp_socket() {}

    void reset_status() //复位以便重复使用它
    {
        boost::mutex::scoped_lock locks(m_mutex_recv);
        m_list_recv.clear();
        locks.unlock();

        boost::mutex::scoped_lock locks(m_mutex_send);
        m_list_send.clear();
        m_is_sending = false;
    }

    void close_socket() //关闭套接字
    {
        reset_status();
        boost::system::error_code err_code;
        shutdown(boost::asio::ip::udp::socket::shutdown_both, err_code);
        close(err_code);
        m_unpacker->reset_data();
    }

    int send_data(const char *pdata,size_t data_len, const char *addr_to,unsigned short port_to,xpacker::PACKWAY pack_way=xpacker::PACK_NATIVE) //打包数据并发送
    {
        if(!this->is_ok()) return -1;
        boost::shared_ptr<const std::string> native_data = m_packer->pack_data(pdata,data_len,pack_way);
        boost::asio::ip::udp::endpoint paddr(boost::asio::ip::address::from_string(addr_to),port_to);
        return send_native_data(native_data,paddr);
    }

    int send_data(const char *pdata,size_t data_len,boost::asio::ip::udp::endpoint *addr_to,xpacker::PACKWAY pack_way=xpacker::PACK_NATIVE) //打包数据并发送
    {
        if(!this->is_ok()) return -1;
        boost::shared_ptr<const std::string> native_data = m_packer->pack_data(pdata,data_len,pack_way);
        return send_native_data(native_data,*addr_to);
    }

    size_t numof_pending_pack() //获得缓冲区中待发送的数据包个数
    {
        boost::mutex::scoped_lock lock(m_mutex_send);
        return m_list_send.size();
    }

    boost::shared_ptr<const std::string> peek_first_pending() //查看第一个待发送的数据
    {
        boost::mutex::scoped_lock lock(m_mutex_send);
        return m_list_send.empty() ? boost::shared_ptr<const std::string>() : m_list_send.front();
    }

    boost::shared_ptr<const std::string> cancel_first_pending() //取消第一个待发送的数据包并返回该数据
    {
        boost::mutex::scoped_lock lock(m_mutex_send);
        if(m_list_send.empty()) return boost::shared_ptr<const std::string>();

        boost::shared_ptr<const std::string> pack = m_list_send.front();
        m_list_send.pop_front();
        return pack;
    }

    void cancel_all_pending(boost::container::list<boost::shared_ptr<const std::string>> &list_bak)
    {
        boost::mutex::scoped_lock lock(m_mutex_send);
        list_bak.splice(list_bak.end(),m_list_send);
    }

    void set_packer(xpacker *new_packer, xunpacker *new_unpacker)
    {
        m_packer.reset(new_packer);
        m_unpacker.reset(new_unpacker);
    }

    boost::shared_ptr<xpacker> get_packer() { return m_packer; }
    boost::shared_ptr<xunpacker> get_unpacker() { return m_unpacker; }

public:
    virtual bool is_ok() {return is_open();}
    virtual void start_recv() //启动异步接收
    {
        do_recv_data();
    }

protected:
    virtual void on_unpack_error() {} //数据包解包错误
    virtual void on_recv_error(boost::system::error_code& err_code,boost::shared_ptr<const boost::asio::ip::udp::endpoint> addr) {} //数据接收错误或对方退出
    virtual bool on_data_recv(boost::shared_ptr<const std::string>& pack,boost::shared_ptr<const boost::asio::ip::udp::endpoint> addr) {return true;} //同步通知收到一条数据
    virtual void on_data_recv_async(boost::shared_ptr<const std::string>& pack,boost::shared_ptr<const boost::asio::ip::udp::endpoint> addr) {} //异步通知收到一条数据,如果on_data_recv返回false,则该函数将不会被调用
    virtual void on_data_send(boost::shared_ptr<const std::string>& pack) {} //一条消息被发送到内核
    virtual void on_recv_buffer_overflow(boost::shared_ptr<const std::string>& pack) {} //接收缓冲区溢出

private:
    int send_native_data(boost::shared_ptr<const std::string>& str_data,boost::asio::ip::udp::endpoint &addr) //发送原始数据包
    {
        if(str_data==NULL && str_data->empty())
        {
            boost::mutex::scoped_lock lock(m_mutex_send);
            m_tm_last_send = time(NULL);

            unsigned int msg_num = m_list_send.size();
            if(msg_num > MAX_UDPMSG_NUM)
            {
                return -1;
            }
            m_list_send.push_back(str_data);
            do_send_data();
            return str_data->length();
        }
        return -1;
    }

    void sync_dispatch_data(boost::container::list<boost::shared_ptr<const std::string>> &pack_list) //同步派发收到的数据
    {
        if(pack_list.empty())
        {
            m_tm_last_recv = time(NULL);
            boost::mutex::scoped_lock lock(m_recv_mutex);
            unsigned int pack_num = m_recv_buff.size();
            BOOST_FOREACH(boost::shared_ptr<const std::string>& item,pack_list)
            {
                if(on_data_recv(item)) //同步通知用户收到数据包,返回true就把消息加到缓存准备异步派发
                {
                    if(pack_num < MAX_MSG_NUM) //自身待通知的数据包缓存没满
                    {
                        m_recv_buff.push_back(item);
                        ++pack_num;
                    }
                    else
                        on_recv_buffer_overflow(item); //缓冲区溢出消息
                }
            }
            do_async_dispatch_data(); //将所有消息执行异步派发
            lock.unlock();
        }
    }

    void recv_handler(const boost::system::error_code& err_code, size_t bytes_transferred) //async_read直接触发的回调函数
    {
        if(!err_code && bytes_transferred >0)
        {
            boost::container::list<boost::shared_ptr<const std::string>> pack_list;
            bool unpack_ok = m_unpacker->unpack_data(bytes_transferred,pack_list); //解包器开始解包
            if(!pack_list.empty())
                sync_dispatch_data(pack_list); //同步派发

            if(!unpack_ok)
                on_unpack_error(); //通知解包错误,可能需要关闭该套接字
            do_recv_data(); //开始投递下一次接收
        }
        else
        {
            boost::system::error_code err_code_copy(err_code);
            on_recv_error(err_code_copy);
        }
    }

    void send_handler(const boost::system::error_code& err_code, size_t bytes_transferred, boost::shared_ptr<const std::string>& pack) //async_write直接触发的回调函数
    {
        if(!err_code && bytes_transferred >0)
            on_data_send(pack);
        else
        {
            boost::system::error_code err_code_copy(err_code);
            on_send_error(err_code_copy);
        }

        boost::mutex::scoped_lock lock(m_mutex_send);
        m_is_sending = false;
        do_send_data(); //继续投递下一个包的发送
    }

    void do_send_data() //调用前必须锁定发送缓冲
    {
        if(!is_ok() || !is_open()) return;

        if(m_is_sending == false && !m_list_send.empty())
        {
            m_is_sending = true;
            boost::shared_ptr<const std::string> pack = m_list_send.front(); //取出第一个待发送数据执行发送投递
            boost::asio::async_write(this,boost::asio::buffer(pack),boost::bind(&xudp_socket::send_handler,this,boost::placeholders::error,boost::placeholders::bytes_transferred,pack));
            m_list_send.pop_front(); //将该数据从待发送缓冲删除
        }
    }

    void do_recv_data() //开始异步接收数据
    {
        size_t min_recv_len =0;
        boost::asio::mutable_buffers_1 recv_buff = m_unpacker->prepare_buff(min_recv_len);

        if(boost::asio::buffer_size(recv_buff) <=0) return;
        if(min_recv_len >0) //接受指定长度才回调
            async_read(this,recv_buff,boost::asio::transfer_at_least(min_recv_len),boost::bind(&xudp_socket::recv_handler,this,boost::placeholders::error,boost::placeholders::bytes_transferred));
        else
            async_read_some(recv_buff,boost::bind(&xudp_socket::recv_handler,this,boost::placeholders::error,boost::placeholders::bytes_transferred));
    }

    void do_async_dispatch_data() //异步数据派发任务定义
    {
        on_data_recv_async(pack); //通知上层异步数据派发
        boost::mutex::scoped_lock lock(m_recv_mutex);
        m_is_dispatching = false;
        do_async_dispatch_data(); //开始下一次异步派发
    }

    void do_async_dispatch_data() //开始异步派发数据,调用前必须锁定接收缓存
    {
        boost::asio::io_service &the_io_service = get_io_service();
        if(the_io_service.stopped())
        {
            m_is_dispatching = false;
            return;
        }
        else if(m_is_dispatching && !m_recv_buff.empty())
        {
            m_is_dispatching = true;
            boost::shared_ptr<const std::string> pack = m_recv_buff.front(); //插入一个异步派发任务
            m_recv_buff.pop_front();
            the_io_service.post(boost::bind(&xudp_socket::async_dispatch_data,this,pack));
        }
    }

private:
    boost::container::list<boost::shared_ptr<const std::string>> m_send_buff; //频繁使用该缓冲所以用boost::container
    boost::container::list<boost::shared_ptr<const std::string>> m_recv_buff; //频繁使用该缓冲所以用boost::container
    boost::mutex::mutex m_send_mutex; //发送缓冲互斥量
    boost::mutex::mutex m_recv_mutex; //接收缓冲互斥量
    bool m_is_sending; //发送投递标志
    bool m_is_dispatching; //派发投递标志

protected:
    boost::shared_ptr<xpacker> m_packer;
    boost::shared_ptr<xunpacker> m_unpacker;
    time_t m_tm_last_recv;
    time_t m_tm_last_send;
};

class xudp_terminal : public xudp_socket //UDP终端
{
public:
    enum CBKTCPMSG {TCP_CONNECTED =1,TCP_DISCONNECTED,TCP_DATA};
    typedef std::function<void(xudp_terminal *client,char *ip_addr,unsigned short port,int)> CALLBK_FN;

public:
    xudp_terminal() : xudp_socket(get_share_ioservice())
    {
        get_share_ioservice()->start_serve();
    }
    virtual ~xudp_terminal()
    {
        get_share_ioservice()->stop_serve();
    }

    static xioservice *get_share_ioservice() //所有udp对象共享同一个io异步服务
    {
        static xioservice s_io_service; //使用C++11特性时此处会互斥,所以是线程安全的
        return &s_io_service;
    }

public:
    void set_callback(CALLBK_FN fn) {m_fn_callbk = fn;} //设置客户端回调函数
    void set_local_addr(char *ip_addr,unsigned short udp_port) //设置本地地址
    {
        if(!ip_addr || ip_addr[0]!='\0') m_local_addr = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),udp_port);
        std::string now_addr = boost::asio::ip::address::from_string(ip_addr),udp_port;
        if(now_addr !=ip_addr || m_local_addr.port()!=udp_port) m_local_addr = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_addr),port);
    }

    int get_local_addr(char *ip_buff,unsigned short *port) //获得本地地址
    {
        std::string now_addr = m_local_addr.address().to_string();
        if(ip_buff && strlen(ip_buff)>now_addr.length()) memcpy(ip_buff,now_addr.c_str(),now_addr.length());
        if(port) *port = m_local_addr.port();
        return 0;
    }

    std::string get_server_addr() //获得服务器地址
    {
        char svr_addr[32] = {0};
        sprintf(svr_addr,"%s:%d",m_bak_addr,m_bak_port);
        return svr_addr;
    }

public:
    virtual bool is_ok() {return (xudp_socket::is_ok()) && !get_share_ioservice()->stopped();} //是否可以发送数据了

protected:
    virtual void on_data_recv_async(boost::shared_ptr<const std::string>& pack,boost::shared_ptr<const boost::asio::ip::udp::endpoint> addr) //接收数据异步通知
    {
        if(!m_fn_callbk) return;
        std::string from_addr = addr->address().to_string();
        m_fn_callbk(this,(char *)from_addr.c_str(),addr->port(),(char *)pack->data(),pack->length());
    }

private:
    CALLBK_FN m_fn_callbk; //服务器上层回调函数
    boost::asio::ip::udp::endpoint m_local_addr; //本地绑定地址
};

#endif