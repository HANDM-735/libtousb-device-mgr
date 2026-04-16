#pragma once
#include <string>
#include <vector>
#include "xbasic.hpp"
#include "mgr_log.h"

#define UTP_REGISTER_NUM     11    // UTP 寄存器数量
#define UTP102_REGISTER_NUM  11    // UTP102 寄存器数量

// PCB温度结构体
class pcb_temp
{
public:
    pcb_temp() {}
    ~pcb_temp() {}

public:
    int     m_id = 0;
    float   m_temp_value = 0.0;
};

// 合路温度结构体
class comb_temp
{
public:
    comb_temp() {}
    ~comb_temp() {}

public:
    int     m_id = 0;
    float   m_temp_value = 0.0;
};

// PCB电压结构体
class pcb_volatage
{
public:
    pcb_volatage() {}
    ~pcb_volatage() {}

public:
    int     m_id = 0;
    float   m_volt_value = 0.0;
};

// AD9528锁相状态结构体
class ad9528_lock
{
public:
    ad9528_lock() { m_lockstatus = 1; }
    ~ad9528_lock() {}

public:
    int                     m_lockstatus;
    std::vector<float>      m_chnpwm_vct;
};

// AD9545锁相状态结构体
class ad9545_lock
{
public:
    ad9545_lock() { m_lockstatus = 1; }
    ~ad9545_lock() {}

public:
    int                     m_lockstatus;
    std::vector<float>      m_chnpwm_vct;
};

// PWM通道检测结构体
class pwm_check
{
public:
    pwm_check() {}
    ~pwm_check() {}

public:
    int m_channel_id = 0;
    int m_pwm_value = 0;
};

// Serdes链路状态结构体
class serdes_state
{
public:
    serdes_state() {}
    ~serdes_state() {}

public:
    short           m_id = 0;
    unsigned char   m_serdes_value = 0;
};

// UTP寄存器信息结构体
class utp_register_info
{
public:
    utp_register_info()
    {
        m_valid     = -1;   // 默认-1无效
        m_reg_ch    = -1;   // 默认通道-1
        m_reg_addr  = -1;   // 默认地址
        m_reg_data  = -1;   // 寄存器默认值
    }
    ~utp_register_info() {}

public:
    int8_t      m_valid;        // 寄存器是否有效
    uint8_t     m_reg_ch;       // 通道号
    uint16_t    m_reg_addr;     // 寄存器地址
    uint16_t    m_reg_data;     // 寄存器值
};

// UTP40芯片寄存器集合
class utp40_register
{
public:
    utp40_register() {}
    ~utp40_register() {}

public:
    short                                   m_utp40_chip_id = 0;    // utp40 芯片连续编号
    std::vector<utp_register_info>         m_register;              // 寄存器信息数组
};

// 二进制报文解析转换工具类
class xconvert
{
public:
    xconvert() {}
    ~xconvert() {}

public:
    /**
     * @brief 将输入字符串value中的二进制温度数据解析出来放到数组中
     * @param value  该字符串里面存的是二进制数据
     * @param vct_temp 存放从value中解析出来的温度数据
     * @return 0 表示解析成功；-1 表示解析失败
     */
    static int parse_pcbtemp_data(const std::string &value, std::vector<pcb_temp> &vct_temp)
    {
        // xbasic::debug_output("Enter into xconvert::parse_pcbtemp_data()\n");
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_pcbtemp_data()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            // xbasic::debug_output("xconvert::parse_pcbtemp_data() value is empty\n");
            LOG_MSG(WRN_LOG, "xconvert::parse_pcbtemp_data() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            int num = xbasic::read_bigendian((const void *)(reinterpret_cast<const void *>(value_ptr)), 4);
            // xbasic::debug_output("xconvert::parse_pcbtemp_data() num=%d", num);
            LOG_MSG(MSG_LOG, "xconvert::parse_pcbtemp_data() num=%d", num);

            if ((num * 4) != (length - 4))
            {
                // xbasic::debug_output("xconvert::parse_pcbtemp_data() value is invalid,length=%d num=%d\n", length, num);
                LOG_MSG(ERR_LOG, "xconvert::parse_pcbtemp_data() value is invalid,length=%d num=%d", length, num);
                ret = -1;
            }
            else
            {
                int i = 0;
                char *pay_data = (char *)&value_ptr[4];
                while (i < num)
                {
                    float tmp = xbasic::readFloat_bigendian(reinterpret_cast<const void *>(pay_data + i * 4), 4);
                    pcb_temp temp;
                    temp.m_id = ++i;
                    temp.m_temp_value = tmp;
                    vct_temp.push_back(temp);
                }
            }
        }

        // xbasic::debug_output("Exited xconvert::parse_pcbtemp_data() vct_temp size=%d ret=%d\n", vct_temp.size(), ret);
        LOG_MSG(MSG_LOG, "Exited xconvert::parse_pcbtemp_data() vct_temp size=%d ret=%d", vct_temp.size(), ret);
        return ret;
    }

