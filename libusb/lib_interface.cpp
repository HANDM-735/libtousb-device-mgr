#include <string>
#include <string.h>
#include <boost/filesystem.hpp>
#include "lib_interface.h"
#include "mgr_session.h"
#include "mgr_network.h"
#include "xconvert.hpp"
#include "mgr_log.h"
#include "xconfig.hpp"

#define CONFIG_DIR "/userdata/config/libtousb-device-mgr/"

//READ,WRITE,ALARM报文命令字的tlv的type值
#define RWA_TEMP             0x0001
#define RWA_VOLTAGE          0x0002
#define RWA_ELE_CURRENT      0x0003
#define RWA_TEMP_SET         0x0010
#define RWA_TEMP_ALARM       0x0011
#define RWA_POWER_STATUS     0x0020
#define RWA_POS_STATUS       0x0021
#define RWA_INPUT_STATUS     0x0023
#define RWA_OUTPUT_STATUS    0x0024
#define RWA_BOARD_TYPE       0x0050
#define RWA_BOARD_SN         0x0051
#define RWA_MANU_INFO        0x0052
#define RWA_HW_VER           0x0053
#define RWA_SW_VER           0x0054
#define RWA_SLOT             0x0056
#define RWA_REPORT_CYCLE     0x0058
#define RWA_COMM_STATUS      0x0059
#define RWA_TOTAL_RUNTIME    0x005A

//下面仅用于装备自检的数据字段
#define RWA_SERDES_STATUS    0x0062
#define RWA_AD9528_LOCKSTATUS 0x0063
#define RWA_AD9545_LOCKSTATUS 0x0064
#define RWA_PWM_CHECK        0x0065

#define REAL_STR_DEF(s) #s
#define STR(s) REAL_STR_DEF(s)

typedef enum
{
    VALUE_PCB_TMP = 0,
    VALUE_UTP40_TMP,
    VALUE_FPGA_TMP,    //预留，暂不支持
    VALUE_ASIC_TMP,    //其他接口实现，暂不使用
    VALUE_TEMP,
    VALUE_VOL,
    VALUE_CURRENT,    //未支持
    VALUE_BOARD_ID,
    VALUE_DIG_EXIST,
    VALUE_PPS_EXIST,
    VALUE_ASIC_EXIST,
    VALUE_PGB_EXIST,
    VALUE_SYNC_EXIST,
    VALUE_BOARD_EXIST,

    VALUE_SYNC_ID,
    VALUE_PGB_ID,
    VALUE_PPS_ID,
    VALUE_ASIC_ID,
    VALUE_DIG_ID,
    VALUE_GET_ID,

    VALUE_SYNC_SN,
    VALUE_PGB_SN,
    VALUE_PPS_SN,
    VALUE_ASIC_SN,

    VALUE_SYNC_HW_VER,
    VALUE_PGB_HW_VER,
    VALUE_PPS_HW_VER,
    VALUE_ASIC_HW_VER,

    VALUE_SYNC_SOFT_VER,
    VALUE_PGB_SOFT_VER,
    VALUE_PPS_SOFT_VER,
    VALUE_ASIC_SOFT_VER,

    VALUE_DIG_LOGIC_VER,
    VALUE_RCAFPGA_TEMP,
    VALUE_9528_STATUS,
    VALUE_9545_STATUS,
    VALUE_PWM_FREQUENCY,
    VALUE_SERDES_STATUS,

    VALUE_UTP40_REGISTER,
    VALUE_UTP102_REGISTER,

    VALUE_TYPE_MAX
} value_type;

const char* lib_version()
{
#if defined(SOFT_VERSION) && defined(DEV_TYPE)
    const char* version_info = "version_module:" STR(SOFT_VERSION) "(" __DATE__ " " __TIME__ ") " "(" STR(DEV_TYPE) ")";
#elif defined(SOFT_VERSION)
    const char* version_info = "version_module:" STR(SOFT_VERSION) "(" __DATE__ " " __TIME__ ") " "(" "general" ")";
#elif defined(DEV_TYPE)
    const char* version_info = "version_module:" "unknow" "(" __DATE__ " " __TIME__ ") " "(" STR(DEV_TYPE) ")";
#else
    const char* version_info = "version_module:" "unknow" "(" __DATE__ " " __TIME__ ") " "(" "general" ")";
#endif
    return version_info;
}

static bool judge_type_serialid(int ota_type, int serialid)
{
    bool ret = false;
    switch(ota_type)
    {
        //PGB板主FPGA
        case OTA_TYPE_PGB_M_FPGA:
        //PGB板从FPGA
        case OTA_TYPE_PGB_D_FPGA:
        //PGB板单片机
        case OTA_TYPE_PGB_MCU:
        // FT PGB 板上的UTP40校准文件
        case OTA_TYPE_FTPGBUTP40_MV_CAL:
        case OTA_TYPE_FTPGBUTP40_MI_CAL:
        case OTA_TYPE_FTPGBUTP40_FV_CAL:
        case OTA_TYPE_FTPGBUTP40_FI_CAL:
        case OTA_TYPE_FTPGBUTP40_ADC_CAL:
        case OTA_TYPE_FTPGBUTP40_IC_CAL:
            ret = ((serialid & 0x00FF) == BOARDTYPE_FT_PGB);
            break;

        //PPS板主FPGA
        case OTA_TYPE_PPS_M_FPGA:
        //PPS板从FPGA
        case OTA_TYPE_PPS_D_FPGA:
        //PPS板单片机
        case OTA_TYPE_PPS_MCU:
        // FT PPS 板UTP40校准文件
        case OTA_TYPE_FTPPSUTP40_MV_CAL:
        case OTA_TYPE_FTPPSUTP40_MI_CAL:
        case OTA_TYPE_FTPPSUTP40_FV_CAL:
        case OTA_TYPE_FTPPSUTP40_FI_CAL:
        case OTA_TYPE_FTPPSUTP40_ADC_CAL:
        case OTA_TYPE_FTPPSUTP40_IC_CAL:
            ret = ((serialid & 0x00FF) == BOARDTYPE_FT_PPS);
            break;

        //SYNC板FPGA
        case OTA_TYPE_SYNC_FPGA:
        //SYNC板单片机
        case OTA_TYPE_SYNC_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_FT_SYNC);
            break;

        //ASIC板主FPGA
        case OTA_TYPE_ASIC_M_FPGA:
        //ASIC板从FPGA
        case OTA_TYPE_ASIC_D_FPGA:
        //ASIC板单片机
        case OTA_TYPE_ASIC_MCU:
        //FEB板上PE幅值校准文件
        case OTA_TYPE_FTFEB_PE_AC_CAL:
        //FEB板上PE相位校准文件
        case OTA_TYPE_FTFEB_PE_DC_CAL:
            ret = ((serialid & 0x00FF) == BOARDTYPE_ASIC);
            break;

        case OTA_TYPE_CAL_FILE:
            ret = true;
            break;

        //CP设备类型
        case OTA_TYPE_CPPGB_ZU11_M_FPGA:
        case OTA_TYPE_CPPGB_S_FPGA:
        case OTA_TYPE_CPPGB_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_PGB);
            break;

        //CPDPS板主FPGA
        case OTA_TYPE_CPDPS_M_FPGA:
        case OTA_TYPE_CPDPS_CCCU_FPGA:
        //CPDPS板从FPGA
        case OTA_TYPE_CPDPS_S_FPGA:
        case OTA_TYPE_CPDPS_CPSU_FPGA:
        case OTA_TYPE_CPDPS_MCU:
        // CP DPS 板UTP40校准文件
        case OTA_TYPE_CPDPSUTP40_MV_CAL:
        case OTA_TYPE_CPDPSUTP40_MI_CAL:
        case OTA_TYPE_CPDPSUTP40_FV_CAL:
        case OTA_TYPE_CPDPSUTP40_FI_CAL:
        case OTA_TYPE_CPDPSUTP40_ADC_CAL:
        case OTA_TYPE_CPDPSUTP40_IC_CAL:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_DPS);
            break;

        //CPSYNC板的FPGA
        case OTA_TYPE_CPSYNC_FPGA:
        case OTA_TYPE_CPSYNC_CXBU_FPGA:
        case OTA_TYPE_CPSYNC_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_SYNC);
            break;

        //CPPEM板200T的FPGA
        case OTA_TYPE_CPPEM_FPGA:
        case OTA_TYPE_CPPEM_CPMU_FPGA:
        case OTA_TYPE_CPPEM_MCU:
        case OTA_TYPE_CPRCA_MCU:
        // CP PEM 板UTP40校准文件
        case OTA_TYPE_CPPEMUTP40_MV_CAL:
        case OTA_TYPE_CPPEMUTP40_MI_CAL:
        case OTA_TYPE_CPPEMUTP40_FV_CAL:
        case OTA_TYPE_CPPEMUTP40_FI_CAL:
        case OTA_TYPE_CPPEMUTP40_ADC_CAL:
        case OTA_TYPE_CPPEMUTP40_IC_CAL:
        // CP PEM 板UTP102校准文件
        case OTA_TYPE_CPPEMUTP102_MV_CAL:
        case OTA_TYPE_CPPEMUTP102_MI_CAL:
        case OTA_TYPE_CPPEMUTP102_FV_CAL:
        case OTA_TYPE_CPPEMUTP102_FI_CAL:
        //PEM板上PE幅值校准文件
        case OTA_TYPE_CPPEM_PE_AC_CAL:
        //PEM板上PE相位校准文件
        case OTA_TYPE_CPPEM_PE_DC_CAL:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_PEM);
            break;

        //FT TH监控板MCU
        case OTA_TYPE_FTTH_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_TH_MONITOR);
            break;
        //FT MF监控板MCU
        case OTA_TYPE_FTMF_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_MF_MONITOR);
            break;

        //CP TH监控板MCU
        case OTA_TYPE_CPTH_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_TH_MONITOR);
            break;
        //CP MF监控板MCU
        case OTA_TYPE_CPMF_MCU:
            ret = ((serialid & 0x00FF) == BOARDTYPE_MF_MONITOR);
            break;

        //CPRCA板FPGA
        case OTA_TYPE_CPRCA_CAFU_FPGA:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_RCA);
            break;
        //CPDIG板FPGA
        case OTA_TYPE_CPDIG_CDCU_FPGA:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_DIG);
            break;
        //CPPEM板1ST的FPGA
        case OTA_TYPE_CPPEM_CPDS_FPGA:
        //CPPEM板AGFB的FPGA
        case OTA_TYPE_CPPEM_CPGM_FPGA:
            ret = ((serialid & 0x00FF) == BOARDTYPE_CP_PEM);
            break;

        //FTFEB_FPGM的FPGA
        case OTA_TYPE_FTFEB_FPGM_FPGA:
        //FTFEB_FPDS的FPGA
        case OTA_TYPE_FTFEB_FPDS_FPGA:
        //FTDIG_FDCU的FPGA
        case OTA_TYPE_FTDIG_FDCU_FPGA:
            ret = ((serialid & 0x00FF) == BOARDTYPE_ASIC);
            break;
    }
    return ret;
}

static void set_real_data(struct real_data* data_ptr, boost::shared_ptr<xusbadapter_package::rw_data> rwdata)
{
    if(data_ptr == NULL)
    {
        return;
    }

    boost::unordered_map<int,boost::shared_ptr<xusbadapter_package::rw_data::data_value> >::iterator iter = rwdata->m_data_map.begin();
    boost::unordered_map<int,boost::shared_ptr<xusbadapter_package::rw_data::data_value> >::iterator end = rwdata->m_data_map.end();
    for(iter != end; iter++)
    {
        boost::shared_ptr<xusbadapter_package::rw_data::data_value> item = iter->second;
        switch(item->tid)
        {
            case RWA_TEMP:
            {
                int len = item->length;
                data_ptr->temperature_ptr = new char[len];
                memset(data_ptr->temperature_ptr,0, len);
                memcpy(data_ptr->temperature_ptr,item->value.data(),len);
                break;
            }
            case RWA_VOLTAGE:
            {
                int len = item->length;
                data_ptr->voltage_ptr = new char[len];
                memset(data_ptr->voltage_ptr,0, len);
                memcpy(data_ptr->voltage_ptr,item->value.data(),len);
                break;
            }
            case RWA_ELE_CURRENT:
            {
                int len = item->length;
                data_ptr->current_ptr = new char[len+1];
                memset(data_ptr->current_ptr,0, len+1);
                strncpy(data_ptr->current_ptr,item->value.c_str(),len);
                break;
            }
            case RWA_TEMP_SET:
            {
                int len = item->length;
                data_ptr->temp_range_ptr = new char[len+1];
                memset(data_ptr->temp_range_ptr,0, len+1);
                strncpy(data_ptr->temp_range_ptr,item->value.c_str(),len);
                break;
            }
            case RWA_TEMP_ALARM:
            {
                int len = item->length;
                data_ptr->temp_alarm_ptr = new char[len+1];
                memset(data_ptr->temp_alarm_ptr,0, len+1);
                strncpy(data_ptr->temp_alarm_ptr,item->value.c_str(),len);
                break;
            }
            case RWA_INPUT_STATUS:
            {
                int len = item->length;
                data_ptr->input_io_status = new char[len+1];
                memset(data_ptr->input_io_status,0, len+1);
                strncpy(data_ptr->input_io_status,item->value.c_str(),len);
                break;
            }
            case RWA_OUTPUT_STATUS:
            {
                int len = item->length;
                data_ptr->output_io_status = new char[len+1];
                memset(data_ptr->output_io_status,0, len+1);
                strncpy(data_ptr->output_io_status,item->value.c_str(),len);
                break;
            }
            case RWA_POWER_STATUS:
            {
                int len = item->length;
                int st = xbasic::read_bigendian(const_cast<char*>(item->value.c_str()),1);
                std::string val = std::to_string(st);
                memset(data_ptr->power_status,0, sizeof(data_ptr->power_status));
                strncpy(data_ptr->power_status,val.c_str(),val.length());
                break;
            }
            case RWA_POS_STATUS:
            {
                int len = item->length;
                int st = xbasic::read_bigendian(const_cast<char*>(item->value.c_str()),1);
                std::string val = std::to_string(st);
                memset(data_ptr->board_status,0, sizeof(data_ptr->board_status));
                strncpy(data_ptr->board_status,val.c_str(),val.length());
                break;
            }
            case RWA_BOARD_TYPE:
            {
                int len = item->length;
                int boardtype = xbasic::read_bigendian(const_cast<char*>(item->value.c_str()),1);
                std::string val = std::to_string(boardtype);
                memset(data_ptr->board_type,0, sizeof(data_ptr->board_type));
                strncpy(data_ptr->board_type,val.c_str(),val.length());
                break;
            }
            case RWA_BOARD_SN:
            {
                int len = item->length;
                memset(data_ptr->board_sn,0, sizeof(data_ptr->board_sn));
                strncpy(data_ptr->board_sn,item->value.c_str(),len);
                break;
            }
            case RWA_MANU_INFO:
            {
                int len = item->length;
                memset(data_ptr->vendor_info,0, sizeof(data_ptr->vendor_info));
                strncpy(data_ptr->vendor_info,item->value.c_str(),len);
                break;
            }
            case RWA_HW_VER:
            {
                int len = item->length;
                memset(data_ptr->hardware_ver,0, sizeof(data_ptr->hardware_ver));
                strncpy(data_ptr->hardware_ver,item->value.c_str(),len);
                break;
            }
            case RWA_SW_VER:
            {
                int len = item->length;
                memset(data_ptr->software_ver,0, sizeof(data_ptr->software_ver));
                strncpy(data_ptr->software_ver,item->value.c_str(),len);
                break;
            }
            case RWA_SLOT:
            {
                int len = item->length;
                int slotid = xbasic::read_bigendian(const_cast<char*>(item->value.c_str()),1);
                std::string val = std::to_string(slotid);
                memset(data_ptr->slot_id,0, sizeof(data_ptr->slot_id));
                strncpy(data_ptr->slot_id,val.c_str(),val.length());
                break;
            }
            case RWA_REPORT_CYCLE:
            {
                int len = item->length;
                memset(data_ptr->report_cycle,0, sizeof(data_ptr->report_cycle));
                strncpy(data_ptr->report_cycle,item->value.c_str(),len);
                break;
            }
            case RWA_COMM_STATUS:
            {
                int len = item->length;
                memset(data_ptr->status,0, sizeof(data_ptr->status));
                strncpy(data_ptr->status,item->value.c_str(),len);
                break;
            }
            case RWA_TOTAL_RUNTIME:
            {
                int len = item->length;
                memset(data_ptr->accumulative_time,0, sizeof(data_ptr->accumulative_time));
                strncpy(data_ptr->accumulative_time,item->value.c_str(),len);
                break;
            }
            case RWA_AD9528_LOCKSTATUS:
            {
                int len = item->length;
                memset(data_ptr->ad9528_ppl_lockstatus,0, sizeof(data_ptr->ad9528_ppl_lockstatus));
                memcpy(data_ptr->ad9528_ppl_lockstatus,item->value.data(),len);
                data_ptr->ad9528_buff_len = len;
                break;
            }
            case RWA_AD9545_LOCKSTATUS:
            {
                int len = item->length;
                memset(data_ptr->ad9545_ppl_lockstatus,0, sizeof(data_ptr->ad9545_ppl_lockstatus));
                memcpy(data_ptr->ad9545_ppl_lockstatus,item->value.data(),len);
                data_ptr->ad9545_buff_len = len;
                break;
            }
            case RWA_PWM_CHECK:
            {
                int len = item->length;
                memset(data_ptr->pwm_check,0, sizeof(data_ptr->pwm_check));
                memcpy(data_ptr->pwm_check,item->value.data(),len);
                data_ptr->pwm_buff_len = len;
                break;
            }
            case RWA_SERDES_STATUS:
            {
                int len = item->length;
                memset(data_ptr->serdes_val,0, sizeof(data_ptr->serdes_val));
                memcpy(data_ptr->serdes_val,item->value.data(),len);
                data_ptr->serdes_val_len = len;
                break;
            }
        }
    }
    return ;
}

