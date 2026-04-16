#ifndef USBADAPTER_PACKAGE_H_H
#define USBADAPTER_PACKAGE_H_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "xbasicasio.hpp"
#include "xpackage.hpp"
#include "mgr_log.h"
#include "lib_interface.h"

#define USB_ADAPTER_HEADER      0xFFAS    //USB adapter包头标识
#define USB_ADAPTER_HEADER_LEN  14        //USB adapter包头长度
#define USB_ADAPTER_MAXMSG_LEN  40960     //USB adapter消息最大包长

//USB二进制报文打包器
class usbadapter_bin_packer : public xpacker
{
public:
    usbadapter_bin_packer();
    virtual ~usbadapter_bin_packer();

    virtual boost::shared_ptr<const std::string> pack_data(const char *pdata,size_t datalen,PACKWAY pack_way);
};

//USB二进制报文解包器
class usbadapter_bin_unpacker : public xunpacker
{
public:
    usbadapter_bin_unpacker();
    virtual ~usbadapter_bin_unpacker();

public:
    virtual void reset_data();
    virtual bool unpack_data(size_t bytes_data, boost::container::list<boost::shared_ptr<const std::string>> &list_pack);
    virtual boost::asio::mutable_buffers_1 prepare_buff(size_t& min_recv_len); //准备下一个数据接收缓冲区
};

//USB适配器完整报文封装类
class xusbadapter_package : public xpacket
{
public:
    //键值读写数据结构体
    class rw_data
    {
    public:
        int         tid;
        int         length;
        std::string value;
    };

    boost::unordered_map<int,boost::shared_ptr<data_value>> m_data_map;

    //芯片结温结构体
    class junction_temp
    {
    public:
        int     m_asicchip_id;
        float   m_temp;
    };

    //电源开关控制结构体
    class power_switch
    {
    public:
        power_switch()
        {
            m_utp40_id = 0;
            m_switch = 0;
        }
        ~power_switch(){}

        int     m_utp40_id;
        int     m_switch;
    };

    //频点通道设置结构体
    class frequency_set
    {
    public:
        frequency_set()
        {
            m_frequency = 0;
            m_channel = 0;
        }
        ~frequency_set(){}

        float   m_frequency; //频率
        int     m_channel;  //通道
    };

public:
    //报文类型枚举
    enum MSG_TYPE {MT_REQUEST=0x01, MT_NOTIFY=0x02, MT_RESPOND=0x81};
    //报文命令字枚举
    enum MSG_CMD {MC_NONE=0x00,MC_START=0x01,MC_QUERY,MC_CANCEL,MC_COMPLETE,MC_DATA,MC_HEARTBEAT,MC_CAL_READ,MC_USBDSTSerials_ID,MC_ASIC_JUNCTION_TEMP,MC_GET_VALUE,MC_COMMAND,MC_FREQUENCY,MC_GET_UTP_REGISTER,MC_GET_UTP102_REGISTER};
    //报文错误码枚举
    enum MSG_ERR {ME_SUCCESS=0x00, ME_FAILED=0x01};

    usbadapter_package(MSG_TYPE msg_type=MT_REQUEST,MSG_CMD msg_cmd=MC_NONE,unsigned short version=0,unsigned int session=0);
    usbadapter_package(const unsigned char *bin_data,int data_len);
    usbadapter_package(const xusbadapter_package& other);
    usbadapter_package& operator=(const xusbadapter_package& other);
    virtual xpacket *clone(); //克隆对象
    virtual ~xusbadapter_package();

public:
    virtual bool parse_from_bin(unsigned char *pack_data,int pack_len); //从BIN解析数据包
    virtual std::string serial_to_bin(); //串行化成BIN
    virtual void reset();

    virtual bool need_confirm()     {return (m_msg_type == MT_REQUEST);}    //需要确认并重传的包
    virtual bool type_confirm()     {return (m_msg_type == MT_RESPOND);}   //确认应答包
    virtual int flag_confirm()     {return m_session;}                    //获取确认报文匹配标志

private:
    bool parse_msg_header(unsigned char *pack_data,int pack_len);

    bool parse_request_body(MSG_CMD msg_cmd,unsigned char *pack_data,int pack_len);
    bool parse_notify_body(MSG_CMD msg_cmd,unsigned char *pack_data,int pack_len);
    bool parse_response_body(MSG_CMD msg_cmd,unsigned char *pack_data,int pack_len);

    bool parse_heartbeat_body(unsigned char *pack_data,int pack_len);