    /**
     * @brief 将输入字符串value中的二进制电压数据解析出来放到数组中
     * @param value  该字符串里面存的是二进制数据
     * @param vct_vol 存放从value中解析出来的电压数据
     * @return 0 表示解析成功；-1 表示解析失败
     */
    static int parse_pcbvolt_data(const std::string &value, std::vector<pcb_volatage> &vct_vol)
    {
        // xbasic::debug_output("Enter into xconvert::parse_pcbvolt_data()\n");
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_pcbvolt_data()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            // xbasic::debug_output("xconvert::parse_pcbvolt_data() value is empty\n");
            LOG_MSG(WRN_LOG, "xconvert::parse_pcbvolt_data() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            int num = xbasic::read_bigendian((const void *)(reinterpret_cast<const void *>(value_ptr)), 4);
            // xbasic::debug_output("xconvert::parse_pcbvolt_data() num=%d", num);
            LOG_MSG(MSG_LOG, "xconvert::parse_pcbvolt_data() num=%d", num);

            if ((num * 4) != (length - 4))
            {
                // xbasic::debug_output("xconvert::parse_pcbvolt_data() value is invalid,length=%d num=%d\n", length, num);
                LOG_MSG(ERR_LOG, "xconvert::parse_pcbvolt_data() value is invalid,length=%d num=%d", length, num);
                ret = -1;
            }
            else
            {
                int i = 0;
                char *pay_data = (char *)&value_ptr[4];
                while (i < num)
                {
                    float vol = xbasic::readFloat_bigendian(reinterpret_cast<const void *>(pay_data + i * 4), 4);
                    pcb_volatage voltage;
                    voltage.m_id = ++i;
                    voltage.m_volt_value = vol;
                    vct_vol.push_back(voltage);
                }
            }
        }

        // xbasic::debug_output("Exited xconvert::parse_pcbvolt_data() vct_vol size=%d ret=%d\n", vct_vol.size(), ret);
        LOG_MSG(MSG_LOG, "Exited xconvert::parse_pcbvolt_data() vct_vol size=%d ret=%d", vct_vol.size(), ret);
        return ret;
    }

    // id(2个字节) + 温度(4个字节float)
    static int parse_comb_temp(const std::string &value, std::vector<comb_temp> &vct_temp)
    {
        // xbasic::debug_output("Enter into xconvert::parse_comb_temp()\n");
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_comb_temp()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            // xbasic::debug_output("xconvert::parse_comb_temp() value is empty");
            LOG_MSG(WRN_LOG, "xconvert::parse_comb_temp() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            if ((length % 6) != 0)
            {
                // xbasic::debug_output("xconvert::parse_comb_temp() value is invalid,length=%d", length);
                LOG_MSG(ERR_LOG, "xconvert::parse_comb_temp() value is invalid,length=%d", length);
                ret = -1;
            }
            else
            {
                int offset = 0;
                char *pay_data = (char *)value_ptr;
                while (offset < length)
                {
                    unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
                    offset += 2;
                    float tmp = xbasic::readFloat_bigendian(&pay_data[offset], 4);
                    offset += 4;

                    comb_temp temp;
                    temp.m_id = id;
                    temp.m_temp_value = tmp;
                    vct_temp.push_back(temp);
                }
            }
        }

        // xbasic::debug_output("Exited xconvert::parse_comb_temp() vct_temp size=%d ret=%d", vct_temp.size(), ret);
        LOG_MSG(MSG_LOG, "Exited xconvert::parse_comb_temp() vct_temp size=%d ret=%d", vct_temp.size(), ret);
        return ret;
    }

