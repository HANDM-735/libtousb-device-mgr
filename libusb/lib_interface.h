#ifndef __LIB_INTERFACE__H_H
#define __LIB_INTERFACE__H_H

#ifdef __cplusplus
extern "C" {
#endif

// 无错误(成功)
#define USB_ERR_NONE        0
// 失败
#define USB_ERR_FAIL        -1
// 参数错误
#define USB_ERR_PARAM       -1001
// 返回值错误
#define USB_ERR_RETURN      -1002
// 网络错误
#define USB_ERR_NETWORK     -1050

// 连续获取温度（电压）的最大个数
#define MAX_MULTI_SIZE      4096
// 9528 中颗粒的最大数量
#define MAX_9528_CHIP_SIZE  10
// PWM 通道最大个数
#define MAX_PWM_CHIP_SIZE   5
// 一块单板上 serdes 的最大数量
#define MAX_SERDES_SIZE     40
// 一个 UTP40 芯片上寄存器的个数
#define MAX_UTP40_REGISTER_NUM  11
// 一个 UTP102 芯片上寄存器的个数
#define MAX_UTP102_REGISTER_NUM 11
// #define MAX_UTP_CHIP_NUM    148    // UTP 芯片的最大个数

// 板卡类型枚举
typedef enum
{
    BOARDTYPE_CP_SYNC   = 0x11,
    BOARDTYPE_CP_PGB    = 0x12,
    BOARDTYPE_CP_DPS    = 0x13,
    BOARDTYPE_CP_PEM    = 0x14,
    BOARDTYPE_CP_RCA    = 0x15,
    BOARDTYPE_CP_DIG    = 0x1B,

    BOARDTYPE_ASIC      = 0x20,
    BOARDTYPE_FT_SYNC   = 0x21,
    BOARDTYPE_FT_PGB    = 0x22,
    BOARDTYPE_FT_PPS    = 0x23,
    BOARDTYPE_FT_DIG    = 0x2B,

    BOARDTYPE_TH_MONITOR = 0x40,
    BOARDTYPE_MF_MONITOR = 0x4F,

    BOARDTYPE_MAX
} board_type;

// OTA升级类型枚举
typedef enum
{
    //PGB板主FPGA
    OTA_TYPE_PGB_M_FPGA     = 0,
    OTA_TYPE_FTPGB_FPCU_FPGA = 0,
    //PGB板从FPGA
    OTA_TYPE_PGB_D_FPGA     = 1,
    OTA_TYPE_FTPGB_FPMU_FPGA = 1,
    //PGB板单片机
    OTA_TYPE_PGB_MCU        = 2,
    OTA_TYPE_FTPGB_MCU      = 2,

    //PPS板主FPGA
    OTA_TYPE_PPS_M_FPGA     = 3,
    OTA_TYPE_FTPPS_FCCU_FPGA = 3,
    //PPS板从FPGA
    OTA_TYPE_PPS_D_FPGA     = 4,
    OTA_TYPE_FTPPS_FPSU_FPGA = 4,
    //PPS板单片机
    OTA_TYPE_PPS_MCU        = 5,
    OTA_TYPE_FTPPS_MCU      = 5,

    //SYNC板FPGA
    OTA_TYPE_SYNC_FPGA      = 6,
    OTA_TYPE_FTSYNC_FXBU_FPGA = 6,
    //SYNC板单片机
    OTA_TYPE_SYNC_MCU       = 7,
    OTA_TYPE_FTSYNC_MCU     = 7,

    //ASIC板主FPGA
    OTA_TYPE_ASIC_M_FPGA    = 8,
    //ASIC板从FPGA
    OTA_TYPE_ASIC_D_FPGA    = 9,
    //ASIC板单片机
    OTA_TYPE_ASIC_MCU       = 10,
    OTA_TYPE_FTASIC_MCU     = 10,

    //CAL校准文件
    OTA_TYPE_CAL_FILE       = 11,

    //CP设备类型
    OTA_TYPE_CPPGB_ZU11_M_FPGA = 12,
    OTA_TYPE_CPPGB_S_FPGA      = 13,
    OTA_TYPE_CPPGB_MCU         = 14,

    //CPDPS板主FPGA
    OTA_TYPE_CPDPS_M_FPGA      = 15,
    OTA_TYPE_CPDPS_CCCU_FPGA   = 15,
    //CPDPS板从FPGA
    OTA_TYPE_CPDPS_S_FPGA      = 16,
    OTA_TYPE_CPDPS_CPSU_FPGA   = 16,
    OTA_TYPE_CPDPS_MCU         = 17,

    //CPSYNC板的FPGA
    OTA_TYPE_CPSYNC_FPGA       = 18,
    OTA_TYPE_CPSYNC_CXBU_FPGA  = 18,
    OTA_TYPE_CPSYNC_MCU        = 19,

    //CPPEM板200T的FPGA
    OTA_TYPE_CPPEM_FPGA        = 20,
    OTA_TYPE_CPPEM_CPMU_FPGA   = 20,
    OTA_TYPE_CPPEM_MCU         = 21,
    OTA_TYPE_CPRCA_MCU         = 22,

    //FT TH监控板MCU
    OTA_TYPE_FTTH_MCU          = 23,
    //FT MF监控板MCU
    OTA_TYPE_FTMF_MCU          = 24,

    //CP TH监控板MCU
    OTA_TYPE_CPTH_MCU          = 25,
    //CP MF监控板MCU
    OTA_TYPE_CPMF_MCU          = 26,

    //CPRCA板FPGA
    OTA_TYPE_CPRCA_CAFU_FPGA   = 27,
    //CPDIG板FPGA
    OTA_TYPE_CPDIG_CDCU_FPGA   = 28,
    //CPPEM板1ST的FPGA
    OTA_TYPE_CPPEM_CPDS_FPGA   = 29,
    //CPPEM板AGFB的FPGA
    OTA_TYPE_CPPEM_CPGM_FPGA   = 30,

    //FTFEB_FPGM的FPGA
    OTA_TYPE_FTFEB_FPGM_FPGA   = 31,
    //FTFEB_FPDS的FPGA
    OTA_TYPE_FTFEB_FPDS_FPGA   = 32,
    //FTDIG_FDCU的FPGA
    OTA_TYPE_FTDIG_FDCU_FPGA   = 33,

    //校准文件类型定义
    //CP校准文件类型
    // pem 板UTP40校准文件
    OTA_TYPE_CPPEMUTP40_MV_CAL = 40,
    OTA_TYPE_CPPEMUTP40_MI_CAL = 41,
    OTA_TYPE_CPPEMUTP40_FV_CAL = 42,
    OTA_TYPE_CPPEMUTP40_FI_CAL = 43,
    OTA_TYPE_CPPEMUTP40_ADC_CAL = 44,
    OTA_TYPE_CPPEMUTP40_IC_CAL = 45,

    // pem 板UTP102校准文件
    OTA_TYPE_CPPEMUTP102_MV_CAL = 50,
    OTA_TYPE_CPPEMUTP102_MI_CAL = 51,
    OTA_TYPE_CPPEMUTP102_FV_CAL = 52,
    OTA_TYPE_CPPEMUTP102_FI_CAL = 53,

    // dps 板UTP40校准文件
    OTA_TYPE_CPDPSUTP40_MV_CAL = 60,
    OTA_TYPE_CPDPSUTP40_MI_CAL = 61,
    OTA_TYPE_CPDPSUTP40_FV_CAL = 62,
    OTA_TYPE_CPDPSUTP40_FI_CAL = 63,
    OTA_TYPE_CPDPSUTP40_ADC_CAL = 64,
    OTA_TYPE_CPDPSUTP40_IC_CAL = 65,

    //FT校准文件类型
    // pgb 板UTP40校准文件
    OTA_TYPE_FTPGBUTP40_MV_CAL = 70,
    OTA_TYPE_FTPGBUTP40_MI_CAL = 71,
    OTA_TYPE_FTPGBUTP40_FV_CAL = 72,
    OTA_TYPE_FTPGBUTP40_FI_CAL = 73,
    OTA_TYPE_FTPGBUTP40_ADC_CAL = 74,
    OTA_TYPE_FTPGBUTP40_IC_CAL = 75,

    // pps 板UTP40校准文件
    OTA_TYPE_FTPPSUTP40_MV_CAL = 80,
    OTA_TYPE_FTPPSUTP40_MI_CAL = 81,
    OTA_TYPE_FTPPSUTP40_FV_CAL = 82,
    OTA_TYPE_FTPPSUTP40_FI_CAL = 83,
    OTA_TYPE_FTPPSUTP40_ADC_CAL = 84,
    OTA_TYPE_FTPPSUTP40_IC_CAL = 85,

    // CP PE 校准文件
    //PEM板上PE幅值校准文件
    OTA_TYPE_CPPEM_PE_AC_CAL = 90,
    //PEM板上PE相位校准文件
    OTA_TYPE_CPPEM_PE_DC_CAL = 91,

    // FT PE 校准文件
    //FEB板上PE幅值校准文件
    OTA_TYPE_FTFEB_PE_AC_CAL = 100,
    //FEB板上PE相位校准文件
    OTA_TYPE_FTFEB_PE_DC_CAL = 101,

    OTA_TYPE_MAX
} ota_type;

// 命令类型枚举
enum command_type
{
    //整板电源控制
    COMMAND_TYPE_BOARD_PS_CONTROL,
    //FPGA电源控制
    COMMAND_TYPE_FPGA_PS_CONTROL,
    //UTP40电源控制
    COMMAND_TYPE_UTP40_PS_CONTROL,
    //ASIC芯片电源控制
    COMMAND_TYPE_ASIC_PS_CONTROL,
    //PPS板控制ASIC板整板断电
    COMMAND_TYPE_PPS_ASIC_CONTROL,
    //FPGA电源控制(7A 200T)
    COMMAND_TYPE_FPGA200T_CONTROL,
    //UTP40总电源控制
    COMMAND_TYPE_UTP40_MASTER_CONTROL,
    //KU060 FPGA电源控制
    COMMAND_TYPE_FPGAKU060_CONTROL,

    COMMAND_TYPE_MAX
};

// 频率控制类型
enum frequency_type
{
    //4356频率控制
    FREQUENCY_4356_CONTROL
};

// 电源开关控制枚举
enum power_switch
{
    POWER_SWITCH_NONE,
    //上电/主从逻辑同上电
    POWER_SWITCH_ON,
    //断电/主从逻辑同断电
    POWER_SWITCH_OFF,
    //主逻辑同上电
    POWER_MASTER_SWITCH_ON,
    //主逻辑同断电
    POWER_MASTER_SWITCH_OFF,
    //从逻辑同上电
    POWER_SLAVER_SWITCH_ON,
    //从逻辑同断电
    POWER_SLAVER_SWITCH_OFF,

    POWER_SWITCH_MAX
};

// 时钟类型枚举
typedef enum
{
    CLOCK_9528,
    CLOCK_9545,
    CLOCK_MAX
} clock_type;

// 设备实时上报数据结构体
struct real_data
{
    //温度
    char*           temperature_ptr;
    //电压
    char*           voltage_ptr;
    //电流
    char*           current_ptr;
    //温度设定范围
    char*           temp_range_ptr;
    //温度告警线
    char*           temp_alarm_ptr;
    //电源状态
    char            power_status[256];
    //板在位状态
    char            board_status[256];
    //输入IO状态
    char*           input_io_status;
    //输出IO状态
    char*           output_io_status;
    //单板类型
    char            board_type[64];
    //单板sn号
    char            board_sn[64];
    //制造信息
    char            vendor_info[256];
    //硬件版本
    char            hardware_ver[256];
    //软件版本
    char            software_ver[256];
    //槽位号
    char            slot_id[256];
    //上报周期
    char            report_cycle[64];
    //通讯状态
    char            status[256];
    //累计运行时间
    char            accumulative_time[32];
    //ad9528 ppl锁定状态
    char            ad9528_ppl_lockstatus[512];
    int             ad9528_buff_len;
    //ad9545 ppl锁定状态
    char            ad9545_ppl_lockstatus[512];
    int             ad9545_buff_len;
    //pwm检测
    char            pwm_check[512];
    int             pwm_buff_len;
    //serdes状态
    char            serdes_val[256];
    //serdes数据长度
    int             serdes_val_len;
};

// 板卡ID信息结构体
struct boardids
{
    //数组实际大小
    int num;
    //short高字节是槽位号，低字节是板类型
    short ids[256];
};

// ASIC芯片结温结构体
struct asic_juncttemp
{
    //asic芯片ID
    int     asic_id;
    //asic芯片温度
    float   temp;
};

// 单板基础信息结构体
struct board_basicinfo
{
    //板类型
    board_type  board_type;
    //板ID
    int         id;
    //槽位号
    int         slot;
    //SN号
    char        sn[64];
    //硬件版本
    char        hw_ver[64];
    //软件版本
    char        soft_ver[64];
};

// 单路温度数据结构体
struct board_temp
{
    // 位号所对应的序号
    int     m_id;
    // 对应的温度
    float   m_temp_value;
};

// 存放多组温度数据结构体
struct multiple_temp
{
    struct board_temp arry[MAX_MULTI_SIZE];
    // 有效的温度数据个数
    int sz;
};

// 单路电压数据结构体
struct board_volatage
{
    // 位号所对应的序号
    int     m_id;
    // 对应的电压
    float   m_volatage_value;
};

// 多路电压数据结构体
struct multiple_volatage
{
    struct board_volatage arry[MAX_MULTI_SIZE];
    // 有效的电压数据个数
    int sz;
};

// 时钟锁定状态结构体
struct clock_status
{
    // 记录对应颗粒的锁定状态，0 表示未锁定，1 表示锁定
    int arry[MAX_9528_CHIP_SIZE];
    // 实际颗粒数
    int sz;
};

// 单路PWM频率结构体
struct pwm_frequency
{
    // 通道编号
    int     m_id;
    // 频率值
    int     m_frequency;
};

// 多路PWM频率结构体
struct pwm_val
{
    struct pwm_frequency arry[MAX_PWM_CHIP_SIZE];
    // 通道数量
    int sz;
};

// 单路serdes状态结构体
struct serdes_status
{
    // serdes 编号
    int             m_id;
    // serdes 状态
    unsigned char   m_state;
};

// 多路serdes状态结构体
struct serdes_val
{
    struct serdes_status arry[MAX_SERDES_SIZE];
    // 实际的 serdes 数量
    int sz;
};

// 寄存器信息结构体
struct register_info
{
    int8_t      valid;      // 寄存器值是否合法
    uint8_t     reg_ch;     // 通道号
    uint16_t    reg_addr;   // 寄存器地址
    uint16_t    reg_data;   // 寄存器值
};

// UTP40芯片寄存器信息结构体
struct utp_register_val
{
    // UTP 芯片编号
    short chip_id;
    // 该 UTP 芯片对应的寄存器信息
    struct register_info register_val[MAX_UTP40_REGISTER_NUM];
};

// UTP102芯片寄存器信息结构体
struct utp102_register_val
{
    // UTP 芯片编号
    short chip_id;
    // 该 UTP 芯片对应的寄存器信息
    struct register_info register_val[MAX_UTP102_REGISTER_NUM];
};

//版本信息
const char* lib_version();
/**
 * 功能：初始化lib库
 * 参数: adapter_srv 设备管理服务端地址，格式: ip:port
 *       如果adapter_srv为NULL时，则从/userdata/config/libtousb-device-mgr/g_libtousb.conf文件读取
 *       文件格式如下
 *       libserver_ip_addr=ip:port
 * 返回值: 0: 执行成功 非0: 执行失败
 */
int usb_init(const char* adapter_srv);

/**
 * 功能: 卸载usb lib库
 * 参数: 无
 * 返回值: 0
 */
int usb_uninit();

//usb ota开始升级接口函数
int usb_ota_start_upgrade(int ota_type,const char* version_num,int usb_device_addr_id);
//usb ota取消升级接口函数
int usb_ota_cancel_upgrade(int ota_type, int usb_device_addr_id);
//usb ota升级进度接口函数
int usb_ota_query_progress(int ota_type,int usb_device_addr_id,float* progress);
//usb ota升级完成接口函数
int usb_ota_complete_upgrade(int ota_type, int usb_device_addr_id);

//usb设备的实时数据获取接口函数
struct real_data* usb_fetch_real_data(int usb_device_addr_id);
//usb设备的实时数据内存释放接口函数
void usb_free_real_data(struct real_data* data_ptr);

//读取cal校准文件接口函数
int cal_read_file(int ota_type,const char* version_num,int usb_device_addr_id,int &errcode,std::string &result,const std::string &file_name="");

//获取usb板卡ID列表
int usb_get_boardids(struct boardids* borad_ids);
//读取asic芯片结温接口
int usb_get_asicjunct_temp(struct asic_juncttemp* juncts, int size);

/**
 * 功能: 向设备管理发送请求，开始向MCU传输校准文件
 * 参数:
 *      cal_type: 校准文件的类型
 *      type: 板卡类型
 *      id: 全局索引号，与硬件slot对应
 *      file_name: 要保存文件的绝对路径
 * 返回值:
 *      0: 执行成功
 *      USB_ERR_PARAM: 参数错误
 *      USB_ERR_NETWORK: 网络错误
 *      非0: 其他错误
 */
int cal_file_start_save(int cal_type,board_type type, int id,const char* file_name);

/**
 * 功能: 向设备管理发送请求，获取校准文件传输进度
 * 参数:
 *      cal_type: 校准文件的类型
 *      type: 板卡类型
 *      id: 全局索引号，与硬件slot对应
 *      progress: 输出参数，返回升级进度，进度为 1 说明文件保存成功
 * 返回值:
 *      0: 执行成功
 *      USB_ERR_PARAM: 参数错误
 *      USB_ERR_NETWORK: 网络错误
 *      非0: 其他错误
 */
int cal_file_query_progress(int cal_type,board_type type, int id,float* progress);

/**
 * 功能: 向设备管理发送通知，校准文件保存成功
 * 参数:
 *      cal_type: 校准文件的类型
 *      type: 板卡类型
 *      id: 全局索引号，与硬件slot对应
 * 返回值:
 *      0: 执行成功
 *      USB_ERR_PARAM: 参数错误
 *      USB_ERR_NETWORK: 网络错误
 */
int cal_file_complete_save(int cal_type,board_type type, int id);

/**
 * 功能: 向设备管理发送请求，开始从 MCU 读取校准文件
 * 参数:
 *      cal_type: 校准文件的类型
 *      type: 板卡类型
 *      id: 全局索引号，与硬件slot对应
 *      file_name: 读出来的文件最终要保存的绝对路径
 * 返回值:
 *      0: 执行成功
 *      USB_ERR_PARAM: 参数错误
 *      USB_ERR_NETWORK: 网络错误
 *      非0: 其他错误
 */
int cal_file_start_read(int cal_type,board_type type, int id,const char* file_name);

/**
 * 功能: 获取PGB板槽位号
 * 参数: *val: 输出slot值
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbSlot(int *val);

/**
 * 功能: 获取SYNC板槽位号
 * 参数: *val: 输出slot值
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncSlot(int *val);

/**
 * 功能: 获取ASIC板槽位号
 * 参数: *val: 输出slot值
 *       slot_nums: 要获取的单板数量，必须大于实际存在的单板
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicSlot(int *val, int slot_nums);

/**
 * 功能: 获取PPS板槽位号
 * 参数: *val: 输出slot值
 *       slot_nums: 要获取的单板数量，必须大于实际存在的单板
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsSlot(int *val, int slot_nums);

/**
 * 功能: 获取dig板在位信号
 * 参数: dig_index: 根据硬件规格是 1~16
 *       val: 在位值 1在位，0不在位
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetDigExist(int dig_index, int *val);

/**
 * 功能: 获取pps板在位信号
 * 参数: pps_index: 根据硬件规格是 1~32
 *       val: 在位值 1在位，0不在位
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsExist(int pps_index, int *val);

/**
 * 功能: 获取asic板在位信号
 * 参数: asic_index: 根据硬件规格 1~64
 *       val: 在位值 1在位，0不在位
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicExist(int asic_index, int *val);

/**
 * 功能: 获取pgb板在位信号
 * 参数: pgb_index: 根据硬件规格是 1~4
 *       val: 在位值 1在位，0不在位
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbExist(int pgb_index, int *val);

/**
 * 功能: 获取sync板在位信号
 * 参数: dev: 设备类型 0: CP 1: FT
 *       sync_index: sync板slot号
 *       val: 在位返回对应slot 不在位返回失败
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncExist(int dev,int sync_index, int *val);

/**
 * 功能: 获取sync板ID
 * 参数: dev: 设备类型 0: CP 1: FT
 *       sync_index: 全局索引号，与硬件slot对应
 *       id: 板ID
 *       buf_len: 输入的缓冲区大小
 *       id_len: 输出的id大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncID(int dev,int sync_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取sync板SN
 * 参数: dev: 设备类型 0: CP 1: FT
 *       sync_index: 全局索引号，与硬件slot对应
 *       sn: 板SN
 *       buf_len: 输入的缓冲区大小
 *       sn_len: 输出的sn大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncSN(int dev,int sync_index, char* sn, size_t buf_len, size_t* sn_len);

/**
 * 功能: 获取PGB板ID
 * 参数: pgb_index: 全局索引号，与硬件slot对应
 *       id: 板ID
 *       buf_len: 输入的缓冲区大小
 *       id_len: 输出的id大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbID(int pgb_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取PGB板SN
 * 参数: pgb_index: 全局索引号，与硬件slot对应
 *       id: 板SN
 *       buf_len: 输入的缓冲区大小
 *       sn_len: 输出的sn大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbSN(int pgb_index, char* sn, size_t buf_len, size_t* sn_len);

/**
 * 功能: 获取PPS板ID
 * 参数: pps_index: 全局索引号，与硬件slot对应
 *       id: 板ID
 *       buf_len: 输入的缓冲区大小
 *       id_len: 输出的id大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsID(int pps_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取PPS板SN
 * 参数: pps_index: 全局索引号，与硬件slot对应
 *       id: 板SN
 *       buf_len: 输入的缓冲区大小
 *       sn_len: 输出的sn大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsSN(int pps_index, char* sn, size_t buf_len, size_t* sn_len);

/**
 * 功能: 获取ASIC板ID
 * 参数: asic_index: 全局索引号，与硬件slot对应
 *       id: 板ID
 *       buf_len: 输入的缓冲区大小
 *       id_len: 输出的id大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicID(int asic_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取ASIC板SN
 * 参数: asic_index: 全局索引号，与硬件slot对应
 *       id: 板SN
 *       buf_len: 输入的缓冲区大小
 *       sn_len: 输出的sn大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicSN(int asic_index, char* sn, size_t buf_len, size_t* sn_len);

/**
 * 功能: 获取DIG板ID
 * 参数: dig_index: 全局索引号，与硬件slot对应
 *       id: 板ID
 *       buf_len: 输入的缓冲区大小
 *       id_len: 输出的id大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetDigID(int dig_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取DSA板ID
 * 参数:
 *      asic_index：全局索引号，与硬件slot对应
		id：板ID
		buf_len: 输入的缓冲区大小
		id_len: 输出的id大小
返回值：0：执行成功 	-1：执行失败
 */

int DIG_GetDsaID(int asic_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取sync板硬件版本
 * 参数:
 *      dev: 设备类型 0: CP 1: FT
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 硬件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncHwVersion(int dev,int sync_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取PGB板硬件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 硬件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbHwVersion(int pgb_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取PPS板硬件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 硬件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsHwVersion(int pps_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取ASIC板硬件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 硬件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicHwVersion(int asic_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取sync板软件版本
 * 参数:
 *      dev: 设备类型 0: CP 1: FT
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 软件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetSyncSoftVersion(int dev,int sync_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取PGB板软件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 软件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPgbSoftVersion(int pgb_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取PPS板软件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 软件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetPpsSoftVersion(int pps_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取ASIC板软件版本
 * 参数:
 *      sync_index: 全局索引号，与硬件slot对应
 *      version: 软件版本号
 *      buf_len: 输入的缓冲区大小
 *      version_len: 输出的version大小
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetAsicSoftVersion(int asic_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取DIG的版本号
 * 参数: dev: 设备类型 0: CP 1: FT
 *       dig_index: 根据硬件规格是 1~16
 *       version: DIG的版本号
 *       buf_len: version buffer空间大小
 *       version_len: version的实际长度
 * 返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
 */
int DIG_GetDigVersion(int dev,int dig_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取板是否在位
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       val: 0: 不在位，1: 在位
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetBoardExist(board_type type, int board_index, int *val);

/**
 * 功能: 获取板BoardID
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       id: 板id
 *       buf_len: id buffer空间大小
 *       id_len: id的实际长度
 * 返回值: 0: 执行成功    -1: 执行失败  -2: id空间大小不足
 */
int DIG_GetBoardID(board_type type, int board_index, char* id, size_t buf_len, size_t* id_len);

/**
 * 功能: 获取板SN号
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sn: 板sn
 *       buf_len: sn buffer空间大小
 *       sn_len: sn的实际长度
 * 返回值: 0: 执行成功    -1: 执行失败  -2: sn空间大小不足
 */
int DIG_GetBoardSN(board_type type, int board_index, char* sn, size_t buf_len, size_t* sn_len);

/**
 * 功能: 获取板硬件版本
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       version: 硬件版本
 *       buf_len: version buffer空间大小
 *       version_len: 硬件版本version的实际长度
 * 返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
 */
int DIG_GetBoardHwVersion(board_type type, int board_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取板软件版本
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       version: 软件版本
 *       buf_len: version buffer空间大小
 *       version_len: 软件版本version的实际长度
 * 返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
 */
int DIG_GetBoardSoftVersion(board_type type, int board_index, char* version, size_t buf_len, size_t* version_len);

/**
 * 功能: 获取某一种类型Board的基本信息(Boardid、SN、硬件版本、软件版本)
 * 参数: type: 板类型
 *       arr: 基本信息数据指针
 *       siz: 数组大小
 *       real_size: 输出参数，记录了 arr 数组保存的实际数据个数
 * 返回值: 0: 执行成功    -1: 执行失败  -2: version空间大小不足
 */
int DIG_GetBoardInfo(board_type type,struct board_basicinfo* arr, int size, int *real_size);

/**
 * 功能: 获取各种板卡类型的所有逻辑芯片温度，MCU温度、所有电源温度、和板载的温度传感器温度
 * 参数: type: 板卡类型
 *       id: 全局索引号，与硬件slot对应
 *       serial_num: 位号所对应的序号(从1开始)
 *       temp: 温度值
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetBoardTemp(board_type type, int id, int serial_num, double* temp);

/**
 * 功能: 获取各种板卡类型的所有逻辑芯片温度，MCU温度、所有电源温度、和板载的温度传感器温度
 * 参数: type: 板卡类型
 *       id: 全局索引号，与硬件slot对应
 *       begin_num: 开始位号所对应的序号(从1开始)
 *       end_num: 结束位号所对应的序号
 *       mul_tmp: 里面有一个温度 buffer 和 sz，其中 sz 记录了查找到的温度个数。
 * 返回值: 0: 执行成功    -1: 执行失败  -2: buf空间大小不足
 */
int DIG_GetBoardMultiTemp(board_type type, int id, int begin_num, int end_num, struct multiple_temp *mul_tmp);

/**
 * 功能: 获取各种板卡电压
 * 参数: type: 板卡类型
 *       id: 全局索引号，与硬件slot对应
 *       serial_num: 位号所对应的序号(从1开始)
 *       vol: 电压值
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_GetBoardVol(board_type type, int id, int serial_num, double* vol);

/**
 * 功能: 获取各种板卡电压
 * 参数: type: 板卡类型
 *       id: 全局索引号，与硬件slot对应
 *       begin_num: 开始位号所对应的序号
 *       end_num: 结束位号所对应的序号
 *       mul_vol: 里面有一个电压 buffer 和 sz，其中 sz 记录了查找到的电压个数。FT目前可以传空
 * 返回值: 0: 执行成功    -1: 执行失败  -2: buf空间大小不足
 */
int DIG_GetBoardMultiVol(board_type type, int id, int begin_num, int end_num, struct multiple_volatage *mul_vol);

/**
 * 功能: 整板电源断电(不包括MCU本身)
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sw_val: 1: 上电， 2: 断电
 * 返回值: 0: 执行成功    -1: 执行失败
 */
int DIG_BoardPowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: FPGA断电(1ST250/AGBF019)
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sw_val:
 *              1: 主从逻辑同时上电
 *              2: 主从逻辑同时断电断电
 *              3: 主逻辑单独上电
 *              4: 主逻辑单独断电
 *              5: 从逻辑单独上电
 *              6: 从逻辑单独断电
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_FPGAPowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: UTP40单元电源断电
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       utp40_id: utp40的编号
 *       sw_val: 1: 上电， 2: 断电
 * 返回值: 0: 执行失败 -1: 执行失败
 */
int DIG_UTP40PowerOff(board_type type,int board_index,int utp40_id,int sw_val);

/**
 * 功能: ASIC芯片电源断电
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       utp40_id: utp40的编号
 *       sw_val: 1: 上电， 2: 断电
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_ASICPowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: FPGA断电(7A 200T)
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sw_val:
 *              1: 主从逻辑同时上电
 *              2: 主从逻辑同时断电
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_FPGA200T_PowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: FPGA断电(KU060)
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sw_val: 1: 上电
 *              2: 断电
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_FPGAKU060_PowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: UTP40总电源断电
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       sw_val: 1: 上电， 2: 断电
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_UTP40_MasterPowerOff(board_type type,int board_index,int sw_val);

/**
 * 功能: 获取 9528 和 9545 锁定状态
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       c_type: 时钟芯片类型
 *       status: 里面有一个状态 buffer 和 sz，其中 sz 记录了对应芯片的颗粒个数。在状态 buffer 中 0 表示未锁定 1 表示锁定
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_GetClockChipState(board_type type, int board_index, clock_type c_type, struct clock_status* status);

/**
 * 功能: 获取各个单板上的逻辑芯片时钟频率
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       p_val: 输出参数，里面有一个 array 记录了检测到的逻辑芯片的时钟频率值，sz 记录了当前逻辑芯片上的时钟通道数量，每个通道都有一个频率值，对应一个 struct pwm_frequency
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_GetPwmFrequency(board_type type, int board_index, struct pwm_val* p_val);

/**
 * 功能: 获取各个单板上的所有serdes状态
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       s_val: 输出参数，里面有一个 array 记录了检测到的serdes的状态，sz 记录了当前serdes的个数，每个serdes对应一个 struct serdes_status
 *              其中 m_id 序号，m_state 是序号对应的状态，状态值一共有三种:
 *              0: 表示空，也就是没有这个对应的板子（也就在读取sync板和pe或者dps之间的serdes状态的时候才会使用）
 *              1: 表示锁定，也就是serdes状态正常。
 *              2: 表示未锁定，也就是serdes状态不正常。
 * 返回值: 0: 执行成功 -1: 执行失败
 */
int DIG_GetSerdesState(board_type type, int board_index, struct serdes_val* s_val);

/**
 * 功能: 获取各个单板上特定区间的serdes状态
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       begin_num: 开始序号(从1开始)
 *       end_num: 结束序号
 *       s_val: 输出参数，里面有一个 array 记录了检测到的serdes的状态，sz 记录了当前serdes的个数，每个serdes对应一个 struct serdes_status
 *              其中 m_id 序号，m_state 是序号对应的状态，状态值一共有三种:
 *              0: 表示空，也就是没有这个对应的板子（也就在读取sync板和pe或者dps之间的serdes状态的时候才会使用）
 *              1: 表示锁定，也就是serdes状态正常。
 *              2: 表示未锁定，也就是serdes状态不正常。
 * 返回值: 0: 执行成功 -1: 执行失败  -2: buffer空间错误
 */
int DIG_GetMultiSerdesState(board_type type, int board_index, int begin_num, int end_num, struct serdes_val* s_val);

/**
 * 功能: 获取某一个特定 UTP40 芯片的寄存器信息
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       chip_id: UTP 芯片编号，DPS板[1,148]，PEM板[1,12]
 *       val: 输出参数，返回 chip_id 对应 UTP 芯片的寄存器信息
 */
int DIG_GetUtpRegister(board_type type, int board_index, int chip_id, struct utp_register_val* val);

/**
 * 功能: 获取一段连续 UTP40 芯片的寄存器信息
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       begin_id: 起始芯片编号
 *       end_id: 结束芯片编号
 *       val_arry: 应该传入一个 struct utp_register_val 数组，返回[begin_id, end_id]对应 UTP 芯片的寄存器信息
 *       len: 传入数组的大小，不得超过20
 * 注意: [begin_id, end_id] 区间范围不得超过 20 ，end_id-begin_id+1 不得大于20
 * 返回值: 实际获取到的 UTP 芯片个数
 *        -1: 获取失败
 *        -2: 区间错误
 */
int DIG_GetMultiUtpRegister(board_type type, int board_index, int begin_id, int end_id, struct utp_register_val* val_arry, int len);

/**
 * 功能: 获取某一个特定 UTP102 芯片的寄存器信息
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       chip_id: UTP 芯片编号，PEM板[1, 16]
 *       val: 输出参数，返回 chip_id 对应 UTP 芯片的寄存器信息
 */
int DIG_GetUtp102Register(board_type type, int board_index, int chip_id, struct utp102_register_val* val);

/**
 * 功能: 获取一段连续 UTP102 芯片的寄存器信息
 * 参数: type: 板卡类型
 *       board_index: 全局索引号，与硬件slot对应
 *       begin_id: 起始芯片编号
 *       end_id: 结束芯片编号
 *       val_arry: 输出参数，返回[begin_id, end_id]对应 UTP 芯片的寄存器信息
 *       len: 传入数组的大小，不得超过20
 * 注意: [begin_id, end_id] 区间范围不得超过 20 ，end_id-begin_id+1 不得大于20
 *       PEM 板 UTP102 的数量是 16 个
 * 返回值: 实际获取到的 UTP 芯片个数
 *        -1: 获取失败
 *        -2: 区间错误
 */
int DIG_GetMultiUtp102Register(board_type type, int board_index, int begin_id, int end_id, struct utp102_register_val* val_arry, int len);

/**
 * 功能: 设置4356的频率
 * 参数: type: 板类型
 *       board_index: 全局索引号，与硬件slot对应
 *       fre_val: 频率值
 *       channel: 通道，扩展字段，默认设置为 0 即可
 */
int set_4356_frequency(board_type type,int board_index,float fre_val,int channel);

#ifdef __cplusplus
}
#endif

#endif