    bool parse_start_request_body(unsigned char *pack_data,int pack_len);
    bool parse_query_request_body(unsigned char *pack_data,int pack_len);
    bool parse_cancel_request_body(unsigned char *pack_data,int pack_len);
    bool parse_complete_notify_body(unsigned char *pack_data,int pack_len);
    bool parse_data_request_body(unsigned char *pack_data,int pack_len);
    bool parse_cal_read_request_body(unsigned char *pack_data,int pack_len);
    bool parse_usbdstids_request_body(unsigned char *pack_data,int pack_len);
    bool parse_junction_temp_request_body(unsigned char *pack_data,int pack_len);
    bool parse_get_value_request_body(unsigned char *pack_data,int pack_len);
    bool parse_power_switch_request_body(unsigned char *pack_data,int pack_len);
    bool parse_frequency_set_request_body(unsigned char *pack_data,int pack_len);
    bool parse_get_utp_register_request_body(unsigned char *pack_data,int pack_len);

    bool parse_start_response_body(unsigned char *pack_data,int pack_len);
    bool parse_query_response_body(unsigned char *pack_data,int pack_len);
    bool parse_cancel_response_body(unsigned char *pack_data,int pack_len);
    bool parse_data_response_body(unsigned char *pack_data,int pack_len);
    bool parse_cal_read_response_body(unsigned char *pack_data,int pack_len);
    bool parse_usbdstids_response_body(unsigned char *pack_data,int pack_len);
    bool parse_junction_temp_response_body(unsigned char *pack_data,int pack_len);
    bool parse_get_value_response_body(unsigned char *pack_data,int pack_len);
    bool parse_power_switch_response_body(unsigned char *pack_data,int pack_len);
    bool parse_frequency_set_response_body(unsigned char *pack_data,int pack_len);
    bool parse_get_utp_register_response_body(unsigned char *pack_data,int pack_len);

    std::string serail_request(MSG_CMD msg_cmd);
    std::string serail_notify(MSG_CMD msg_cmd);
    std::string serail_response(MSG_CMD msg_cmd);

    void serial_msg_header(unsigned char *pack_header);
    void serial_heartbeat(std::string& pack_data);

    void serial_start_request(std::string& pack_data);
    void serial_query_request(std::string& pack_data);
    void serial_cancel_request(std::string& pack_data);
    void serial_complete_notify(std::string& pack_data);
    void serial_data_request(std::string& pack_data);
    void serial_cal_read_request(std::string& pack_data);
    void serial_usbdstids_request(std::string& pack_data);
    void serial_junction_temp_request(std::string& pack_data);
    void serial_get_value_request(std::string& pack_data);
    void serial_power_switch_request(std::string& pack_data);
    void serial_frequency_set_request(std::string& pack_data);
    void serial_get_utp_register_request(std::string& pack_data);

    void serial_start_response(std::string& pack_data);
    void serial_query_response(std::string& pack_data);
    void serial_cancel_response(std::string& pack_data);
    void serial_data_response(std::string& pack_data);
    void serial_cal_read_response(std::string& pack_data);
    void serial_usbdstids_response(std::string& pack_data);
    void serial_junction_temp_response(std::string& pack_data);
    void serial_get_value_response(std::string& pack_data);
    void serial_power_switch_response(std::string& pack_data);
    void serial_frequency_set_response(std::string& pack_data);
    void serial_get_utp_register_response(std::string& pack_data);

private:
    bool is_ota_caltype(int ota_type);

public:
    MSG_TYPE                        m_msg_type;         //报文类型
    MSG_CMD                         m_msg_cmd;          //报文命令字
    unsigned short                  m_version;          //协议版本号
    unsigned int                    m_session;          //请求响应匹配会话ID

    int                             m_ota_type;
    long long                       m_ota_version;
    int                             m_usb_dst;
    int                             m_upgrade_progress;
    int                             m_total_len;
    std::string                     m_calfilename;      //校准文件名

    int                             m_err_code;
    int                             m_value_type;

    int                             m_command_type;     //command操作类型
    power_switch                    m_command_value;    //command操作值

    int                             m_utp_chip_id;      //UTP芯片编号

    rw_data                         m_rw_data;
    std::vector<int>                m_usbdst_ids;
    std::vector<junction_temp>      m_junctions;
    std::string                     m_value_dev;

    int                             m_frequency_type;   //frequency操作类型
    frequency_set                   m_frequency_value;   //frequency操作值

    boost::shared_mutex             m_mux_sid;          //会话id读写互斥锁
};

#endif