    static int parse_ad9528_lockdata(const std::string &value, std::vector<ad9528_lock> &vct_lock)
    {
        // xbasic::debug_output("Enter into xconvert::parse_ad9528_lockdata()\n");
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_ad9528_lockdata()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            // LOG_MSG(WRN_LOG, "xconvert::parse_ad9528_lockdata() value is empty");
            LOG_MSG(WRN_LOG, "xconvert::parse_ad9528_lockdata() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            int length = value.length();

            int num = xbasic::read_bigendian((const void *)(reinterpret_cast<const void *>(value_ptr)), 4);
            int i = 0;
            char *pay_data = (char *)&value_ptr[4];

            if (num == 0)
            {
                ret = -1;
                // xbasic::debug_output("xconvert::parse_ad9545_lockdata() chip size=%d\n", num);
                LOG_MSG(WRN_LOG, "xconvert::parse_ad9545_lockdata() chip size=%d", num);
            }

            for (int iread = 0; iread < (length - 4);)
            {
                int ch = xbasic::read_bigendian(&pay_data[iread], 4);
                int lockstatus = xbasic::read_bigendian(&pay_data[iread + 4], 4);
                ad9528_lock ad9528_lck;
                ad9528_lck.m_lockstatus = lockstatus;

                for (int ich = 0; ich < ch; ++ich)
                {
                    float pwm_val = xbasic::readFloat_bigendian(&pay_data[iread + 8 + ich * 4], 4);
                    ad9528_lck.m_chnpwm_vct.push_back(pwm_val);
                }

                iread += (8 + (ch * 4));
                vct_lock.push_back(ad9528_lck);
            }

            // xbasic::debug_output("xconvert::parse_ad9528_lockdata() pwm size=%d ch=%d iread=%d locks size=%d", ad9528_lck.m_chnpwm_vct.size(), ch, iread, vct_lock.size());
        }

        // xbasic::debug_output("Exited xconvert::parse_ad9528_lockdata()");
        LOG_MSG(MSG_LOG, "Exited xconvert::parse_ad9528_lockdata()");
        return ret;
    }

    static int parse_ad9545_lockdata(const std::string &value, std::vector<ad9545_lock> &ad9545_lck)
    {
        // xbasic::debug_output("Enter into xconvert::parse_ad9545_lockdata()");
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_ad9545_lockdata()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            LOG_MSG(WRN_LOG, "xconvert::parse_ad9545_lockdata() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            int length = value.length();
            char *pay_data = (char *)value_ptr;

            int ch = xbasic::read_bigendian(&pay_data[0], 4);
            if (ch == 0)
            {
                ret = -1;
                // xbasic::debug_output("xconvert::parse_ad9545_lockdata() channel size=%d", ch);
                LOG_MSG(ERR_LOG, "xconvert::parse_ad9545_lockdata() channel size=%d", ch);
            }
            else if ((8 + ch * 4) != length)
            {
                // 数据长度错误
                LOG_MSG(ERR_LOG, "xconvert::parse_ad9545_lockdata() value is invalid length=%d channel=%d", length, ch);
                ret = -1;
            }
            else
            {
                int lockstatus = xbasic::read_bigendian(&pay_data[4], 4);
                ad9545_lck.m_lockstatus = lockstatus;

                for (int ich = 0; ich < ch; ich++)
                {
                    float pwm_val = xbasic::readFloat_bigendian(&pay_data[8 + ich * 4], 4);
                    ad9545_lck.m_chnpwm_vct.push_back(pwm_val);
                }
            }
        }

        LOG_MSG(MSG_LOG, "Exited xconvert::parse_ad9545_lockdata() ret=%d", ret);
        return ret;
    }

    static int parse_pwm_check(const std::string &value, std::vector<pwm_check> &vct_pwm)
    {
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_pwm_check()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            LOG_MSG(ERR_LOG, "xconvert::parse_pwm_check() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            int length = value.length();
            char *pay_data = (char *)value_ptr;

            int ch = xbasic::read_bigendian(&pay_data[0], 4);
            if ((4 + ch * 4) != length)
            {
                // 数据长度错误
                LOG_MSG(ERR_LOG, "xconvert::parse_pwm_check() value is invalid length=%d channel=%d", length, ch);
                ret = -1;
            }
            else
            {
                for (int ich = 0; ich < ch; ich++)
                {
                    int pwm_val = xbasic::read_bigendian(&pay_data[4 + ich * 4], 4);
                    LOG_MSG(MSG_LOG, "xconvert::parse_pwm_check() pwm_val=%d", pwm_val);

                    pwm_check pwm;
                    pwm.m_channel_id = ++ich;
                    pwm.m_pwm_value = pwm_val;
                    vct_pwm.push_back(pwm);
                }
            }
        }

        LOG_MSG(MSG_LOG, "Exited xconvert::parse_pwm_check() vct_pwm size=%d ret=%d", vct_pwm.size(), ret);
        return ret;
    }