static std::string parseConfigIniFile(const std::string& filename, const std::string& keyword)
{
    std::string value;
    std::ifstream file(filename);
    std::string line;

    if(!file.is_open())
    {
        LOG_MSG(ERR_LOG, "parseConfigIniFile() Error: failed to open %s", filename.c_str());
    }
    else
    {
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '[') continue;
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

            if (line.find(keyword) != std::string::npos)
            {
                size_t pos = line.find('=');
                if (pos != std::string::npos && pos + 1 < line.size())
                {
                    value = line.substr(pos + 1);
                    value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
                    break;
                }
            }
        }
    }
    return value;
}

static int get_loglevel(const std::string& loglevel)
{
    int level = MSG_LOG;
    if(loglevel == std::string("debug")) {
        level = MSG_LOG;
    } else if(loglevel == std::string("warning")) {
        level = WRN_LOG;
    } else if(loglevel == std::string("error")) {
        level = ERR_LOG;
    } else {
        level = MSG_LOG;
    }
    return level;
}

static int get_logmode(const std::string& logmode)
{
    int mode = LOG_MODE_NORMAL;
    if(logmode == std::string("normal")) {
        mode = LOG_MODE_NORMAL;
    } else if(logmode == std::string("debug")) {
        mode = LOG_MODE_DEBUG;
    } else if(logmode == std::string("verbose")) {
        mode = LOG_MODE_VERBOSE;
    } else {
        mode = LOG_MODE_NORMAL;
    }
    return mode;
}

/**
 * @brief 确保目录存在，如果不存在则创建
 * @param dir_path 目录路径
 * @return true 如果目录存在或创建成功，false 如果失败
 */
// static bool ensure_directory_exists(const boost::filesystem::path& dir_path)
// {
//     try {
//         // 检查目录是否存在
//         if (boost::filesystem::exists(dir_path)) {
//             // 如果路径存在，检查它是否是目录
//             if (boost::filesystem::is_directory(dir_path)) {
//                 xbasic::debug_output("ensure_directory_exists(): dir_path:%s exists!\n", dir_path.c_str());
//                 return true;
//             } else {
//                 xbasic::debug_output("ensure_directory_exists(): the_path exists, but not dir!\n", dir_path.c_str());
//                 return false;
//             }
//         }
//
//         // 目录不存在，尝试创建（包括所有父目录）
//         if (boost::filesystem::create_directories(dir_path)) {
//             xbasic::debug_output("ensure_directory_exists(): dir_path:%s create success!\n", dir_path.c_str());
//             return true;
//         } else {
//             xbasic::debug_output("ensure_directory_exists(): dir_path:%s create fail!!\n", dir_path.c_str());
//             return false;
//         }
//
//     } catch (const boost::filesystem::filesystem_error& e) {
//         xbasic::debug_output("ensure_directory_exists(): boost:filesystem error: %s\n", e.what());
//         return false;
//     } catch (const std::exception& e) {
//         xbasic::debug_output("ensure_directory_exists(): exception error. %s\n", e.what());
//         return false;
//     }
// }

static void load_config() //加载配置文件
{
    std::string module_path = xbasic::get_module_path();
    if(module_path.back() != '/')
    {
        module_path += std::string("/");
    }
    std::string cfg_file = module_path + std::string("libusb_config.ini");

    std::string loglevel;
    std::string logmode;

    if(access(cfg_file.c_str(), F_OK) != 0)
    {
        loglevel = std::string("warning");
        logmode = std::string("normal");
    }
    else
    {
        xini_config xini_cfg;
        xini_cfg.set_file(std::string(xbasic::get_module_path())+"libusb_config.ini");
        loglevel = xini_cfg.get_data("SYS_CONFIG.log_level","warning");
        logmode = xini_cfg.get_data("SYS_CONFIG.log_mode","normal");
    }

    xbasic::debug_output("load_config() log_level=%s log_mode=%s\n",loglevel.c_str(),logmode.c_str());

    xconfig *sys_config = xconfig::get_instance();
    sys_config->set_data("log_level",get_loglevel(loglevel));
    sys_config->set_data("log_mode",get_logmode(logmode));
}

/**
 * 功能：启动日志模块
 */
static void start_log()
{
    mgr_log  *log_mgr = mgr_log::get_instance();
    xconfig  *sys_config = xconfig::get_instance();

    //获取module_path
    std::string module_path = xbasic::get_module_path();
    if(module_path.back() != '/')
    {
        module_path += std::string("/");
    }

    //设置log
    std::string log_path = module_path + std::string("libusb_log");
    //创建log目录
    int ret = xbasic::creat_dir(log_path.c_str());
    if(ret != 0)
    {
        xbasic::debug_output("start_log() log dir:%s, creat failed\n", log_path.c_str());
    }

    std::string log_prefix = log_path+std::string("/")+std::string("LIBUSB");
    std::string log_filename = log_path+std::string("/")+std::string("libusb.log");
    int loglevel = std::stol(sys_config->get_data("log_level"));
    int logmode = std::stol(sys_config->get_data("log_mode"));

    xbasic::debug_output("start_log() log_filename=%s\n",log_filename.c_str());
    xbasic::debug_output("start_log() log_prefix=%s\n",log_prefix.c_str());
    xbasic::debug_output("start_log() loglevel=%ld\n",loglevel);
    xbasic::debug_output("start_log() logmode=%ld\n",logmode);

    log_mgr->log_config(log_filename.c_str(), log_prefix.c_str(), loglevel, logmode, 50000);

    //设置log work线程工作周期为1毫秒
    int work_cycle_ms = 1;
    log_mgr->start_work(work_cycle_ms);
    ussleep(10*1000);

    return ;
}

static void stop_log()
{
    mgr_log  *log_mgr = mgr_log::get_instance();
    log_mgr->stop_work();
    return ;
}

/**
 * 参数: adapter_srv 设备管理服务端地址，格式: ip:port
 *       如果adapter_srv为NULL时，则从/userdata/config/libtousb-device-mgr/g_libtousb.conf文件读取
 *       文件格式如下
 *       libserver_ip_addr=ip:port
 * 返回值: 0: 执行成功 非0: 执行失败
 */
int usb_init(const char* adapter_srv)
{
    //加载配置文件
    load_config();
    //启动日志模块必须是第一个模块
    start_log();

    char srv_addr[32] = {0};
    int len = 0;
    memset(&srv_addr[0],0,sizeof(srv_addr));

    if((adapter_srv == NULL) || (strlen(adapter_srv) == 0))
    {
        std::string cfg_file = std::string(CONFIG_DIR)+"g_libtousb.conf";
        std::string libserver_ip_addr = parseConfigIniFile(cfg_file,"libserver_ip_addr");
        if (!libserver_ip_addr.empty())
        {
            xbasic::trim(libserver_ip_addr);
            len = libserver_ip_addr.length();
            strncpy(&srv_addr[0],libserver_ip_addr.c_str(),len);
            // xbasic::debug_output("usb_init() libserver_ip_addr=%s len=%d\n", srv_addr,len);
            LOG_MSG(MSG_LOG, "usb_init() libserver_ip_addr=%s len=%d\n", srv_addr,len);
        }
        else
        {
            // xbasic::debug_output("usb_init() read configure file failed,please check /etc/g_libtousb.conf file\n");
            LOG_MSG(ERR_LOG, "usb_init() read configure file failed,please check /etc/g_libtousb.conf file\n");
            return USB_ERR_NETWORK;
        }
    }
    else
    {
        len = strlen(adapter_srv);
        strncpy(&srv_addr[0],adapter_srv,len);
        // xbasic::debug_output("usb_init() adapter_srv=%s\n", srv_addr);
        LOG_MSG(MSG_LOG, "usb_init() adapter_srv=%s", srv_addr);
    }

    mgr_session *session_mgr = mgr_session::get_instance();
    session_mgr->start_work(20);
    mgr_network *network_mgr = mgr_network::get_instance();

    network_mgr->set_adapter_addr(srv_addr);
    network_mgr->start_work(500);
    sleep(5);

    if (network_mgr->is_init_ok())
    {
        return USB_ERR_NONE;
    }

    return USB_ERR_NETWORK;
}

/*
*功能: 卸载usb lib库
*参数: 无
*返回值: 0
*/
int usb_uninit()
{
    sleep(5);
    mgr_network *network_mgr = mgr_network::get_instance();
    network_mgr->stop_work();
    mgr_session *session_mgr = mgr_session::get_instance();
    session_mgr->stop_work();
    stop_log(); // 停止日志模块
    return USB_ERR_NONE;
}