    static int parse_serdes_state(const std::string &value, std::vector<serdes_state> &vct_serdes)
    {
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_serdes_state()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            LOG_MSG(ERR_LOG, "xconvert::parse_serdes_state() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            if ((length % 3) != 0)
            {
                LOG_MSG(ERR_LOG, "xconvert::parse_serdes_state() value is invalid,length=%d", length);
                ret = -1;
            }
            else
            {
                int offset = 0;
                char *pay_data = (char *)value_ptr;
                while (offset < length)
                {
                    unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
                    offset += 2;
                    unsigned char state = xbasic::read_bigendian(&pay_data[offset], 1);
                    offset += 1;

                    serdes_state serdes_tmp;
                    serdes_tmp.m_id = id;
                    serdes_tmp.m_serdes_value = state;
                    vct_serdes.push_back(serdes_tmp);
                }
            }
        }

        LOG_MSG(MSG_LOG, "Exited xconvert::parse_serdes_state() vct_serdes size=%d ret=%d", vct_serdes.size(), ret);
        return ret;
    }

    static int parse_utp40_register(const std::string &value, std::vector<utp40_register> &vct_register)
    {
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_utp40_register()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            LOG_MSG(ERR_LOG, "xconvert::parse_utp40_register() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            // 其中 2 字节chip id，6字节一个寄存器信息
            if (length % (2 + 6 * UTP_REGISTER_NUM) != 0)
            {
                LOG_MSG(ERR_LOG, "xconvert::parse_utp40_register() value is invalid,length=%d", length);
                ret = -1;
            }
            else
            {
                int offset = 0;
                char *pay_data = (char *)&value_ptr[0];
                while (offset < length)
                {
                    utp40_register u_register;
                    unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
                    u_register.m_utp40_chip_id = id;
                    offset += 2;

                    for (int i = 0; i < UTP_REGISTER_NUM; i++)
                    {
                        // 读取该芯片通道上的寄存器信息
                        utp_register_info info;
                        int valid = xbasic::read_bigendian(&pay_data[offset], 1); offset += 1;
                        int reg_ch = xbasic::read_bigendian(&pay_data[offset], 1); offset += 1;
                        int reg_addr = xbasic::read_bigendian(&pay_data[offset], 2); offset += 2;
                        int reg_data = xbasic::read_bigendian(&pay_data[offset], 2); offset += 2;

                        // 填充信息
                        info.m_valid = valid;
                        info.m_reg_ch = reg_ch;
                        info.m_reg_addr = reg_addr;
                        info.m_reg_data = reg_data;

                        u_register.m_register.push_back(info);
                    }
                    vct_register.push_back(u_register);
                }
            }
        }

        LOG_MSG(MSG_LOG, "Exited xconvert::parse_utp40_register()");
        return ret;
    }

    static int parse_utp102_register(const std::string &value, std::vector<utp40_register> &vct_register)
    {
        LOG_MSG(MSG_LOG, "Enter into xconvert::parse_utp102_register()");
        int ret = 0;

        if (value.empty())
        {
            ret = -1;
            LOG_MSG(ERR_LOG, "xconvert::parse_utp102_register() value is empty");
        }
        else
        {
            const char *value_ptr = value.data();
            size_t length = value.length();

            // 其中 2 字节chip id，6字节一个寄存器信息
            if (length % (2 + 6 * UTP102_REGISTER_NUM) != 0)
            {
                LOG_MSG(ERR_LOG, "xconvert::parse_utp102_register() value is invalid,length=%d", length);
                ret = -1;
            }
            else
            {
                int offset = 0;
                char *pay_data = (char *)&value_ptr[0];
                while (offset < length)
                {
                    utp40_register u_register;
                    unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
                    u_register.m_utp40_chip_id = id;
                    offset += 2;

                    for (int i = 0; i < UTP102_REGISTER_NUM; i++)
                    {
                        // 读取该芯片通道上的寄存器信息
                        utp_register_info info;
                        int valid = xbasic::read_bigendian(&pay_data[offset], 1); offset += 1;
                        int reg_ch = xbasic::read_bigendian(&pay_data[offset], 1); offset += 1;
                        int reg_addr = xbasic::read_bigendian(&pay_data[offset], 2); offset += 2;
                        int reg_data = xbasic::read_bigendian(&pay_data[offset], 2); offset += 2;

                        // 填充信息
                        info.m_valid = valid;
                        info.m_reg_ch = reg_ch;
                        info.m_reg_addr = reg_addr;
                        info.m_reg_data = reg_data;

                        u_register.m_register.push_back(info);
                    }
                    vct_register.push_back(u_register);
                }
            }
        }

        LOG_MSG(MSG_LOG, "Exited xconvert::parse_utp102_register()");
        return ret;
    }
};