//usb ota开始升级接口函数
int usb_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id)
{
    int errcode = 0;
    std::string str_return;

    //判断ota的type类型与对应串口的id是否一致，
    //防止升级文件到错误的单片机串口上
    if(judge_type_serialid(ota_type,usb_device_addr_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_start_upgrade(ota_type,version_num,usb_device_addr_id,errcode,str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    return errcode;
}


//usb ota取消升级接口函数
int usb_ota_cancel_upgrade(int ota_type, int usb_device_addr_id)
{
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(ota_type,usb_device_addr_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_cancel_upgrade(ota_type,usb_device_addr_id,errcode,str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    return errcode;
}

//usb ota升级进度接口函数
int usb_ota_query_progress(int ota_type, int usb_device_addr_id,float* progress)
{
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(ota_type,usb_device_addr_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_query_upgrade(ota_type, usb_device_addr_id,errcode, str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    *progress = std::stof(str_return);
    // xbasic::debug_output("usb_ota_query_progress()  ota_type=%d usb_device_addr_id=%d progress=%0.2f.\n",ota_type,usb_device_addr_id,*progress);
    LOG_MSG(MSG_LOG, "usb_ota_query_progress() ota_type=%d usb_device_addr_id=%d progress=%f",ota_type,usb_device_addr_id,*progress);

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    return errcode;
}

//usb ota升级完成接口函数
int usb_ota_complete_upgrade(int ota_type, int usb_device_addr_id)
{
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(ota_type,usb_device_addr_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_complete_upgrade(ota_type,usb_device_addr_id,errcode,str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    return errcode;
}

//usb设备的实时数据获取接口函数
struct real_data* usb_fetch_real_data(int usb_device_addr_id)
{
    int errcode = 0;
    boost::shared_ptr<xusbadapter_package::rw_data> rw_data;
    if (mgr_session::get_instance()->usb_fetch_real_data(usb_device_addr_id,errcode, rw_data) < 0)
    {
        return NULL;
    }

    // xbasic::debug_output("usb_fetch_real_data() usb_device_addr_id=%d.\n",usb_device_addr_id);
    LOG_MSG(MSG_LOG, "usb_fetch_real_data() usb_device_addr_id=%d",usb_device_addr_id);

    if (errcode < 0)
    {
        return NULL;
    }

    real_data *ret_data = NULL;
    if(rw_data->m_data_map.size() != 0)
    {
        ret_data = new real_data();
        set_real_data(ret_data,rw_data);
    }

    return ret_data;
}

//usb设备的实时数据内存释放接口函数
void usb_free_real_data(struct real_data* real_data_ptr)
{
    if(real_data_ptr != NULL)
    {
        //to do free internal struct memory
        if(real_data_ptr->temperature_ptr != NULL)
        {
            delete []real_data_ptr->temperature_ptr;
            real_data_ptr->temperature_ptr = NULL;
        }

        if(real_data_ptr->voltage_ptr != NULL)
        {
            delete []real_data_ptr->voltage_ptr;
            real_data_ptr->voltage_ptr = NULL;
        }

        if(real_data_ptr->current_ptr != NULL)
        {
            delete []real_data_ptr->current_ptr;
            real_data_ptr->current_ptr = NULL;
        }

        if(real_data_ptr->temp_range_ptr != NULL)
        {
            delete []real_data_ptr->temp_range_ptr;
            real_data_ptr->temp_range_ptr = NULL;
        }

        if(real_data_ptr->temp_alarm_ptr != NULL)
        {
            delete []real_data_ptr->temp_alarm_ptr;
            real_data_ptr->temp_alarm_ptr = NULL;
        }

        if(real_data_ptr->input_io_status != NULL)
        {
            delete []real_data_ptr->input_io_status;
            real_data_ptr->input_io_status = NULL;
        }

        if(real_data_ptr->output_io_status != NULL)
        {
            delete []real_data_ptr->output_io_status;
            real_data_ptr->output_io_status = NULL;
        }

        delete real_data_ptr;
        real_data_ptr = NULL;
    }

    return ;
}

int cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id)
{
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(ota_type,usb_device_addr_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_cal_read_file(ota_type,version_num,usb_device_addr_id,errcode,str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    return errcode;
}

int usb_get_boardids(struct boardids* borad_ids)
{
    int errcode = 0;
    std::set<int> serialids;
    if (mgr_session::get_instance()->call_usb_serials_ids(serialids,errcode) < 0)
    {
        return -1;
    }

    int id_size = serialids.size();
    // xbasic::debug_output("usb_get_boardids() size=%d.\n",id_size);
    LOG_MSG(MSG_LOG,"usb_get_boardids() size=%d",id_size);

    if (errcode < 0)
    {
        return -1;
    }

    if(id_size != 0)
    {
        std::set<int>::iterator it = serialids.begin();
        int i = 0;
        for(; it != serialids.end(); it++)
        {
            if(i < sizeof(borad_ids->ids)/sizeof(borad_ids->ids[0]))
            {
                borad_ids->ids[i] = *it;
                i++ ;
            }
        }
    }

    return id_size;
}

//读取asic芯片结温接口函数
int usb_get_asicjunct_temp(struct asic_juncttemp* juncts, int size)
{
    int errcode = 0;
    std::vector<xusbadapter_package::junction_temp> juncts_temp;
    if (mgr_session::get_instance()->call_asic_juctions_temp(juncts_temp,errcode) < 0)
    {
        return -1;
    }

    int temp_size = juncts_temp.size();
    // xbasic::debug_output("usb_get_asicjunct_temp() size=%d.\n",temp_size);
    LOG_MSG(MSG_LOG, "usb_get_asicjunct_temp() size=%d",temp_size);

    if (errcode < 0)
    {
        return -1;
    }

    if(temp_size != 0)
    {
        std::vector<xusbadapter_package::junction_temp>::iterator it = juncts_temp.begin();
        int i = 0;
        for(; it != juncts_temp.end(); it++)
        {
            if(i < size)
            {
                juncts[i].asic_id = it->m_asicchip_id;
                juncts[i].temp   = it->m_temp;
                i++ ;
            }
        }
    }

    return temp_size;
}

/*
*功能: 根据boardid获取PCB的温度
*参数: dstid, 单片机识别, slot+板类型组成  0x121 对应slot1 板类型0x21是sync板
*参数: value, 字符串, 获取到的值存放, 数据格式: 1: value1, 2: value2, ..., n:valuen
*返回值: 0: 成功, -1: fail
*/
int usb_get_pcb_temp(int dstid, std::string& value)
{
    int errcode = 0;
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PCB_TMP, errcode, value) < 0)
    {
        return -1;
    }

    // xbasic::debug_output("usb_get_pcb_temp() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "usb_get_pcb_temp() value=%s",value.c_str());

    if (errcode < 0)
    {
        return -1;
    }

    return 0;
}

/*
*功能: 根据boardid获取UTP40的温度
*参数: dstid, 单片机识别, slot+板类型组成  0x121 对应slot1 板类型0x21是sync板
*参数: value, 字符串, 获取到的值存放, 数据格式: 1: value1, 2: value2, ..., n:valuen
*返回值: 0: 成功, -1: fail
*/
int usb_get_utp40_temp(int dstid, std::string& value)
{
    int errcode = 0;
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_UTP40_TMP, errcode, value) < 0)
    {
        return -1;
    }

    // xbasic::debug_output("usb_get_utp40_temp() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "usb_get_utp40_temp() value=%s",value.c_str());

    if (errcode < 0)
    {
        return -1;
    }

    return 0;
}

/*
*功能: 根据boardid获取板的温度
*参数: dstid, 单片机识别, slot+板类型组成  0x121 对应slot1 板类型0x21是sync板
*参数: value, 字符串, 获取到的值存放, 数据格式: 1: value1, 2: value2, ..., n:valuen
*返回值: 0: 成功, -1: fail
*/
int usb_get_board_temp(int dstid, std::string& value)
{
    int errcode = 0;
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_TEMP, errcode, value) < 0)
    {
        // xbasic::debug_output("usb_get_board_temp() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "usb_get_board_temp() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("usb_get_board_temp() value_size=%d, value=%s\n",value.size(),value.c_str());
    LOG_MSG(MSG_LOG, "usb_get_board_temp() value_size=%d, value=%s",value.size(),value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("usb_get_board_temp() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "usb_get_board_temp() errcode:%d", errcode);
        return -1;
    }

    return 0;
}

/*
*功能: 根据boardid获取板的电压
*参数: dstid, 单片机识别, slot+板类型组成  0x121 对应slot1 板类型0x21是sync板
*参数: value, 字符串, 获取到的值存放, 数据格式: 1: value1, 2: value2, ..., n:valuen
*返回值: 0: 成功, -1: fail
*/
int usb_get_board_vol(int dstid, std::string& value)
{
    int errcode = 0;
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_VOL, errcode, value) < 0) {
        // xbasic::debug_output("usb_get_board_vol() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "usb_get_board_vol() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("usb_get_board_vol() value_size()=%d, value=%s\n",value.size(),value.c_str());
    LOG_MSG(MSG_LOG, "usb_get_board_vol() value_size()=%d, value=%s",value.size(),value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("usb_get_board_vol() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "usb_get_board_vol() errcode:%d", errcode);
        return -1;
    }

    return 0;
}

/**
*功能: 根据 serial_num 查找对应的温度
*参数:
* base_value: 客户端接收到的原始的二进制数据
* serial_num: 位号
* tmp: 输出参数, 返回温度值
*返回值: 0 表示查找成功; -1 表示查找失败
*/
int find_temp_by_serialnum(const std::string& base_value, int serial_num, double &tmp)
{
    int ret = -1;
    // xbasic::debug_output("Enter into find_temp_by_serialnum()\n");
    LOG_MSG(MSG_LOG, "Enter into find_temp_by_serialnum()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<comb_temp> vct_temp;
    int r = xconvert::parse_comb_temp(base_value, vct_temp);
    if(r != 0)
    {
        // 数据解析失败直接返回
        // xbasic::debug_output("Exited find_temp_by_serialnum() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited find_temp_by_serialnum() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_temp 中了
        // 2. 遍历数组查找对应的温度并返回
        for(auto &it : vct_temp)
        {
            if(it.m_id == serial_num)
            {
                // 找到了
                tmp = it.m_temp_value;
                ret = 0;
                break;
            }
        }
    }
    // xbasic::debug_output("Exited find_temp_by_serialnum() serial_num: %d, temp: %f, ret: %d\n", serial_num, tmp, ret);
    LOG_MSG(MSG_LOG, "Exited find_temp_by_serialnum() serial_num: %d, temp: %f, ret: %d", serial_num, tmp, ret);
    return ret;
}

/**
*功能: 批量获取温度
*参数:
* base_value: 客户端收到的原始二进制数据
* begin_num: 开始位号所对应的序号(从1开始)
* end_num: end num: 结束位号所对应的序号
* mul_tmp: 输出参数, 存放温度数据
*返回值: 0 表示检索成功; -1 表示检索失败
*/
int find_temp_from_start_to_end(const std::string& base_value, int begin_num, int end_num, struct multiple_temp *mul_tmp)
{
    // xbasic::debug_output("Enter into find_temp_from_start_to_end()\n");
    LOG_MSG(MSG_LOG, "Enter into find_temp_from_start_to_end() begin_num:%d, end_num:%d", begin_num, end_num);
    int ret = 0;
    // 1. 先对原始的二进制数据进行解析
    std::vector<comb_temp> vct_temp;
    int r = xconvert::parse_comb_temp(base_value, vct_temp);
    if(r != 0)
    {
        ret = -1;
        // 数据解析失败直接返回
        // xbasic::debug_output("Exited find_temp_from_start_to_end() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited find_temp_from_start_to_end() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_temp 中了
        // 2. 遍历数组查找对应的温度并返回
        memset(mul_tmp, 0, sizeof(struct multiple_temp));
        for(auto &it : vct_temp)
        {
            if(it.m_id >= begin_num && it.m_id <= end_num)
            {
                mul_tmp->arry[mul_tmp->sz].m_id = it.m_id;
                mul_tmp->arry[mul_tmp->sz].m_temp_value = it.m_temp_value;
                ++(mul_tmp->sz);
            }
        }
    }
    // xbasic::debug_output("Exited find_temp_from_start_to_end() find_size: %d, ret: %d\n", mul_tmp->sz, ret);
    LOG_MSG(MSG_LOG, "Exited find_temp_from_start_to_end() find_size: %d, ret: %d", mul_tmp->sz, ret);
    return ret;
}

/**
*功能: 根据 serial_num 查找对应的电压
*参数:
* base_value: 客户端接收到的原始的二进制数据
* serial_num: 位号
* tmp: 输出参数, 返回温度值
*返回值: 0 表示查找成功; -1 表示查找失败
*/
int find_voltage_by_serialnum(const std::string& base_value, int serial_num, double &voltage)
{
    int ret = -1;
    // xbasic::debug_output("Enter into find_voltage_by_serialnum()\n");
    LOG_MSG(MSG_LOG, "Enter into find_voltage_by_serialnum()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<pcb_voltage> vct_vol;
    int r = xconvert::parse_pcbvolt_data(base_value, vct_vol);
    if(r != 0)
    {
        // 数据解析失败直接返回
        // xbasic::debug_output("Exited find_voltage_by_serialnum() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited find_voltage_by_serialnum() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_vol 中了
        // 2. 遍历数组查找对应的温度并返回
        for(auto &it : vct_vol)
        {
            if(it.m_id == serial_num)
            {
                // 找到了
                voltage = it.m_volt_value;
                ret = 0;
                break;
            }
        }
    }
    // xbasic::debug_output("Exited find_voltage_by_serialnum() serial_num: %d, voltage: %lf\n.", serial_num, voltage);
    LOG_MSG(MSG_LOG, "Exited find_voltage_by_serialnum() serial_num: %d, voltage: %lf", serial_num, voltage);
    return ret;
}

/**
*功能: 批量获取电压
*参数:
* base_value: 客户端收到的原始二进制数据
* begin_num: 开始位号所对应的序号(从1开始)
* end_num: end num: 结束位号所对应的序号
* mul_vol: 输出参数, 存放电压数据
*返回值: 0 表示检索成功; -1 表示检索失败
*/
int find_voltage_from_start_to_end(const std::string& base_value, int begin_num, int end_num, struct multiple_voltage *mul_vol)
{
    // xbasic::debug_output("Enter into find_voltage_from_start_to_end()\n");
    LOG_MSG(MSG_LOG, "Enter into find_voltage_from_start_to_end()");
    int ret = 0;
    // 1. 先对原始的二进制数据进行解析
    std::vector<pcb_voltage> vct_vol;
    int r = xconvert::parse_pcbvolt_data(base_value, vct_vol);
    if(r != 0)
    {
        ret = -1;
        // 数据解析失败直接返回
        // xbasic::debug_output("Exited find_voltage_from_start_to_end() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited find_voltage_from_start_to_end() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_vol 中了
        // 2. 遍历数组查找对应的温度并返回
        memset(mul_vol, 0, sizeof(struct multiple_voltage));
        for(auto &it : vct_vol)
        {
            if(it.m_id >= begin_num && it.m_id <= end_num)
            {
                mul_vol->arry[mul_vol->sz].m_id = it.m_id;
                mul_vol->arry[mul_vol->sz].m_voltage_value = it.m_volt_value;
                ++(mul_vol->sz);
            }
        }
    }
    // xbasic::debug_output("Exited find_voltage_from_start_to_end() find_size: %d, ret: %d\n", mul_vol->sz, ret);
    LOG_MSG(MSG_LOG, "Exited find_voltage_from_start_to_end() find_size: %d, ret: %d", mul_vol->sz, ret);
    return ret;
}

std::string find_string_by_serialnum(std::string str, int serial_num)
{
    std::vector<std::string> vct_str_new;
    int key = 0;

    if (str.empty())
    {
        return "";
    }

    int count_str_new =xbasic::split_string(str,",",&vct_str_new);
    for(int i=0; i<count_str_new; i++)
    {
        std::vector<std::string> vct_value;
        int count_value =xbasic::split_string(vct_str_new[i],":",&vct_value);
        if (count_value != 2)
        {
            // xbasic::debug_output("find_string_by_serialnum() input string is error,please check!!!\n");
            LOG_MSG(ERR_LOG, "find_string_by_serialnum() input string is error,please check!!!");
            return "";
        }
        else
        {
            int key = std::stoi(vct_value[0]);
            if (key == serial_num)
            {
                return vct_value[1];
            }
        }
    }

    return "";
}

std::string find_string_from_start_to_end(std::string str, int begin_num, int end_num)
{
    std::string find_data;
    std::vector<std::string> vct_str_new;
    int key = 0;
    int count_str_new =xbasic::split_string(str,",",&vct_str_new);
    for(int i=0; i<count_str_new; i++)
    {
        std::vector<std::string> vct_value;
        int count_value =xbasic::split_string(vct_str_new[i],":",&vct_value);
        if (count_value != 2)
        {
            // xbasic::debug_output("find_string_from_start_to_end() input string is error,please check!!!\n");
            LOG_MSG(ERR_LOG, "find_string_from_start_to_end() input string is error,please check!!!");
            continue;
        }
        else
        {
            int key = std::stoi(vct_value[0]);
            if (key >= begin_num && (key <= end_num))
            {
                find_data += vct_str_new[i] + ",";
            }
        }
    }

    if (!find_data.empty() && find_data.back() == ',')
    {
        find_data.pop_back();
    }

    return find_data;
}

/*
*功能: 将字符串类型的连续多组温度转化到 struct multiple_temp 中存储
*参数:
* base_str: 一个字符串类型的连续温度形如 1:23.1,2:63.5
* mul_tmp: 最终转化出来的多组温度数据
*返回值:
* -1 表示转化失败
*/
int str_to_multiple_temp(std::string base_str, struct multiple_temp* mul_tmp)
{
    int ret = 0;
    // 1. 先将逗号连续的温度打散
    std::vector<std::string> vct_str_new;
    int count_str_new =xbasic::split_string(base_str,",",&vct_str_new);
    memset(mul_tmp, 0, sizeof(struct multiple_temp));
    // 遍历每一组温度
    for(int i=0; i<count_str_new; i++)
    {
        std::vector<std::string> vct_value;
        int count_value =xbasic::split_string(vct_str_new[i],":",&vct_value);
        if (count_value != 2)
        {
            // xbasic::debug_output("str_to_multiple_temp() error: input string is error,please check!!!\n");
            LOG_MSG(ERR_LOG, "str_to_multiple_temp() error: input string is error,please check!!!");
            ret = -1;
            break;
        }
        else
        {
            // 每一组温度拆分成功
            mul_tmp->arry[mul_tmp->sz].m_id = std::stoi(vct_value[0]);
            mul_tmp->arry[mul_tmp->sz].m_temp_value = std::stof(vct_value[1]);
            LOG_MSG(MSG_LOG, "str_to_multiple_temp() mul_tmp->arry[%d].m_id=%d, mul_tmp->arry[%d].m_temp_value=%f", mul_tmp->sz, mul_tmp->arry[mul_tmp->sz].m_id, mul_tmp->sz, mul_tmp->arry[mul_tmp->sz].m_temp_value);
            ++(mul_tmp->sz);
        }
    }
    return ret;
}

/*
*功能: 将字符串类型的连续多组电压转化到 struct multiple_voltage 中存储
*参数:
* base_str: 一个字符串类型的连续电压形如 1:23.1,2:63.5
* mul_vol: 最终转化出来的多组电压数据
*返回值:
* -1 表示转化失败 0 表示转化成功
*/
int str_to_multiple_volatage(std::string base_str, struct multiple_voltage* mul_vol)
{
    int ret = 0;
    // 1. 先将多连续的电压打散
    std::vector<std::string> vct_str_new;
    int count_str_new =xbasic::split_string(base_str,",",&vct_str_new);
    memset(mul_vol, 0, sizeof(struct multiple_voltage));
    // 遍历每一组电压
    for(int i=0; i<count_str_new; i++)
    {
        std::vector<std::string> vct_value;
        int count_value =xbasic::split_string(vct_str_new[i],":",&vct_value);
        if (count_value != 2) {
            // xbasic::debug_output("str_to_multiple_volatage() error: input string is error,please check!!!\n");
            LOG_MSG(ERR_LOG, "str_to_multiple_volatage() error: input string is error,please check!!!");
            ret = -1;
            break;
        }
        else
        {
            // 每一组电压拆分成功
            mul_vol->arry[mul_vol->sz].m_id = std::stoi(vct_value[0]);
            mul_vol->arry[mul_vol->sz].m_voltage_value = std::stof(vct_value[1]);
            ++(mul_vol->sz);
        }
    }
    return ret;
}

/**
*功能: 将 9528 的原始二进制信息转化成锁定状态信息给用户返回
*参数: base_str: 从设备管理服务端获取到的原始二进制时钟状态信息
*        status: 返回给调用者的时钟锁定状态信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int parse_ad9528_lckdata(const std::string& base_value, struct clock_status *status)
{
    int ret = 0;
    // xbasic::debug_output("Enter into parse_ad9528_lckdata()\n");
    LOG_MSG(MSG_LOG, "Enter into parse_ad9528_lckdata()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<ad9528_lock> vct_9528;
    int r = xconvert::parse_ad9528_lckdata(base_value, vct_9528);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        // xbasic::debug_output("Exited parse_ad9528_lckdata() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited parse_ad9528_lckdata() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_9528 中了
        status->sz = vct_9528.size();
        if(status->sz > MAX_9528_CHIP_SIZE)
        {
            // xbasic::debug_output("parse_ad9528_lckdata() warning: space is not enough!\n");
            LOG_MSG(WRN_LOG, "parse_ad9528_lckdata() warning: space is not enough!");
        }

        int end = (status->sz < MAX_9528_CHIP_SIZE ? status->sz : MAX_9528_CHIP_SIZE);
        for(int i = 0; i < end; ++i)
        {
            status->arry[i] = vct_9528[i].m_lockstatus;
        }
    }
    // xbasic::debug_output("Exited parse_ad9528_lckdata() ret=%d\n.", ret);
    LOG_MSG(MSG_LOG, "Exited parse_ad9528_lckdata() ret=%d", ret);
    return ret;
}

/**
*功能: 将 9545 的原始二进制信息转化成锁定状态信息给用户返回
*参数: base_str: 从设备管理服务端获取到的原始二进制时钟状态信息
*        status: 返回给调用者的时钟锁定状态信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int parse_ad9545_lckdata(const std::string& base_value, struct clock_status *status)
{
    int ret = 0;
    // xbasic::debug_output("Enter into parse_ad9545_lckdata()\n");
    LOG_MSG(MSG_LOG, "Enter into parse_ad9545_lckdata()");
    // 1. 先对原始的二进制数据进行解析
    ad9545_lock ad9545_lck;
    int r = xconvert::parse_ad9545_lckdata(base_value, ad9545_lck);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        // xbasic::debug_output("Exited parse_ad9545_lckdata() error: value parse failed!\n");
        LOG_MSG(ERR_LOG, "Exited parse_ad9545_lckdata() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_9528 中了
        status->sz = 1;
        status->arry[0] = ad9545_lck.m_lockstatus;
    }
    // xbasic::debug_output("Exited parse_ad9545_lckdata() ret=%d\n.", ret);
    LOG_MSG(MSG_LOG, "Exited parse_ad9545_lckdata() ret=%d", ret);
    return ret;
}

/**
*功能: 将 pwm 的原始二进制信息转化成 struct pwm_val 信息给用户返回
*参数: base_value: 从设备管理服务端获取到的原始二进制 pwm 信息
*        p_val: 返回给调用者的时钟锁定状态信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int parse_pwm_frequency(const std::string& base_value, struct pwm_val *p_val)
{
    int ret = 0;
    LOG_MSG(MSG_LOG, "Enter into parse_pwm_frequency()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<pwm_check> vct_pwm;
    int r = xconvert::parse_pwm_check(base_value, vct_pwm);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        LOG_MSG(ERR_LOG, "Exited parse_pwm_frequency() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_pwm 中了
        p_val->sz = vct_pwm.size();
        if(p_val->sz > MAX_PWM_CHIP_SIZE)
        {
            LOG_MSG(WRN_LOG, "parse_pwm_frequency() warning: space is not enough!");
        }

        int end = (p_val->sz < MAX_PWM_CHIP_SIZE ? p_val->sz : MAX_PWM_CHIP_SIZE);
        for(int i = 0; i < end; ++i)
        {
            p_val->arry[i].m_id = vct_pwm[i].m_channel_id;
            p_val->arry[i].m_frequency = vct_pwm[i].m_pwm_value;
        }
    }
    LOG_MSG(MSG_LOG, "Exited parse_pwm_frequency() ret=%d", ret);
    return ret;
}

/*
*功能: 将 serdes 的原始二进制信息转化成 struct serdes_val 信息给用户返回
*参数: base_value: 从设备管理服务端获取到的原始 serdes 信息
*        s_val: 返回给调用者的 serdes 状态信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int parse_serdes_status(const std::string& base_value, struct serdes_val *s_val)
{
    int ret = 0;
    LOG_MSG(MSG_LOG, "Enter into parse_serdes_status()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<serdes_state> vct_serdes;
    int r = xconvert::parse_serdes_state(base_value, vct_serdes);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        LOG_MSG(ERR_LOG, "Exited parse_serdes_status() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_serdes 中了
        int sz = vct_serdes.size();
        if(sz > MAX_SERDES_SIZE)
        {
            LOG_MSG(WRN_LOG, "parse_serdes_status() warning: space is not enough!");
        }
        s_val->sz = sz > MAX_SERDES_SIZE ? MAX_SERDES_SIZE : sz;
        int end = s_val->sz;
        LOG_MSG(MSG_LOG, "parse_serdes_status() vct_serdes.size:%d s_val->sz:%d end:%d", sz, s_val->sz, end);
        for(int i = 0; i < s_val->sz; ++i)
        {
            s_val->arry[i].m_id = vct_serdes[i].m_id;
            s_val->arry[i].m_state = vct_serdes[i].m_serdes_value;
            LOG_MSG(MSG_LOG, "parse_serdes_status() s_val->arry[%d].m_id=%d, s_val->arry[%d].m_state=%hu", i, s_val->arry[i].m_id, i, s_val->arry[i].m_state);
        }
    }
    LOG_MSG(MSG_LOG, "Exited parse_serdes_status() ret=%d", ret);
    return ret;
}

int find_serdes_from_start_to_end(const std::string& base_value, int begin_num, int end_num, struct serdes_val *s_val)
{
    LOG_MSG(MSG_LOG, "Enter into find_serdes_from_start_to_end()");
    // 1. 先对原始的二进制数据进行解析
    std::vector<serdes_state> vct_serdes;
    int r = xconvert::parse_serdes_state(base_value, vct_serdes);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        LOG_MSG(ERR_LOG, "Exited find_serdes_from_start_to_end() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_serdes 中了
        // 2. 遍历数组查找对应的温度并返回
        for(auto &it : vct_serdes)
        {
            if(it.m_id >= begin_num && it.m_id <= end_num)
            {
                s_val->arry[s_val->sz].m_id = it.m_id;
                s_val->arry[s_val->sz].m_state = it.m_serdes_value;
                ++(s_val->sz);
            }
        }
    }
    // xbasic::debug_output("Exited find_serdes_from_start_to_end() find_size: %d, ret: %d\n", mul_tmp->sz, ret);
    LOG_MSG(MSG_LOG, "Exited find_serdes_from_start_to_end() find_size: %d, ret: %d", s_val->sz, ret);
    return ret;
}

/**
*功能: 解析原始的 UTP 芯片寄存器信息
*参数:
* base_value: 原始的二进制信息, 包含一个 UTP 的所有寄存器信息
* val: 输出参数, 返回给用户
*
*/
int parse_utp_register_info(const std::string& base_value, struct utp_register_val* val)
{
    LOG_MSG(MSG_LOG, "Enter into parse_utp_register_info() base_value size:%d", base_value.size());
    int ret = 0;

    const char* value_ptr   = base_value.data();
    size_t length           = base_value.length();
    // 其中 2 是 chip id 的两字节, 6 是一个寄存器信息的六个字节
    if(length != (2+6*MAX_UTP_REGISTER_NUM))
    {
        ret = -1;
        LOG_MSG(ERR_LOG,"parse_utp_register_info() value is invaild,length=%d",length);
    }
    else
    {
        int offset  = 0;
        char* pay_data = (char*)&value_ptr[0];
        while(offset < length)
        {
            unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
            val->chip_id = id;
            offset = offset + 2;
            for(int i = 0; i < MAX_UTP_REGISTER_NUM; i++)
            {
                // 读该芯片寄存器的信息
                val->register_val[i].valid = xbasic::read_bigendian(&pay_data[offset], 1);
                offset = offset + 1;
                val->register_val[i].reg_ch = xbasic::read_bigendian(&pay_data[offset], 1);
                offset = offset + 1;
                val->register_val[i].reg_addr = xbasic::read_bigendian(&pay_data[offset], 2);
                offset = offset + 2;
                val->register_val[i].reg_data = xbasic::read_bigendian(&pay_data[offset], 2);
                offset = offset + 2;
            }
        }
    }

    LOG_MSG(MSG_LOG, "Exited parse_utp_register_info()");
}

/**
*功能: 获取指定区间 utp40 芯片的寄存器值
*参数:
* base_value: 从设备管理服务端获取到的原始二进制 utp register 信息
* begin_id: 起始芯片编号
* end_id: 结束芯片编号
* val: 返回给调用者的 utp register 信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int find_utp_register_in_interval(const std::string& base_value, int begin_id, int end_id, struct utp_register_val* val, int len)
{
    int ret = 0;
    LOG_MSG(MSG_LOG, "Enter into find_utp_register_in_interval() begin_id:%d, end_id:%d, len:%d", begin_id, end_id, len);
    // 1. 先对原始的二进制数据进行解析
    std::vector<utp40_register> vct_register;
    int r = xconvert::parse_utp40_register(base_value, vct_register);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        LOG_MSG(ERR_LOG, "Exited find_utp_register_in_interval() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_register 中了
        for(const auto& it : vct_register)
        {
            if(it.m_utp40_chip_id >= begin_id && it.m_utp40_chip_id <= end_id)
            {
                // 是区间范围内的数据
                LOG_MSG(MSG_LOG, "find_utp_register_in_interval() utp chip_id:%d register num:%d", it.m_utp40_chip_id, it.m_register.size());
                // 提取芯片ID
                val[ret].chip_id = it.m_utp40_chip_id;
                // 提取该芯片寄存器信息
                for(int i = 0; i < MAX_UTP_REGISTER_NUM && i < it.m_register.size(); ++i)
                {
                    val[ret].register_val[i].valid = it.m_register[i].m_valid;
                    val[ret].register_val[i].reg_ch = it.m_register[i].m_reg_ch;
                    val[ret].register_val[i].reg_addr = it.m_register[i].m_reg_addr;
                    val[ret].register_val[i].reg_data = it.m_register[i].m_reg_data;
                }
                ++ret;
            }
        }
    }
    LOG_MSG(MSG_LOG, "Exited find_utp_register_in_interval() ret=%d", ret);
    return ret;
}

/**
*功能: 解析原始的 UTP102 芯片寄存器信息
*参数:
* base_value: 原始的二进制信息, 包含一个 UTP102 的所有寄存器信息
* val: 输出参数, 返回给用户
*
*/
int parse_utp102_register_info(const std::string& base_value, struct utp102_register_val* val)
{
    LOG_MSG(MSG_LOG, "Enter into parse_utp102_register_info() base_value size:%d", base_value.size());
    int ret = 0;

    const char* value_ptr   = base_value.data();
    size_t length           = base_value.length();
    // 其中 2 是 chip id 的两字节, 6 是一个寄存器信息的六个字节
    if(length != (2+6*MAX_UTP102_REGISTER_NUM))
    {
        ret = -1;
        LOG_MSG(ERR_LOG,"parse_utp102_register_info() value is invaild,length=%d",length);
    }
    else
    {
        int offset  = 0;
        char* pay_data = (char*)&value_ptr[0];
        while(offset < length)
        {
            unsigned short id = xbasic::read_bigendian(&pay_data[offset], 2);
            val->chip_id = id;
            offset = offset + 2;
            for(int i = 0; i < MAX_UTP102_REGISTER_NUM; i++)
            {
                // 读该芯片寄存器的信息
                val->register_val[i].valid = xbasic::read_bigendian(&pay_data[offset], 1);
                offset = offset + 1;
                val->register_val[i].reg_ch = xbasic::read_bigendian(&pay_data[offset], 1);
                offset = offset + 1;
                val->register_val[i].reg_addr = xbasic::read_bigendian(&pay_data[offset], 2);
                offset = offset + 2;
                val->register_val[i].reg_data = xbasic::read_bigendian(&pay_data[offset], 2);
                offset = offset + 2;
            }
        }
    }

    LOG_MSG(MSG_LOG, "Exited parse_utp102_register_info()");
}

/**
*功能: 获取指定区间 utp102 芯片的寄存器值
*参数:
* base_value: 从设备管理服务端获取到的原始二进制 utp register 信息
* begin_id: 起始芯片编号
* end_id: 结束芯片编号
* val: 返回给调用者的 utp register 信息
*返回值:
*        -1: 表示转化失败
*        0: 表示转化成功
*/
int find_utp102_register_in_interval(const std::string& base_value, int begin_id, int end_id, struct utp102_register_val* val, int len)
{
    int ret = 0;
    LOG_MSG(MSG_LOG, "Enter into find_utp102_register_in_interval() begin_id:%d, end_id:%d, len:%d", begin_id, end_id, len);
    // 1. 先对原始的二进制数据进行解析
    std::vector<utp40_register> vct_register;
    int r = xconvert::parse_utp102_register(base_value, vct_register);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        LOG_MSG(ERR_LOG, "Exited find_utp102_register_in_interval() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功, 此时数据已经被存放到 vct_register 中了
        for(const auto& it : vct_register)
        {
            if(it.m_utp40_chip_id >= begin_id && it.m_utp40_chip_id <= end_id)
            {
                // 是区间范围内的数据
                LOG_MSG(MSG_LOG, "find_utp102_register_in_interval() utp chip_id:%d register num:%d", it.m_utp40_chip_id, it.m_register.size());
                // 提取芯片ID
                val[ret].chip_id = it.m_utp40_chip_id;
                // 提取该芯片寄存器信息
                for(int i = 0; i < MAX_UTP102_REGISTER_NUM && i < it.m_register.size(); ++i)
                {
                    val[ret].register_val[i].valid = it.m_register[i].m_valid;
                    val[ret].register_val[i].reg_ch = it.m_register[i].m_reg_ch;
                    val[ret].register_val[i].reg_addr = it.m_register[i].m_reg_addr;
                    val[ret].register_val[i].reg_data = it.m_register[i].m_reg_data;
                }
                ++ret;
            }
        }
    }
    LOG_MSG(MSG_LOG, "Exited find_utp102_register_in_interval() ret=%d", ret);
    return ret;
}

std::map<int, std::string> get_all_data_from_result(std::string value)
{
    std::map<int, std::string> result;
    std::vector<std::string> vct_str_new;
    int count_str_new =xbasic::split_string(value,",",&vct_str_new);
    for(int i=0; i<count_str_new; i++)
    {
        std::vector<std::string> vct_value;
        int count_value =xbasic::split_string(vct_str_new[i],":",&vct_value);
        if (count_value != 2)
        {
            // xbasic::debug_output("get_all_board_from_result() input string is error,please check!!!\n");
            LOG_MSG(ERR_LOG, "get_all_board_from_result() input string is error,please check!!!");
            return result;
        }
        else
        {
            int boardid = std::stoi(vct_value[0]);
            std::string new_value = vct_value[1];
            result[boardid] = new_value;
        }
    }

    return result;
}

std::map<int, std::string> get_all_board()
{
    std::map<int, std::string> returnedMap;
    int errcode = 0;
    std::string value;
    if (mgr_session::get_instance()->call_get_value_from_dev(0xff, VALUE_BOARD_ID, errcode, value) < 0)
    {
        return returnedMap;
    }
    // xbasic::debug_output("get_all_board() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "get_all_board() value=%s",value.c_str());
    if (errcode < 0)
    {
        return returnedMap;
    }
    else
    {
        if (!value.empty())
        {
            returnedMap = get_all_data_from_result(value);
        }
    }

    // for debug
    for (const auto& pair : returnedMap)
    {
        // xbasic::debug_output("get_all_board() key=%d value=%s\n", pair.first, pair.second.c_str());
        LOG_MSG(MSG_LOG, "get_all_board() key=%d value=%s", pair.first, pair.second.c_str());
    }

    return returnedMap;
}

int cal_file_start_save(int cal_type, board_type type, int id, const char *file_name)
{
    int board_id = ((id & 0xFF) << 8) | type;
    LOG_MSG(MSG_LOG, "Enter into cal_file_start_save(cal_type:%d, type:0x%x, id:0x%x, file_name:%s) board_id:0x%x",cal_type,type,id,file_name,board_id);
    int errcode = 0;
    std::string str_return;
    //判断校准文件的类型与对应串口的id是否一致，
    //防止升级文件到错误的单片机串口上
    if(judge_type_serialid(cal_type,board_id) == false)
    {
        return USB_ERR_PARAM;
    }

    // 校准文件没有版本号的概念, 所以这里传一个固定的版本号 0.0.0.1
    if (mgr_session::get_instance()->call_ota_start_upgrade(cal_type,"0.0.0.1",board_id,errcode,str_return,file_name) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    LOG_MSG(MSG_LOG, "Exited cal_file_start_save() errcode:%d", errcode);
    return errcode;
}

int cal_file_query_progress(int cal_type, board_type type, int id, float *progress)
{
    int board_id = ((id & 0xFF) << 8) | type;
    LOG_MSG(MSG_LOG, "Enter into cal_file_query_progress(cal_type:%d, type:0x%x, id:0x%x) board_id:0x%x",cal_type,type,id,board_id);
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(cal_type,board_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_query_upgrade(cal_type, board_id,errcode, str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    *progress = std::stof(str_return);
    // xbasic::debug_output("usb_ota_query_progress()  ota_type=%d usb_device_addr_id=%d progress=%0.2f.\n",ota_type,usb_device_addr_id,*progress);
    LOG_MSG(MSG_LOG, "usb_ota_query_progress() cal_type=%d board_id=%d progress=%f.",cal_type,board_id,*progress);

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    LOG_MSG(MSG_LOG, "Exited cal_file_query_progress() errcode:%d", errcode);
    return errcode;
}

int cal_file_complete_save(int cal_type, board_type type, int id)
{
    int board_id = ((id & 0xFF) << 8) | type;
    LOG_MSG(MSG_LOG, "Enter into cal_file_query_progress(cal_type:%d, type:0x%x, id:0x%x) board_id:0x%x",cal_type,type,id,board_id);
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(cal_type,board_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_ota_complete_upgrade(cal_type,board_id,errcode,str_return) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }
}

int cal_file_start_read(int cal_type, board_type type, int id, const char *file_name)
{
    int board_id = ((id & 0xFF) << 8) | type;
    LOG_MSG(MSG_LOG, "Enter into cal_file_start_read(cal_type:%d, type:0x%x, id:0x%x, file_name:%s) board_id:0x%x",cal_type,type,id,file_name,board_id);
    int errcode = 0;
    std::string str_return;

    if(judge_type_serialid(cal_type,board_id) == false)
    {
        return USB_ERR_PARAM;
    }

    if (mgr_session::get_instance()->call_cal_read_file(cal_type,"0.0.0.1",board_id,errcode,str_return,file_name) < 0)
    {
        return USB_ERR_NETWORK;
    }

    if (errcode < 0)
    {
        return (errcode == -1 ? USB_ERR_RETURN : errcode);
    }

    LOG_MSG(MSG_LOG, "Exited cal_file_start_read() errcode:%d", errcode);
    return errcode;
}

/*
*功能: 获取PGB板槽位号
*参数: *val: 输出slot值
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbSlot(int *val) //for pgb use
{
    std::map<int, std::string> i_board_map;
    i_board_map = get_all_board();
    std::string local_ip = xbasic::getFirstNonLoopbackIPv4();
    for (auto& i_board : i_board_map)
    {
        // xbasic::debug_output("DIG_GetPgbSlot() local_ip=%s i_board.ip=%s\n", local_ip.c_str(), i_board.second.c_str());
        LOG_MSG(MSG_LOG, "DIG_GetPgbSlot() local_ip=%s i_board.ip=%s", local_ip.c_str(), i_board.second.c_str());
        if ((i_board.first & 0xFF) == BOARDTYPE_FT_PGB && (i_board.second == local_ip))
        {
            *val = (i_board.first & 0xFF00) >> 8 ;
            return 0;
        }
    }

    return -1;
}

/*
*功能: 获取SYNC板槽位号
*参数: *val: 输出slot值
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncSlot(int *val) //for pgb use
{
    std::map<int, std::string> i_board_map;
    i_board_map = get_all_board();
    std::string local_ip = xbasic::getFirstNonLoopbackIPv4(); //debug
    for (auto& i_board : i_board_map)
    {
        // xbasic::debug_output("DIG_GetSyncSlot() local_ip=%s i_board.ip=%s\n", local_ip.c_str(), i_board.second.c_str()); //debug
        LOG_MSG(MSG_LOG, "DIG_GetSyncSlot() local_ip=%s i_board.ip=%s", local_ip.c_str(), i_board.second.c_str());
        if ((i_board.first & 0xFF) == BOARDTYPE_FT_SYNC || (i_board.first & 0xFF) == BOARDTYPE_CP_SYNC)
        {
            *val = (i_board.first & 0xFF00) >> 8 ;
            return 0;
        }
    }

    return -1;
}

/*
*功能: 获取ASIC板槽位号
*参数: *val: 输出slot值
*        slot_nums:  要获取的单板数量, 必须大于实际存在的单板
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetAsicSlot(int *val, int slot_nums) //for pgb use
{
    std::map<int, std::string> i_board_map;
    int slot_num = 0;
    i_board_map = get_all_board();
    std::string local_ip = xbasic::getFirstNonLoopbackIPv4(); //debug
    std::vector<int> asic_slots;
    for (auto& i_board : i_board_map)
    {
        // xbasic::debug_output("DIG_GetAsicSlot() local_ip=%s i_board.ip=%s\n", local_ip.c_str(), i_board.second.c_str()); //debug
        LOG_MSG(MSG_LOG, "DIG_GetAsicSlot() local_ip=%s i_board.ip=%s", local_ip.c_str(), i_board.second.c_str());
        if (((i_board.first & 0xFF) == BOARDTYPE_ASIC) && (i_board.second == local_ip))
        {
            int asic_slot = (i_board.first & 0xFF00) >> 8;
            slot_num = slot_num + 1;
            asic_slots.push_back(asic_slot);
        }
    }

    if (slot_num > slot_nums)
    {
        // xbasic::debug_output("DIG_GetAsicSlot() input slots(%d) is not enough(%d) \n", slot_nums, slot_num);
        LOG_MSG(ERR_LOG, "DIG_GetAsicSlot() input slots(%d) is not enough(%d)", slot_nums, slot_num);
        return -1;
    }
    else
    {
        for (int i = 0; i < slot_num; i++)
        {
            val[i] = asic_slots[i];
        }

        return 0;
    }

    return -1;
}

/*
*功能: 获取PPS板槽位号
*参数: *val: 输出slot值
*        slot_nums:  要获取的单板数量, 必须大于实际存在的单板
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPpsSlot(int *val, int slot_nums) //for pgb use
{
    std::map<int, std::string> i_board_map;
    int slot_num = 0;
    i_board_map = get_all_board();
    //std::string local_ip = xbasic::getFirstNonLoopbackIPv4(); //debug
    std::vector<int> pps_slots;
    for (auto& i_board : i_board_map)
    {
        //xbasic::debug_output("DIG_GetPpsSlot() local_ip=%s i_board.ip=%s\n", local_ip.c_str(), i_board.second.c_str()); //debug
        if ((i_board.first & 0xFF) == BOARDTYPE_FT_PPS)
        {
            int pps_slot = (i_board.first & 0xFF00) >> 8;
            slot_num = slot_num + 1;
            pps_slots.push_back(pps_slot);
        }
    }

    if (slot_num > slot_nums)
    {
        // xbasic::debug_output("DIG_GetPpsSlot() input slots(%d) is not enough(%d) \n", slot_nums, slot_num);
        LOG_MSG(ERR_LOG, "DIG_GetPpsSlot() input slots(%d) is not enough(%d)", slot_nums, slot_num);
        return -1;
    }
    else
    {
        for (int i = 0; i < slot_num; i++)
        {
            val[i] = pps_slots[i];
        }

        return 0;
    }

    return -1;
}

/*
*功能: 获取dig板在位信号
*参数: dig_index: 根据硬件规格是 1~16
*        val: 在位值 1在位, 0个在位
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetDigExist(int dig_index, int *val)
{
    int errcode = 0;
    std::string value;
    int dstid = dig_index;
    // xbasic::debug_output("DIG_GetDigExist() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetDigExist() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_DIG_EXIST, errcode, value) < 0)
    {
        return -1;
    }

    // xbasic::debug_output("DIG_GetDigExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetDigExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }

    // xbasic::debug_output("DIG_GetDigExist() value=%d\n",*val);
    LOG_MSG(MSG_LOG, "DIG_GetDigExist() value=%d",*val);
    return 0;
}

/*
*功能: 获取pps板在位信号
*参数: pps_index: 根据硬件规格是 1~32
*        val: 在位值 1在位, 0不在位
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPpsExist(int pps_index, int *val)
{
    int errcode = 0;
    std::string value;
    if (0 > pps_index)
    {
        // xbasic::debug_output("DIG_GetPpsExist() pps_index=0x%x\n",pps_index);
        LOG_MSG(ERR_LOG, "DIG_GetPpsExist() pps_index=0x%x",pps_index);
        return -1;
    }

    int dstid = ((pps_index & 0xFF) << 8) | BOARDTYPE_FT_PPS;
    // xbasic::debug_output("DIG_GetPpsExist() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPpsExist() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PPS_EXIST, errcode, value) < 0)
    {
        return -1;
    }

    // xbasic::debug_output("DIG_GetPpsExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPpsExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }

    // xbasic::debug_output("DIG_GetPpsExist() value=%d\n",*val);
    LOG_MSG(MSG_LOG, "DIG_GetPpsExist() value=%d",*val);
    return 0;
}

/*
*功能: 获取asic板在位信号
*参数: asic_index: 根据硬件规格是 1~64
*        val: 在位值 1在位, 0不在位
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetAsicExist(int asic_index, int *val)
{
    int errcode = 0;
    std::string value;
    if (0 > asic_index)
    {
        // xbasic::debug_output("DIG_GetAsicExist() asic_index=0x%x\n",asic_index);
        LOG_MSG(ERR_LOG, "DIG_GetAsicExist() asic_index=0x%x",asic_index);
        return -1;
    }

    int dstid = ((asic_index & 0xFF) << 8) | BOARDTYPE_ASIC;
    // xbasic::debug_output("DIG_GetAsicExist() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetAsicExist() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_EXIST, errcode, value) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetAsicExist() call_get_value_from_dev() failed dstid:0x%x errcode:%d",dstid,errcode);
        return -1;
    }

    // xbasic::debug_output("DIG_GetAsicExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetAsicExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }

    // xbasic::debug_output("DIG_GetAsicExist() value=%d\n",*val);
    LOG_MSG(MSG_LOG, "DIG_GetAsicExist() value=%d",*val);
    return 0;
}

/*
*功能: 获取pgb板在位信号
*参数: pgb_index: 根据硬件规格是 1~4
*        val: 在位值 1在位, 0不在位
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbExist(int pgb_index, int *val)
{
    int errcode = 0;
    std::string value;
    if (0 > pgb_index)
    {
        // xbasic::debug_output("DIG_GetPgbExist() pgb_index=0x%x\n",pgb_index);
        LOG_MSG(ERR_LOG, "DIG_GetPgbExist() pgb_index=0x%x",pgb_index);
        return -1;
    }
    //pgb_index = 0xFF; //无效值, 直接获取运行此so单板的ID

    int dstid = ((pgb_index & 0xFF) << 8) | BOARDTYPE_FT_PGB;
    // xbasic::debug_output("DIG_GetPgbExist() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPgbExist() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PGB_EXIST, errcode, value) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetPgbExist() call_get_value_from_dev() failed dstid:0x%x errcode:%d",dstid,errcode);
        return -1;
    }

    // xbasic::debug_output("DIG_GetPgbExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPgbExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetPgbExist() call_get_value_from_dev() failed dstid:0x%x the errcode:%d",dstid,errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }
}

/*
*功能: 获取sync板在位信号
*参数:
*        dev: 设备类型 0: CP 1: FT
*        index: sync板slot号
*        val: 在位返回对应slot 不在位返回失败
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncExist(int dev,int sync_index, int *val)
{
    int errcode = 0;
    std::string value;
    if (0 > sync_index)
    {
        // xbasic::debug_output("DIG_GetSyncExist() sync_index=0x%x\n",sync_index);
        LOG_MSG(ERR_LOG, "DIG_GetSyncExist() sync_index=0x%x",sync_index);
        return -1;
    }
    //sync_index = 0xFF; //无效值, 直接获取运行此so单板的ID

    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_CP_SYNC;
    }
    else if(dev == 1)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_FT_SYNC;
    }
    else
    {
        // xbasic::debug_output("DIG_GetSyncExist() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetSyncExist() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncExist() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSyncExist() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SYNC_EXIST, errcode, value) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetSyncExist() call_get_value_from_dev() failed dstid:0x%x errcode:%d",dstid,errcode);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSyncExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetSyncExist() call_get_value_from_dev() failed dstid:0x%x the errcode:%d",dstid,errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }

    // xbasic::debug_output("DIG_GetSyncExist() value=%d\n",*val);
    LOG_MSG(MSG_LOG, "DIG_GetSyncExist() value=%d",*val);
    return 0;
}

/*
*功能: 获取sync板ID
*参数:
*        dev: 设备类型 0: CP 1: FT
*        sync_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncID(int dev, int sync_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;
    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_CP_SYNC;
    }
    else if(dev == 1)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_FT_SYNC;
    }
    else
    {
        // xbasic::debug_output("DIG_GetSyncID() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetSyncID() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncID() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSyncID() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SYNC_ID, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetSyncID() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetSyncID() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSyncID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetSyncID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetSyncID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetSyncID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetSyncID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取sync板SN
*参数:
*        dev: 设备类型 0: CP 1: FT
*        sync_index: 全局索引号, 与硬件slot对应
*        id: 板SN
*        buf_len: 输入的缓冲区大小
*        sn_len: 输出的sn大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncSN(int dev,int sync_index, char* sn, size_t buf_len, size_t* sn_len)
{
    int errcode = 0;
    std::string value;
    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_CP_SYNC;
    }
    else if(dev == 1)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_FT_SYNC;
    }
    else
    {
        // xbasic::debug_output("DIG_GetSyncSN() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetSyncSN() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncSN() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSyncSN() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SYNC_SN, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetSyncSN() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetSyncSN() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncSN() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSyncSN() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetSyncSN() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetSyncSN() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetSyncSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetSyncSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(sn, value.c_str());
            *sn_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PGB板ID
*参数:
*        pgb_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbID(int pgb_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pgb_index & 0xFF) << 8) | BOARDTYPE_FT_PGB;
    // xbasic::debug_output("DIG_GetPgbID() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPgbID() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PGB_ID, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPgbID() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPgbID() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPgbID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPgbID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPgbID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPgbID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PGB板SN
*参数:
*        pgb_index: 全局索引号, 与硬件slot对应
*        id: 板SN
*        buf_len: 输入的缓冲区大小
*        sn_len: 输出的sn大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbSN(int pgb_index, char* sn, size_t buf_len, size_t* sn_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pgb_index & 0xFF) << 8) | BOARDTYPE_FT_PGB;
    // xbasic::debug_output("DIG_GetPgbSN() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPgbSN() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PGB_SN, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPgbSN() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPgbSN() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPgbSN() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPgbSN() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPgbSN() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPgbSN() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(sn, value.c_str());
            *sn_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PPS板ID
*参数:
*        pps_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPpsID(int pps_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pps_index & 0xFF) << 8) | BOARDTYPE_FT_PPS;
    // xbasic::debug_output("DIG_GetPpsID() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPpsID() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PPS_ID, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPpsID() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPpsID() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPpsID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPpsID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPpsID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPpsID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PPS板SN
*参数:
*        pps_index: 全局索引号, 与硬件slot对应
*        id: 板SN
*        buf_len: 输入的缓冲区大小
*        sn_len: 输出的sn大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPpsSN(int pps_index, char* sn, size_t buf_len, size_t* sn_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pps_index & 0xFF) << 8) | BOARDTYPE_FT_PPS;
    // xbasic::debug_output("DIG_GetPpsSN() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPpsSN() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PPS_SN, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPpsSN() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPpsSN() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPpsSN() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPpsSN() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPpsSN() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPpsSN() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(sn, value.c_str());
            *sn_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取ASIC板ID
*参数:
*        asic_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetAsicID(int asic_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((asic_index & 0xFF) << 8) | BOARDTYPE_ASIC;
    // xbasic::debug_output("DIG_GetAsicID() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetAsicID() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_ID, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetAsicID() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetAsicID() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetAsicID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetAsicID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetAsicID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetAsicID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取ASIC板SN
*参数:
*        asic_index: 全局索引号, 与硬件slot对应
*        id: 板SN
*        buf_len: 输入的缓冲区大小
*        sn_len: 输出的sn大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetAsicSN(int asic_index, char* sn, size_t buf_len, size_t* sn_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((asic_index & 0xFF) << 8) | BOARDTYPE_ASIC;
    // xbasic::debug_output("DIG_GetAsicSN() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetAsicSN() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_SN, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetAsicSN() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetAsicSN() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetAsicSN() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetAsicSN() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetAsicSN() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetAsicSN() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetAsicSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetAsicSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(sn, value.c_str());
            *sn_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取DIG板ID
*参数:
*        dig_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetDigID(int dig_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((dig_index & 0xFF) << 8) | BOARDTYPE_DIG;
    //xbasic::debug_output("DIG_GetDigID() dstid=0x%x\n",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_DIG_ID, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetDigID() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetDigID() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetDigID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetDigID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetDigID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetDigID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取DSA板ID
*参数:
*        asic_index: 全局索引号, 与硬件slot对应
*        id: 板ID
*        buf_len: 输入的缓冲区大小
*        id_len: 输出的id大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetDsaID(int asic_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    int dsa_id = 0;
    int asic_id1, asic_id2, asic_id3, asic_id4;
    int asic_slot_base = (asic_index - 1) / 3;
    int dsa_slot_index = (asic_index - 1) / 3;

    for(int i=0; i<4; i++)
    {
        std::string value;
        int dstid = ((asic_index + i & 0xFF) << 8) | BOARDTYPE_ASIC;
        if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_ID, errcode, value) < 0)
        {
            // xbasic::debug_output("DIG_GetDsaID() call_get_value_from_dev failed");
            LOG_MSG(ERR_LOG, "DIG_GetDsaID() call_get_value_from_dev failed");
            return -1;
        }

        if (errcode < 0)
        {
            // xbasic::debug_output("DIG_GetAsicID() errcode:%d", errcode);
            LOG_MSG(ERR_LOG, "DIG_GetAsicID() errcode:%d", errcode);
            return -1;
        }
    }
    // xbasic::debug_output("DIG_GetDsaID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetDsaID() value=%s",value.c_str());
    if (i == 0)
    {
        asic_id1 = std::stoi(value);
    }
    else if (i == 1)
    {
        asic_id2 = std::stoi(value);
    }
    else if (i == 2)
    {
        asic_id3 = std::stoi(value);
    }
    else if (i == 3)
    {
        asic_id4 = std::stoi(value);
    }

    if (dsa_slot_index == 0)
    {
        dsa_id = ((asic_id1 & 0xF) << 12) | ((asic_id2 & 0xF00) | ((asic_id3 & 0xF) << 4) | ((asic_id4 & 0xF00) >> 8));
    }
    else if (dsa_slot_index == 1)
    {
        dsa_id = ((asic_id1 & 0xF0) << 12) | ((asic_id2 & 0xF0) << 8) | (asic_id3 & 0xF0) | ((asic_id4 & 0xF0) >> 4);
    }
    else if (dsa_slot_index == 2)
    {
        dsa_id = ((asic_id1 & 0xF00) << 4) | ((asic_id2 & 0xF) << 8) | ((asic_id3 & 0xF00) >> 4) | (asic_id4 & 0xF);
    }

    //id = std::to_string(dsa_id);
    // xbasic::debug_output("DIG_GetDsaID() dsa_id=%d\n",dsa_id);
    LOG_MSG(MSG_LOG, "DIG_GetDsaID() dsa_id=%d",dsa_id);
    std::string id_value = std::to_string(dsa_id);
    strcpy(id, id_value.c_str());
    *id_len = id_value.length();

    return 0;
}

/*
*功能: 获取sync板硬件版本
*参数:
*        dev: 设备类型 0: CP 1: FT
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 硬件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncHwVersion(int dev, int sync_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_CP_SYNC;
    }
    else if(dev == 1)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_FT_SYNC;
    }
    else
    {
        // xbasic::debug_output("DIG_GetSyncHwVersion() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetSyncHwVersion() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncHwVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSyncHwVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SYNC_HW_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetSyncHwVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetSyncHwVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncHwVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSyncHwVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetSyncHwVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetSyncHwVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetSyncHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetSyncHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PGB板硬件版本
*参数:
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 硬件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbHwVersion(int pgb_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pgb_index & 0xFF) << 8) | BOARDTYPE_FT_PGB;
    // xbasic::debug_output("DIG_GetPgbHwVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPgbHwVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PGB_HW_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPgbHwVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPgbHwVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPgbHwVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPgbHwVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPgbHwVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPgbHwVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

int DIG_GetPpsHwVersion(int pps_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pps_index & 0xFF) << 8) | BOARDTYPE_FT_PPS;
    // xbasic::debug_output("DIG_GetPpsHwVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPpsHwVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PPS_HW_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPpsHwVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPpsHwVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPpsHwVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPpsHwVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPpsHwVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPpsHwVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPpsHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPpsHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

int DIG_GetAsicHwVersion(int asic_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((asic_index & 0xFF) << 8) | BOARDTYPE_ASIC;
    // xbasic::debug_output("DIG_GetAsicHwVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetAsicHwVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_HW_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetAsicHwVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetAsicHwVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetAsicHwVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetAsicHwVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetAsicHwVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetAsicHwVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetAsicHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetAsicHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取sync板软件版本
*参数:
*        dev: 设备类型 0: CP 1: FT
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 软件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetSyncSoftVersion(int dev, int sync_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_CP_SYNC;
    }
    else if(dev == 1)
    {
        dstid = ((sync_index & 0xFF) << 8) | BOARDTYPE_FT_SYNC;
    }
    else
    {
        // xbasic::debug_output("DIG_GetSyncSoftVersion() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetSyncSoftVersion() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncSoftVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSyncSoftVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SYNC_SOFT_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetSyncSoftVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetSyncSoftVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetSyncSoftVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSyncSoftVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetSyncSoftVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetSyncSoftVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetSyncSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetSyncSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PGB板软件版本
*参数:
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 软件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPgbSoftVersion(int pgb_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pgb_index & 0xFF) << 8) | BOARDTYPE_FT_PGB;
    // xbasic::debug_output("DIG_GetPgbSoftVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPgbSoftVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PGB_SOFT_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPgbSoftVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPgbSoftVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPgbSoftVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPgbSoftVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPgbSoftVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPgbSoftVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取PPS板软件版本
*参数:
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 软件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetPpsSoftVersion(int pps_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((pps_index & 0xFF) << 8) | BOARDTYPE_FT_PPS;
    // xbasic::debug_output("DIG_GetPpsSoftVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPpsSoftVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PPS_SOFT_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetPpsSoftVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetPpsSoftVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetPpsSoftVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPpsSoftVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPpsSoftVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPpsSoftVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPgbSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPgbSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取ASIC板软件版本
*参数:
*        sync_index: 全局索引号, 与硬件slot对应
*        version: 软件版本号
*        buf_len: 输入的缓冲区大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetAsicSoftVersion(int asic_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((asic_index & 0xFF) << 8) | BOARDTYPE_ASIC;
    // xbasic::debug_output("DIG_GetAsicSoftVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetAsicSoftVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_ASIC_SOFT_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetAsicSoftVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetAsicSoftVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetAsicSoftVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetAsicSoftVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetAsicSoftVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetAsicSoftVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetAsicSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetAsicSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取DIG的版本号
*参数:
*        dev: 设备类型 0: CP 1: FT
*        dig_index: 根据硬件规格是 1-16
*        version: DIG的版本号
*        buf_len: version buffer空间大小
*        version_len: version的实际长度
*返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
*/
int DIG_GetDigVersion(int dev, int dig_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = 0;
    if(dev == 0)
    {
        dstid = ((dig_index & 0xFF) << 8) | BOARDTYPE_CP_DIG;
    }
    else if(dev == 1)
    {
        dstid = ((dig_index & 0xFF) << 8) | BOARDTYPE_FT_DIG;
    }
    else
    {
        // xbasic::debug_output("DIG_GetDigVersion() dev is wrong dev=%d",dev);
        LOG_MSG(ERR_LOG, "DIG_GetDigVersion() dev is wrong dev=%d",dev);
        return -1;
    }

    // xbasic::debug_output("DIG_GetDigVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetDigVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_DIG_LOGIC_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetDigVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetDigVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetDigVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetDigVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetDigVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetDigVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetDigVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetDigVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取板是否在位
*参数: type: 板类型
*        board_index: 全局索引号, 与硬件slot对应
*        val: 0: 不在位, 1: 在位
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetBoardExist(board_type type, int board_index, int *val)
{
    int errcode = 0;
    std::string value;
    // xbasic::debug_output("DIG_GetBoardExist() type=0x%x, board_index=0x%x\n", type, board_index);
    LOG_MSG(MSG_LOG, "DIG_GetBoardExist() type=0x%x, board_index=0x%x", type, board_index);
    if ((type == BOARDTYPE_FT_DIG) || (type == BOARDTYPE_CP_DIG))
    {
        if (mgr_session::get_instance()->call_get_value_from_dev(board_index, VALUE_DIG_EXIST, errcode, value) < 0)
        {
            LOG_MSG(ERR_LOG, "DIG_GetBoardExist() call_get_value_from_dev failed(DIG)");
            return -1;
        }
    }
    else
    {
        int dstid = ((board_index & 0xFF) << 8) | type;
        // xbasic::debug_output("DIG_GetBoardExist() dstid=0x%x\n",dstid);
        LOG_MSG(MSG_LOG, "DIG_GetBoardExist() dstid=0x%x",dstid);
        if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_BOARD_EXIST, errcode, value) < 0)
        {
            LOG_MSG(ERR_LOG, "DIG_GetBoardExist() call_get_value_from_dev failed");
            return -1;
        }
    }

    // xbasic::debug_output("DIG_GetBoardExist() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetBoardExist() value=%s",value.c_str());

    if (errcode < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetBoardExist() errcode=%d",errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            *val = std::stoi(value);
        }
    }
    // xbasic::debug_output("DIG_GetBoardExist() val=%d\n",*val);
    LOG_MSG(MSG_LOG, "DIG_GetBoardExist() val=%d",*val);
    return 0;
}

/*
*功能: 获取板BoardID
*参数: type: 板类型
*        board_index: 全局索引号, 与硬件slot对应
*        id: 板id
*        buf_len: id buffer空间大小
*        id_len: id的实际长度
*返回值: 0: 执行成功    -1: 执行失败  -2: id空间大小不足
*/
int DIG_GetBoardID(board_type type, int board_index, char* id, size_t buf_len, size_t* id_len)
{
    int errcode = 0;
    std::string value;

    if ((type == BOARDTYPE_FT_DIG) || (type == BOARDTYPE_CP_DIG))
    {
        if (mgr_session::get_instance()->call_get_value_from_dev(board_index, VALUE_DIG_ID, errcode, value) < 0)
        {
            // xbasic::debug_output("DIG_GetID() call_get_value_from_dev failed");
            LOG_MSG(ERR_LOG, "DIG_GetID() call_get_value_from_dev failed(DIG)");
            return -1;
        }
    }
    else
    {
        int dstid = ((board_index & 0xFF) << 8) | type;
        // xbasic::debug_output("DIG_GetID() dstid=0x%x\n",dstid);
        LOG_MSG(MSG_LOG, "DIG_GetID() dstid=0x%x",dstid);
        if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_GET_ID, errcode, value) < 0)
        {
            // xbasic::debug_output("DIG_GetID() call_get_value_from_dev failed");
            LOG_MSG(ERR_LOG, "DIG_GetID() call_get_value_from_dev failed");
            return -1;
        }
    }

    // xbasic::debug_output("DIG_GetID() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetID() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetID() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetID() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetPpsID() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(id, value.c_str());
            *id_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取板SN号
*参数: type: 板类型
*        board_index: 全局索引号, 与硬件slot对应
*        sn: 板SN
*        buf_len: sn buffer空间大小
*        sn_len: sn的实际长度
*返回值: 0: 执行成功    -1: 执行失败  -2: sn空间大小不足
*/
int DIG_GetBoardSN(board_type type, int board_index, char* sn, size_t buf_len, size_t* sn_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardSN() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardSN() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_GET_SN, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetBoardSN() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetBoardSN() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetBoardSN() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetBoardSN() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetBoardSN() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetBoardSN() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetBoardSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetBoardSN() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(sn, value.c_str());
            *sn_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取板硬件版本
*参数: type: 板类型
*        board_index: 全局索引号, 与硬件slot对应
*        version: 硬件版本
*        buf_len: version buffer空间大小
*        version_len: 硬件版本version的实际长度
*返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
*/
int DIG_GetBoardHwVersion(board_type type, int board_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardHwVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardHwVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_GET_HW_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetBoardHwVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetBoardHwVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetBoardHwVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetBoardHwVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetBoardHwVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetBoardHwVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetBoardHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetBoardHwVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取板软件版本
*参数: type: 板类型
*        board_index: 全局索引号, 与硬件slot对应
*        version: 软件版本
*        buf_len: version buffer空间大小
*        version_len: 输出的version大小
*返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
*/
int DIG_GetBoardSoftVersion(board_type type, int board_index, char* version, size_t buf_len, size_t* version_len)
{
    int errcode = 0;
    std::string value;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardSoftVersion() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardSoftVersion() dstid=0x%x",dstid);
    if (mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_GET_SOFT_VER, errcode, value) < 0)
    {
        // xbasic::debug_output("DIG_GetBoardSoftVersion() call_get_value_from_dev failed");
        LOG_MSG(ERR_LOG, "DIG_GetBoardSoftVersion() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetBoardSoftVersion() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetBoardSoftVersion() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetBoardSoftVersion() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetBoardSoftVersion() errcode:%d", errcode);
        return -1;
    }
    else
    {
        if (!value.empty())
        {
            if ((buf_len == 0) || (value.length() > (buf_len - 1)))
            {
                // xbasic::debug_output("DIG_GetBoardSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                LOG_MSG(ERR_LOG, "DIG_GetBoardSoftVersion() buffer is no enough: value size(%d), buf_len(%d)",value.length(), buf_len);
                return -2;
            }
            strcpy(version, value.c_str());
            *version_len = value.length();
        }
    }

    return 0;
}

/*
*功能: 获取某一种类型Board的基本信息(Boardid、SN、硬件版本、软件版本)
*参数: type: 板类型
*        arr: 基本信息数组指针
*        siz: 数组大小
*        real_size: 输出参数, 记录了arr 数组保存的实际数据个数
*返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
*/
int DIG_GetBoardInfo(board_type type,struct board_basicinfo* arr, int size, int *real_size)
{
    std::map<int, std::string> board_map;
    int slot_num = 0;
    board_map = get_all_board();
    //获取某一类型所有Boardid
    std::vector<int> boardid_vect;
    for (auto& it : board_map)
    {
        if ((it.first & 0xFF) == type)
        {
            boardid_vect.push_back(it.first);
        }
    }
    int vect_sz = boardid_vect.size();
    *real_size = vect_sz;
    if(vect_sz > 0)
    {
        if (vect_sz > size)
        {
            // xbasic::debug_output("DIG_GetBasicInfo() arr size(%d) is not enough(%d) \n", size, vect_sz);
            LOG_MSG(ERR_LOG, "DIG_GetBasicInfo() arr size(%d) is not enough(%d) \n", size, vect_sz);
            return -2;
        }
        else
        {
            for (int i = 0; i < vect_sz; i++)
            {
                std::string value;
                int errcode = -1;
                arr[i].board_type = type;
                arr[i].id  = boardid_vect[i];
                arr[i].slot = (boardid_vect[i] & 0xFF00) >> 8;

                //获取SN号
                if (mgr_session::get_instance()->call_get_value_from_dev(boardid_vect[i], VALUE_GET_SN, errcode, value) == 0)
                {
                    if(errcode == 0)
                    {
                        if(!value.empty())
                        {
                            strncpy(&arr[i].sn[0],value.c_str(),value.length());
                        }
                    }
                }

                //获取硬件版本
                value.clear();
                if (mgr_session::get_instance()->call_get_value_from_dev(boardid_vect[i], VALUE_GET_HW_VER, errcode, value) == 0)
                {
                    if(errcode == 0)
                    {
                        if(!value.empty())
                        {
                            strncpy(&arr[i].hw_ver[0],value.c_str(),value.length());
                        }
                    }
                }

                //获取软件版本
                value.clear();
                if (mgr_session::get_instance()->call_get_value_from_dev(boardid_vect[i], VALUE_GET_SOFT_VER, errcode, value) == 0)
                {
                    if(errcode == 0)
                    {
                        if(!value.empty())
                        {
                            strncpy(&arr[i].soft_ver[0],value.c_str(),value.length());
                        }
                    }
                }
            }
        }
    }
    else
    {
        // xbasic::debug_output("DIG_GetBasicInfo() vect_sz(%d) is zero \n", vect_sz);
        LOG_MSG(WRN_LOG, "DIG_GetBasicInfo() vect_sz(%d) is zero \n", vect_sz);
        return -1;
    }

    return 0;
}

/*
*功能: 获取各种板卡类型的所有逻辑芯片温度，MCU温度、所有电源温度、和板载的温度传感器温度
*参数: type: 板卡类型
*        id: 全局索引号, 与硬件slot对应
*        serial_num: 位号所对应的序号(从1开始)
*        temp: 温度值
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetBoardTemp(board_type type, int id, int serial_num, double* temp)
{
    // 清空
    memset(temp, 0, sizeof(double));
    std::string temp_datas;
    int dstid = ((id & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardTemp() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardTemp() dstid=0x%x",dstid);
    if (-1 == usb_get_board_temp(dstid, temp_datas))
    {
        // xbasic::debug_output("usb_get_board_temp() failed\n");
        LOG_MSG(ERR_LOG, "usb_get_board_temp() failed");
        return -1;
    }

    std::string temp_data;
    if(type == BOARDTYPE_CP_DIG || type == BOARDTYPE_CP_SYNC || type == BOARDTYPE_CP_PPS)
    {
        if(0 == find_temp_by_serialnum(temp_datas, serial_num, *temp)) // 按照二进制形式进行解析。如: id(两字节)温度(四字节)id(两字节)温度(四字节)
        {
            return 0;
        }
    }
    else
    {
        temp_data = find_string_by_serialnum(temp_datas, serial_num); // 按照字符串形式解析。如: 2:0.000000,3:0.000000,4:0.000000
        if (!temp_data.empty())
        {
            *temp = std::stod(temp_data);
            return 0;
        }
    }

    return -1;
}

/*
*功能: 获取各种板卡类型的所有逻辑芯片温度，MCU温度、所有电源温度、和板载的温度传感器温度
*参数: type: 板卡类型
*        id: 全局索引号, 与硬件slot对应
*        begin_num: 开始位号所对应的序号
*        end_num: 结束位号所对应的序号
*        buf: 温度值buffer
*        buf_len: 温度值buffer空间大小
*        length: 温度值buffer空间实际大小
*        mul_tmp: 里面有一个温度 buffer 和 sz，其中 sz 记录了查找到的温度个数
*返回值: 0: 执行成功    -1: 执行失败  -2: buf空间大小不足
*/
int DIG_GetBoardMultiTemp(board_type type, int id, int begin_num, int end_num, struct multiple_temp *mul_tmp)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetBoardMultiTemp(board_type:0x%x, id:0x%x, begin_num:%d, end_num:%d)", type, id, begin_num, end_num);
    memset(mul_tmp, 0, sizeof(struct multiple_temp));
    std::string temps_datas;
    int dstid = ((id & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardMultiTemp() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardMultiTemp() dstid=0x%x",dstid);
    if(mul_tmp == nullptr || begin_num > end_num || end_num-begin_num+1 > MAX_MULTI_SIZE)
    {
        // xbasic::debug_output("DIG_GetBoardMultiTemp() error: Illegal interval\n");
        LOG_MSG(ERR_LOG, "Exited DIG_GetBoardMultiTemp() error: Illegal interval");
        return -2;
    }

    if (-1 == usb_get_board_temp(dstid, temps_datas))
    {
        // xbasic::debug_output("DIG_GetBoardMultiTemp() usb_get_board_temp() failed");
        LOG_MSG(ERR_LOG, "Exited DIG_GetBoardMultiTemp() usb_get_board_temp() failed");
        return -1;
    }

    if(type == BOARDTYPE_CP_DIG || type == BOARDTYPE_CP_SYNC || type == BOARDTYPE_CP_PPS)
    {
        if(0 == find_temp_from_start_to_end(temps_datas, begin_num, end_num, mul_tmp))
        {
            LOG_MSG(MSG_LOG, "Exited DIG_GetBoardMultiTemp()");
            return 0;
        }
    }
    else
    {
        std::string temp_data = find_string_from_start_to_end(temps_datas, begin_num, end_num);
        if (!temp_data.empty())
        {
            // strcpy(buf, temp_data.c_str());
            // *length = temp_data.length();
            if(0 == str_to_multiple_temp(temp_data, mul_tmp))
            {
                LOG_MSG(MSG_LOG, "Exited DIG_GetBoardMultiTemp()");
                return 0;
            }
        }
    }

    LOG_MSG(MSG_LOG, "Exited DIG_GetBoardMultiTemp() return -1");
    return -1;
}

/*
*功能: 获取各种板卡电压
*参数: type: 板卡类型
*        id: 全局索引号, 与硬件slot对应
*        serial_num: 位号所对应的序号(从1开始)
*        vol: 电压值
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_GetBoardVol(board_type type, int id, int serial_num, double* vol)
{
    // 清空
    memset(vol, 0, sizeof(double));
    std::string vol_datas;
    int dstid = ((id & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardVol() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardVol() dstid=0x%x",dstid);
    if (-1 == usb_get_board_vol(dstid, vol_datas))
    {
        // xbasic::debug_output("usb_get_board_vol() failed\n");
        LOG_MSG(ERR_LOG, "usb_get_board_vol() failed");
        return -1;
    }

    std::string vol_data;
    if(type != BOARDTYPE_FT_DIG)
    {
        if(0 == find_voltage_by_serialnum(vol_datas, serial_num, *vol)) // 按照二进制形式进行解析。如: id(两字节)温度(四字节)id(两字节)温度(四字节)
        {
            return 0;
        }
    }
    else
    {
        vol_data = find_string_by_serialnum(vol_datas, serial_num);
        if (!vol_data.empty())
        {
            *vol = std::stod(vol_data);
            return 0;
        }
    }

    return -1;
}

/*
*功能: 获取各种板卡电压
*参数: type: 板卡类型
*        id: 全局索引号, 与硬件slot对应
*        begin_num: 开始位号所对应的序号
*        end_num: 结束位号所对应的序号
*        buf: 电压值buffer
*        buf_len: 电压值buffer空间大小
*        length: 电压值buffer空间实际大小
*        mul_vol: 里面有一个电压 buffer 和 sz，其中 sz 记录了查找到的电压个数。FT目前可以传空
*返回值: 0: 执行成功    -1: 执行失败  -2: buf空间大小不足
*/
int DIG_GetBoardMultiVol(board_type type, int id, int begin_num, int end_num, struct multiple_voltage *mul_vol)
{
    memset(mul_vol, 0, sizeof(struct multiple_voltage));
    std::string vol_datas;
    int dstid = ((id & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetBoardMultiVol() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetBoardMultiVol() dstid=0x%x",dstid);
    if(mul_vol == nullptr || begin_num > end_num || end_num-begin_num+1 > MAX_MULTI_SIZE)
    {
        // xbasic::debug_output("DIG_GetBoardMultiVol() error: Illegal interval");
        LOG_MSG(ERR_LOG, "DIG_GetBoardMultiVol() error: Illegal interval");
        return -2;
    }

    if (-1 == usb_get_board_vol(dstid, vol_datas))
    {
        // xbasic::debug_output("DIG_GetBoardMultiVol() usb_get_board_vol() failed");
        LOG_MSG(ERR_LOG, "Exited DIG_GetBoardMultiVol() usb_get_board_vol() failed");
        return -1;
    }

    if(type != BOARDTYPE_FT_DIG)
    {
        if(0 == find_voltage_from_start_to_end(vol_datas, begin_num, end_num, mul_vol))
        {
            return 0;
        }
    }
    else
    {
        std::string vol_data = find_string_from_start_to_end(vol_datas, begin_num, end_num);
        if (!vol_data.empty())
        {
            // strcpy(buf, vol_data.c_str());
            // *length = vol_data.length();
            if(0 == str_to_multiple_voltage(vol_data, mul_vol))
            {
                return 0;
            }
        }
    }

    return -1;
}

/*
*功能: 整板电源断电(不包括MCU本身)
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val: 1: 上电，2: 断电
*返回值: 0: 执行成功    -1: 执行失败
*/
int DIG_BoardPowerOff(board_type type,int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_BoardPowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_BoardPowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if(sw_val != POWER_SWITCH_ON && (sw_val != POWER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_BoardPowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_BoardPowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_BOARD_PS_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_BoardPowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_BoardPowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_BoardPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_BoardPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: FPGA断电(1ST250/AGBF019)
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val:
*        1: 主从逻辑同时上电
*        2: 主从逻辑同时断电
*        3: 主逻辑单独上电
*        4: 主逻辑单独断电
*        5: 从逻辑单独上电
*        6: 从逻辑单独断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_FPGAPowerOff(board_type type,int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_FPGAPowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_FPGAPowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if((sw_val < POWER_SWITCH_ON) || (sw_val > POWER_SWITCH_SLAVER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_FPGAPowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_FPGAPowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_FPGA_PS_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_FPGAPowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_FPGAPowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_FPGAPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_FPGAPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: UTP40单元电源断电
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        utp40_id: utp40的编号
*        sw_val: 1: 上电，2: 断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_UTP40PowerOff(board_type type, int board_index,int utp40_id,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_UTP40PowerOff() dstid=0x%x utp40_id=%d sw_val=%d\n",dstid,utp40_id,sw_val);
    LOG_MSG(MSG_LOG, "DIG_UTP40PowerOff() dstid=0x%x utp40_id=%d sw_val=%d",dstid,utp40_id,sw_val);
    if((sw_val != POWER_SWITCH_ON) && (sw_val != POWER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_UTP40PowerOff() dstid=0x%x utp40_id=%d sw_val=%d failed\n",dstid,utp40_id,sw_val);
        LOG_MSG(ERR_LOG, "DIG_UTP40PowerOff() dstid=0x%x utp40_id=%d sw_val=%d failed",dstid,utp40_id,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_UPT40_PS_CONTROL, sw_val, errcode, utp40_id) < 0)
    {
        // xbasic::debug_output("DIG_UTP40PowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_UTP40PowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_UTP40PowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_UTP40PowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: ASIC芯片电源断电
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val: 1: 上电，2: 断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_ASICPowerOff(board_type type, int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_ASICPowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_ASICPowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if((sw_val != POWER_SWITCH_ON) && (sw_val != POWER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_ASICPowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_ASICPowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_ASIC_PS_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_ASICPowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_ASICPowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_ASICPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_ASICPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: FPGA断电(7A 200T)
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val:
*        1: 主从逻辑同时上电
*        2: 主从逻辑同时断电
*        3: 主逻辑单独上电
*        4: 主逻辑单独断电
*        5: 从逻辑单独上电
*        6: 从逻辑单独断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_FPGA200TPowerOff(board_type type,int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_FPGA200TPowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_FPGA200TPowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if((sw_val < POWER_SWITCH_ON) || (sw_val > POWER_SWITCH_SLAVER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_FPGA200TPowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_FPGA200TPowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_FPGA200T_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_FPGA200TPowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_FPGA200TPowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_FPGA200TPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_FPGA200TPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: FPGA断电(KU060)
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val: 1: 上电，2: 断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_FPGAKU060PowerOff(board_type type,int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_FPGAKU060PowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_FPGAKU060PowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if((sw_val < POWER_SWITCH_ON) || (sw_val > POWER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_FPGAKU060PowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_FPGAKU060PowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_FPGAKU060_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_FPGAKU060PowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_FPGAKU060PowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_FPGAKU060PowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_FPGAKU060PowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: UTP40总电源断电
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        sw_val: 1: 上电，2: 断电
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_UTP40_MasterPowerOff(board_type type,int board_index,int sw_val)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_UTP40_MasterPowerOff() dstid=0x%x sw_val=%d\n",dstid,sw_val);
    LOG_MSG(MSG_LOG, "DIG_UTP40_MasterPowerOff() dstid=0x%x sw_val=%d",dstid,sw_val);
    if((sw_val < POWER_SWITCH_ON) || (sw_val > POWER_SWITCH_OFF))
    {
        // xbasic::debug_output("DIG_UTP40_MasterPowerOff() dstid=0x%x sw_val=%d failed\n",dstid,sw_val);
        LOG_MSG(ERR_LOG, "DIG_UTP40_MasterPowerOff() dstid=0x%x sw_val=%d failed",dstid,sw_val);
        return -1;
    }

    if(mgr_session::get_instance()->call_powercontrol_req(dstid, COMMAND_TYPE_UTP40_MASTER_CONTROL, sw_val, errcode, 0) < 0)
    {
        // xbasic::debug_output("DIG_UTP40_MasterPowerOff() call_powercontrol_req failed");
        LOG_MSG(ERR_LOG, "DIG_UTP40_MasterPowerOff() call_powercontrol_req failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_UTP40_MasterPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "DIG_UTP40_MasterPowerOff() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    return 0;
}

/*
*功能: 获取 9528 和 9545 锁定状态
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        c_type: 时钟芯片类型
*        status: 里面有一个状态 buffer 和 sz，其中 sz 记录了对应芯片的颗粒个数。在状态 buffer 中 0 表示未锁定 1 表示锁定
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_GetClockChipState(board_type type, int board_index, clock_type c_type, struct clock_status* status)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetClockChipState(board_type:0x%x, board_index:0x%x, clock_type:%d)", type, board_index, c_type);
    // 参数初始化
    memset(status, 0, sizeof(struct clock_status));
    int errcode = 0;
    std::string value;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetClockChipState() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetClockChipState() dstid=0x%x",dstid);
    value_type v_type = VALUE_TYPE_MAX;
    if(c_type == CLOCK_9528)
    {
        v_type = VALUE_9528_STATUS;
    }
    else if(c_type == CLOCK_9545)
    {
        v_type = VALUE_9545_STATUS;
    }
    else
    {
        // xbasic::debug_output("DIG_GetClockChipState() error: c_type=%d is not in clock_type\n",c_type);
        LOG_MSG(ERR_LOG, "DIG_GetClockChipState() error: c_type=%d is not in clock_type",c_type);
        return -1;
    }

    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, v_type, errcode, value) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetClockChipState() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetClockChipState() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetClockChipState() value=%s",value.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetClockChipState() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetClockChipState() errcode:%d", errcode);
        return -1;
    }

    // 解析原始二进制数据 value，将内容放到 status 中
    if(c_type == CLOCK_9528)
    {
        if(0 == parse_ad9528_lckdata(value, status))
        {
            // xbasic::debug_output("Exited DIG_GetClockChipState() success\n");
            LOG_MSG(MSG_LOG, "Exited DIG_GetClockChipState() success");
            return 0;
        }
        else
        {
            // xbasic::debug_output("Exited DIG_GetClockChipState() error: parse_ad9528_lckdata() failed\n");
            LOG_MSG(ERR_LOG, "Exited DIG_GetClockChipState() error: parse_ad9528_lckdata() failed");
        }
    }
    else if(c_type == CLOCK_9545)
    {
        if(0 == parse_ad9545_lckdata(value, status))
        {
            // xbasic::debug_output("Exited DIG_GetClockChipState() success\n");
            LOG_MSG(MSG_LOG, "Exited DIG_GetClockChipState() success");
            return 0;
        }
        else
        {
            // xbasic::debug_output("Exited DIG_GetClockChipState() error: parse_ad9545_lckdata() failed\n");
            LOG_MSG(ERR_LOG, "Exited DIG_GetClockChipState() error: parse_ad9545_lckdata() failed");
        }
    }

    return -1;
}

/*
*功能: 获取各个单板上的逻辑芯片时钟频率
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        p_val: 输出参数，里面有一个 array 记录了检测到的逻辑芯片的时钟频率值，sz 记录了当前逻辑芯片上的时钟通道数量，每个通道都有一个频率值，对应一个 pwm_frequency
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_GetPWMFrequency(board_type type, int board_index, struct pwm_val *p_val)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetPWMFrequency(board_type:0x%x, board_index:0x%x)", type, board_index);
    // 清空
    memset(p_val, 0, sizeof(struct pwm_val));
    int errcode = 0;
    std::string frequency_data;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetPWMFrequency() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetPWMFrequency() dstid=0x%x",dstid);
    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_PWM_FREQUENCY, errcode, frequency_data) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetPWMFrequency() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetClockChipState() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetPWMFrequency() value=%s",frequency_data.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetPWMFrequency() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetPWMFrequency() errcode:%d", errcode);
        return -1;
    }

    // 对收到的原始二进制数据进行解析
    if(0 == parse_pwm_frequency(frequency_data, p_val))
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetPWMFrequency() success");
        return 0;
    }

    return -1;
}

/*
*功能: 获取各个单板上的serdes状态
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        s_val: 输出参数，里面有一个 array 记录了检测到的serdes的状态，sz 记录了当前serdes的个数，每个serdes对应一个 struct serdes_status
*        其中 m_id 是序号，m_state 是序号对应的状态，状态值一共有一下三种:
*        0: 表示空，也就是没有这个对应的板子（也就在读取sync板和pem者或者dps之间的serdes状态的时候才会使用）
*        1: 表示锁定，也就是serdes状态正常。
*        2: 表示未锁定，也就是serdes状态不正常。
*返回值: 0: 执行成功 -1: 执行失败
*/
int DIG_GetSerdesState(board_type type, int board_index, struct serdes_val *s_val)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetSerdesState(board_type:0x%x, board_index:0x%x)", type, board_index);
    memset(s_val, 0, sizeof(struct serdes_val));
    int errcode = 0;
    std::string serdes_data;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetSerdesState() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetSerdesState() dstid=0x%x",dstid);
    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SERDES_STATUS, errcode, serdes_data) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetSerdesState() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetSerdesState() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetSerdesState() value=%s",serdes_data.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetSerdesState() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetSerdesState() errcode:%d", errcode);
        return -1;
    }

    // 对收到的原始二进制数据进行解析
    if(0 == parse_serdes_status(serdes_data, s_val))
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetSerdesState() success");
        return 0;
    }

    return -1;
}

/*
*功能: 获取各个单板上特定区间的serdes状态
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        begin_num: 开始序号(从1开始)
*        end_num: 结束序号
*        s_val: 输出参数，里面有一个 array 记录了检测到的serdes的状态，sz 记录了当前serdes的个数，每个serdes对应一个 struct serdes_status
*返回值: 0: 执行成功 -1: 执行失败 -2: buf空间错误
*/
int DIG_GetMutiSerdesState(board_type type, int board_index, int begin_num, int end_num, struct serdes_val *s_val)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetMutiSerdesState(board_type:0x%x, board_index:0x%x begin_num:%d, end_num:%d)", type, board_index, begin_num, end_num);
    memset(s_val, 0, sizeof(struct serdes_val));
    std::string serdes_datas;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetMutiSerdesState() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetMutiSerdesState() dstid=0x%x",dstid);
    if(s_val == nullptr || begin_num > end_num || end_num-begin_num+1 > MAX_SERDES_SIZE)
    {
        // xbasic::debug_output("DIG_GetMutiSerdesState() error: Illegal interval\n");
        LOG_MSG(ERR_LOG, "Exited DIG_GetMutiSerdesState() error: Illegal interval");
        return -2;
    }

    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_SERDES_STATUS, errcode, serdes_datas) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetMutiSerdesState() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetMutiSerdesState() value=%s\n",value.c_str());
    LOG_MSG(MSG_LOG, "DIG_GetMutiSerdesState() value=%s",serdes_datas.c_str());

    if (errcode < 0)
    {
        // xbasic::debug_output("DIG_GetMutiSerdesState() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetMutiSerdesState() errcode:%d", errcode);
        return -1;
    }

    if(0 == find_serdes_from_start_to_end(serdes_datas, begin_num, end_num, s_val))
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetMutiSerdesState()");
        return 0;
    }

    LOG_MSG(MSG_LOG, "Exited DIG_GetMutiSerdesState() return -1");
    return -1;
}

int DIG_GetUtpRegister(board_type type, int board_index, int chip_num, struct utp_register_val *val)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetUtpRegister(type:0x%x board_index:0x%x chip_num:%d)", type, board_index, chip_num);
    // 清空
    memset(val, 0, sizeof(struct utp_register_val));
    std::string register_val_str;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetUtpRegister() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetUtpRegister() dstid=0x%x",dstid);
    int errcode = -1;
    if(mgr_session::get_instance()->call_get_one_utp_register(dstid, chip_num, errcode, register_val_str) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetUtpRegister() call_get_one_utp_register failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetUtpRegister() value size=%d\n",register_val_str.size());
    LOG_MSG(MSG_LOG, "DIG_GetUtpRegister() value size=%d",register_val_str.size());

    if (errcode != 0)
    {
        // xbasic::debug_output("DIG_GetUtpRegister() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetUtpRegister() errcode:%d", errcode);
        return -1;
    }

    // 对收到的原始二进制数据进行解析
    if(0 == parse_utp_register_info(register_val_str, val))
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetUtpRegister()");
        return 0;
    }

    return -1;
}

int DIG_GetMutiUtpRegister(board_type type, int board_index, int begin_id, int end_id, struct utp_register_val *val_arry, int len)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetMutiUtpRegister(type:0x%x board_index:0x%x begin_id:%d, end_id:%d, len:%d)", type, board_index, begin_id, end_id, len);
    if(end_id - begin_id + 1 > 20 || (end_id < begin_id) || (end_id - begin_id + 1 > len) || len > 20)
    {
        LOG_MSG(WRN_LOG, "Exited DIG_GetMutiUtpRegister() illegal interval");
        return -2;
    }

    // 清空
    memset(val_arry, 0, sizeof(struct utp_register_val)*len);
    int errcode = -1;
    std::string register_val_str;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetMutiUtpRegister() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetMutiUtpRegister() dstid=0x%x",dstid);
    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_UTP40_REGISTER, errcode, register_val_str) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetMutiUtpRegister() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetMutiUtpRegister() value size=%d\n",register_val_str.size());
    LOG_MSG(MSG_LOG, "DIG_GetMutiUtpRegister() value size=%d",register_val_str.size());

    if (errcode != 0)
    {
        // xbasic::debug_output("DIG_GetMutiUtpRegister() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetMutiUtpRegister() errcode:%d", errcode);
        return -1;
    }

    int ret = find_utp_register_in_interval(register_val_str, begin_id, end_id, val_arry, len);
    if(ret > 0)
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetMutiUtpRegister() success find size:%d", ret);
        return ret;
    }

    LOG_MSG(MSG_LOG, "Exited DIG_GetMutiUtpRegister()");
    return -1;
}

int DIG_GetUtp102Register(board_type type, int board_index, int chip_id, struct utp102_register_val *val)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetUtp102Register(type:0x%x board_index:0x%x chip_id:%d)", type, board_index, chip_id);
    // 清空
    memset(val, 0, sizeof(struct utp102_register_val));
    std::string register_val_str;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetUtp102Register() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetUtp102Register() dstid=0x%x",dstid);
    int errcode = -1;
    if(mgr_session::get_instance()->call_get_one_utp102_register(dstid, chip_id, errcode, register_val_str) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetUtp102Register() call_get_one_utp102_register failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetUtp102Register() value size=%d\n",register_val_str.size());
    LOG_MSG(MSG_LOG, "DIG_GetUtp102Register() value size=%d",register_val_str.size());

    if (errcode != 0)
    {
        // xbasic::debug_output("DIG_GetUtp102Register() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetUtp102Register() errcode:%d", errcode);
        return -1;
    }

    // 对收到的原始二进制数据进行解析
    if(0 == parse_utp102_register_info(register_val_str, val))
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetUtp102Register()");
        return 0;
    }

    return -1;
}

int DIG_GetMutiUtp102Register(board_type type, int board_index, int begin_id, int end_id, utp102_register_val *val_arry, int len)
{
    LOG_MSG(MSG_LOG, "Enter into DIG_GetMutiUtp102Register(type:0x%x board_index:0x%x begin_id:%d, end_id:%d, len:%d)", type, board_index, begin_id, end_id, len);
    if(end_id - begin_id + 1 > 20 || (end_id < begin_id) || (end_id - begin_id + 1 > len) || len > 20)
    {
        LOG_MSG(WRN_LOG, "Exited DIG_GetMutiUtp102Register() illegal interval");
        return -2;
    }

    // 清空
    memset(val_arry, 0, sizeof(struct utp102_register_val)*len);
    int errcode = -1;
    std::string register_val_str;
    int dstid = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("DIG_GetMutiUtp102Register() dstid=0x%x\n",dstid);
    LOG_MSG(MSG_LOG, "DIG_GetMutiUtp102Register() dstid=0x%x",dstid);
    if(mgr_session::get_instance()->call_get_value_from_dev(dstid, VALUE_UTP102_REGISTER, errcode, register_val_str) < 0)
    {
        LOG_MSG(ERR_LOG, "DIG_GetMutiUtp102Register() call_get_value_from_dev failed");
        return -1;
    }

    // xbasic::debug_output("DIG_GetMutiUtp102Register() value size=%d\n",register_val_str.size());
    LOG_MSG(MSG_LOG, "DIG_GetMutiUtp102Register() value size=%d",register_val_str.size());

    if (errcode != 0)
    {
        // xbasic::debug_output("DIG_GetMutiUtp102Register() errcode:%d", errcode);
        LOG_MSG(ERR_LOG, "DIG_GetMutiUtp102Register() errcode:%d", errcode);
        return -1;
    }

    int ret = find_utp102_register_in_interval(register_val_str, begin_id, end_id, val_arry, len);
    if(ret > 0)
    {
        LOG_MSG(MSG_LOG, "Exited DIG_GetMutiUtp102Register() success find size:%d", ret);
        return ret;
    }

    LOG_MSG(MSG_LOG, "Exited DIG_GetMutiUtp102Register()");
    return -1;
}

/**
*功能: 设置4356频率
*参数: type: 板卡类型
*        board_index: 全局索引号, 与硬件slot对应
*        fre_val: 频率
*        channel: 通道，扩展字段，默认设置为 0 即可
*/
int set_4356_frequency(board_type type,int board_index,float fre_val,int channel)
{
    int errcode = 0;
    int dstid  = ((board_index & 0xFF) << 8) | type;
    // xbasic::debug_output("set_4356_frequency() dstid=0x%x fre_val=%f channel=%d\n",dstid,fre_val,channel);
    LOG_MSG(MSG_LOG, "set_4356_frequency() dstid=0x%x fre_val=%f channel=%d",dstid,fre_val,channel);
    if(mgr_session::get_instance()->call_set_frequency(dstid, FREQUENCY_4356_CONTROL, fre_val, errcode, channel) < 0)
    {
        // xbasic::debug_output("set_4356_frequency() call_set_frequency failed\n");
        LOG_MSG(ERR_LOG, "set_4356_frequency() call_set_frequency failed");
        return -1;
    }

    if (errcode < 0)
    {
        // xbasic::debug_output("set_4356_frequency() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        LOG_MSG(ERR_LOG, "set_4356_frequency() type=0x%x board_index=0x%x errcode:%d",type,board_index,errcode);
        return -1;
    }

    // xbasic::debug_output("set_4356_frequency() dstid=0x%x fre_val=%f channel=%d success\n",dstid,fre_val,channel);
    LOG_MSG(MSG_LOG, "set_4356_frequency() dstid=0x%x fre_val=%f channel=%d success",dstid,fre_val,channel);
    return 0;
}