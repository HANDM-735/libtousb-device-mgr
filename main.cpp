#ifdef  _WINDOWS //WINDOWS平台
#pragma warning(disable:4996)
#else //非WINDOWS平台
#include <unistd.h>
#include <termios.h>
#endif
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include "xbasic.hpp"
#include "xconfig.hpp"
#include "mgr_network.h"
#include "mgr_session.h"
#include "lib_interface.h"
#include "mgr_shm.h"
#include "xconvert.hpp"
#include <stdio.h>
#include <string.h>
#include <vector>

//源端设备地址（槽位号+板类型）
#define SRC_ADDR_ID        0x0010
//目的端设备地址（槽位号+板类型）
#define DST_ADDR_ID        0x8010


struct voltage_item
{
    char name[32];
    int  index;
    float value;
};


struct voltage
{
    int channel_count;
    voltage_item items[512];
};

struct temp_item
{
    char name[32];
    int index;
    float value;
};

struct temp
{
    int temp_cout;
    struct temp_item items[512];
};

struct frequency_item
{
    char name[32];
    int  index;
    int  value;
};


struct frequency
{
    int channel_count;
    frequency_item items[64];
};


struct lockstate_item
{
    char name[32];
    int  index;
    float value;
};

struct chip_item
{
    int channel_count;
    int state;
    struct lockstate_item items[64];
};

struct ad9528_lockstate
{
    int chip_sum;
    struct chip_item chips[64];
};

struct ad9545_lockstate
{
    struct chip_item chip;
};


static void print_voltage(voltage* pVol)
{
    if(pVol == NULL)
    {
        return ;
    }

    printf("Voltage.channel_count=%d\n",pVol->channel_count);
    int item_size = sizeof(pVol->items)/sizeof(pVol->items[0]);

    for(int i = 0; i < pVol->channel_count && i < item_size; i++)
    {
        printf("Voltage.%s=%f\n",pVol->items[i].name,pVol->items[i].value);
    }
}

static void parse_voltage(char* buff,struct voltage* pVol)
{
    if(buff == NULL || pVol == NULL)
    {
        return ;
    }

    //通道总数  4个字节
    int offset = 0;
    pVol->channel_count = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;

    //防止数组越界
    int channel_count = pVol->channel_count;
    int item_size = sizeof(pVol->items)/sizeof(pVol->items[0]);
    if(channel_count > item_size)
    {
        channel_count = item_size;
    }

    for(int i = 0 ; i < channel_count; i++)
    {
        //解析每个通道的电压值
        sprintf(pVol->items[i].name,"ch_%d",i);
        pVol->items[i].index = i;
        pVol->items[i].value = xbasic::readfloat_bigendian(&buff[offset],4);
        offset += 4;
    }

}

static void print_temp(struct temp* temp)
{
    if(temp == NULL)
    {
        return ;
    }

    printf("Temp.count=%d\n",temp->temp_cout);
    int item_size = sizeof(temp->items)/sizeof(temp->items[0]);

    for(int i = 0; i < temp->temp_cout && i < item_size; i++)
    {
        printf("Temp.%s=%f\n",temp->items[i].name,temp->items[i].value);
    }
}

static void parse_temp(char* buff,struct temp* ptemp)
{
    if(buff == NULL || ptemp == NULL)
    {
        return ;
    }

    //通道总数  4个字节
    int offset = 0;
    ptemp->temp_cout = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;

    //防止数组越界
    int temp_count = ptemp->temp_cout;
    int item_size = sizeof(ptemp->items)/sizeof(ptemp->items[0]);
    if(temp_count > item_size)
    {
        temp_count = item_size;
    }

    for(int i = 0 ; i < temp_count; i++)
    {
        //解析每个通道的温度值
        sprintf(ptemp->items[i].name,"num_%d",i);
        ptemp->items[i].index = i;
        ptemp->items[i].value = xbasic::readfloat_bigendian(&buff[offset],4);
        offset += 4;
    }
  
}

static void print_serdes(struct serdes_val *s_val)
{
    if(s_val == NULL)
    {
        return ;
    }

    printf("Serdes.count=%d\n",s_val->sz);
    int item_size = s_val->sz;

    for(int i = 0; i < item_size; i++)
    {
        printf("Serdes.num_%d=%d\n", s_val->arry[i].m_id, s_val->arry[i].m_state);
    }
}

static int parse_serdes(const std::string& base_value, struct serdes_val *s_val)
{
    int ret = 0;
    // LOG_MSG(MSG_LOG, "Enter into parse_serdes_status()");
    //1、先对原始的二进制数据进行解析
    std::vector<serdes_state> vct_serdes;
    int r = xconvert::parse_serdes(base_value, vct_serdes);
    if(r != 0)
    {
        // 数据解析失败直接返回
        ret = -1;
        // LOG_MSG(ERR_LOG, "Exited parse_serdes_status() error: value parse failed!");
    }
    else
    {
        // 到这里说明数据解析成功，此时数据已经被存放到 vct_serdes 中了
        int sz = vct_serdes.size();
        if(sz > MAX_SERDES_SIZE)
        {
            ret = -2;
            // LOG_MSG(WRN_LOG, "parse_serdes_status() warning: space is not enough!");
        }
        s_val->sz = sz > MAX_SERDES_SIZE ? MAX_SERDES_SIZE : sz;
        int end = s_val->sz;
        // LOG_MSG(MSG_LOG, "parse_serdes_status() vct_serdes.size:%d s_val->sz:%d end:%d", sz, s_val->sz, end);
        for(int i = 0; i < s_val->sz; ++i)
        {
            s_val->arry[i].m_id = vct_serdes[i].m_id;
            s_val->arry[i].m_state = vct_serdes[i].m_serdes_value;
            // LOG_MSG(MSG_LOG, "parse_serdes_status() s_val->arry[%d].m_id=%d, s_val->arry[%d].m_state=%xhu, i, s_val->arry[i].m_id, i, s_val->arry[i].m_state);
        }
    }
    // LOG_MSG(MSG_LOG, "Exited parse_serdes_status() ret=%d", ret);
    return ret;
}

static void print_frequency(frequency* pFreq)
{
    if(pFreq == NULL)
    {
        return ;
    }

    printf("Frequency.channel_count=%d\n", pFreq->channel_count);
    int item_size = sizeof(pFreq->items)/sizeof(pFreq->items[0]);

    for(int i = 0; i < pFreq->channel_count && i < item_size; i++)
    {
        printf("Frequency.%s=%d\n",pFreq->items[i].name,pFreq->items[i].value);
    }
}

static void parse_frequency(char* buff,struct frequency* pFreq)
{
    if(buff == NULL || pFreq == NULL)
    {
        return ;
    }

    //通道总数 4个字节
    int offset = 0;
    pFreq->channel_count = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;

    //防止数组越界
    int channel_count = pFreq->channel_count;
    int item_size = sizeof(pFreq->items)/sizeof(pFreq->items[0]);
    if(channel_count > item_size)
    {
        channel_count = item_size;
    }

    for(int i = 0 ; i < channel_count; i++)
    {
        //解析每个通道的频率值
        sprintf(pFreq->items[i].name,"pwm_%d",i);
        pFreq->items[i].index = i;
        pFreq->items[i].value = xbasic::read_bigendian(&buff[offset],4);
        offset += 4;
    }
}

static void print_ad9528_lockstate(struct ad9528_lockstate* pAD9528)
{
    if(pAD9528 == NULL)
    {
        return ;
    }

    printf("AD9528.chip_sum=%d\n", pAD9528->chip_sum);
    int chiparr_size = sizeof(pAD9528->chips)/sizeof(pAD9528->chips[0]);

    for(int i = 0; i < pAD9528->chip_sum && i < chiparr_size; i++)
    {
        printf("AD9528.chips[%d].channel_count=%d\n", i,pAD9528->chips[i].channel_count);
        printf("AD9528.chips[%d].state=%d\n", i,pAD9528->chips[i].state);
        int items_size = sizeof(pAD9528->chips[i].items)/sizeof(pAD9528->chips[i].items[0]);

        for(int j = 0 ; j < pAD9528->chips[i].channel_count && j < items_size; j++)
        {
            printf("AD9528.chips[%d].channel[%d].value=%f\n", i,j,pAD9528->chips[i].items[j].value);
        }
    }
}

static void parse_ad9528_lockstate(char* buff,struct ad9528_lockstate* pAD9528)
{
    if(buff == NULL || pAD9528 == NULL)
    {
        return ;
    }

    //9528总数  4个字节
    int offset = 0;
    pAD9528->chip_sum = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;

    //防止数组越界
    int chip_count = pAD9528->chip_sum ;
    int chiparr_size = sizeof(pAD9528->chips)/sizeof(pAD9528->chips[0]);
    if(chip_count > chiparr_size)
    {
        chip_count = chiparr_size;
    }

    for(int i = 0 ; i < chip_count; i++)
    {
        //解析每个9528颗粒
        //每个9528的通道总数
        pAD9528->chips[i].channel_count = xbasic::read_bigendian(&buff[offset],4);
        offset += 4;
        pAD9528->chips[i].state = xbasic::read_bigendian(&buff[offset],4);
        offset += 4;

        int items_size = sizeof(pAD9528->chips[i].items)/sizeof(pAD9528->chips[i].items[0]);
        for(int j = 0; j < pAD9528->chips[i].channel_count; j++)
        {
            if(j < items_size)  //防止数组越界，超过数组大小的buff将不进行解析
            {
                sprintf(pAD9528->chips[i].items[j].name,"ad9528_%d_ch_%d",i,j);
                pAD9528->chips[i].items[j].index = j;
                pAD9528->chips[i].items[j].value = xbasic::readfloat_bigendian(&buff[offset],4);
            }
            //注意，超过数组大小的buff的偏移必须后移，否则将影响下一个9528的颗粒的buff解析
            offset += 4;
        }
    }
}

static void print_ad9545_lockstate(struct ad9545_lockstate* pAD9545)
{
    if(pAD9545 == NULL)
    {
        return ;
    }

    printf("AD9545.chip.channel_count=%d\n", pAD9545->chip.channel_count);
    printf("AD9545.chip.state=%d\n", pAD9545->chip.state);
    int item_size = sizeof(pAD9545->chip.items)/sizeof(pAD9545->chip.items[0]);

    for(int i = 0; i< pAD9545->chip.channel_count && i < item_size; i++)
    {
        printf("AD9545.chip.channel[%d].value=%f\n", i,pAD9545->chip.items[i].value);
    }
}

static void parse_ad9545_lockstate(char* buff,struct ad9545_lockstate* pAD9545)
{
    if(buff == NULL || pAD9545 == NULL)
    {
        return ;
    }

    //9545通道总数  4个字节
    int offset = 0;
    pAD9545->chip.channel_count = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;
    //解析9545的状态
    pAD9545->chip.state = xbasic::read_bigendian(&buff[offset],4);
    offset += 4;

    //防止数组越界
    int channel_count = pAD9545->chip.channel_count ;
    int item_size = sizeof(pAD9545->chip.items)/sizeof(pAD9545->chip.items[0]);
    if(channel_count > item_size)
    {
        channel_count = item_size;
    }

    for(int i = 0 ; i <  channel_count; i++)
    {
        //解析每个通道的频率值
        sprintf(pAD9545->chip.items[i].name,"ad9545_ch_%d",i);
        pAD9545->chip.items[i].index = i;
        pAD9545->chip.items[i].value = xbasic::readfloat_bigendian(&buff[offset],4);
        offset += 4;
    }
}

static void load_config() //加载配置文件
{
    xini_config xini_cfg;
    xini_cfg.set_file(std::string(xbasic::get_module_path())+"config.ini");
    xconfig *sys_config = xconfig::get_instance();
    sys_config->set_data("debug",xini_cfg.get_data("SYS_CONFIG.debug",1));
    sys_config->set_data("adapter-server",xini_cfg.get_data("SYS_CONFIG.adapter_server","172.16.4.33:9000"));

    std::string usb_device_addr_id = xini_cfg.get_data("SYS_CONFIG.usb_device_addr_id","");
    int usd_dst_id = 0;
    if(!usb_device_addr_id.empty())
    {
        xbasic::trim(usb_device_addr_id);  //去掉前后的空白字符
        sscanf(usb_device_addr_id.c_str(),"0x%x",&usd_dst_id);
        sys_config->set_data("usb_device_addr_id",usd_dst_id);
    }

}

static void print_real_data(struct real_data* data_ptr)
{
    if(data_ptr == NULL) {
        return ;
    }

    printf("======= real_data information print =================\n");

    if(data_ptr->temperature_ptr != NULL) {
        // printf("data_ptr->temperature_ptr=%s\n",data_ptr->temperature_ptr);
        struct temp tp;
        memset(&tp, 0, sizeof(struct temp));
        parse_temp(data_ptr->temperature_ptr,&tp);
        print_temp(&tp);
    }

    if(data_ptr->voltage_ptr != NULL) {
        //printf("data_ptr->voltage_ptr=%s\n",data_ptr->voltage_ptr);
        struct voltage volt;
        parse_voltage(data_ptr->voltage_ptr,&volt);
        print_voltage(&volt);
    }

    if(data_ptr->current_ptr != NULL) {
        printf("data_ptr->current_ptr=%s\n",data_ptr->current_ptr);
    }

    if(data_ptr->temp_range_ptr != NULL) {
        printf("data_ptr->temp_range_ptr=%s\n",data_ptr->temp_range_ptr);
    }

    if(data_ptr->temp_alarm_ptr != NULL) {
        printf("data_ptr->temp_alarm_ptr=%s\n",data_ptr->temp_alarm_ptr);
    }

    if(strlen(data_ptr->power_status) != 0) {
        printf("data_ptr->power_status=%s\n",data_ptr->power_status);
    }

    if(strlen(data_ptr->board_status) != 0) {
        printf("data_ptr->board_status=%s\n",data_ptr->board_status);
    }

    if(data_ptr->input_io_status != NULL) {
        printf("data_ptr->input_io_status=%s\n",data_ptr->input_io_status);
    }

    if(data_ptr->output_io_status != NULL) {
        printf("data_ptr->output_io_status=%s\n",data_ptr->output_io_status);
    }

    if(strlen(data_ptr->board_type) != 0) {
        printf("data_ptr->board_type=%s\n",data_ptr->board_type);
    }

    if(strlen(data_ptr->board_sn) != 0) {
        printf("data_ptr->board_sn=%s\n",data_ptr->board_sn);
    }

    if(strlen(data_ptr->vendor_info) != 0) {
        printf("data_ptr->vendor_info=%s\n",data_ptr->vendor_info);
    }

    if(strlen(data_ptr->hardware_ver) != 0) {
        printf("data_ptr->hardware_ver=%s\n",data_ptr->hardware_ver);
    }

    if(strlen(data_ptr->software_ver) != 0) {
        printf("data_ptr->software_ver=%s\n",data_ptr->software_ver);
    }

    if(strlen(data_ptr->slot_id) != 0) {
        printf("data_ptr->slot_id=%s\n",data_ptr->slot_id);
    }

    if(strlen(data_ptr->report_cycle) != 0) {
        printf("data_ptr->report_cycle=%s\n",data_ptr->report_cycle);
    }

    if(strlen(data_ptr->status) != 0) {
        printf("data_ptr->status=%s\n",data_ptr->status);
    }

    if(strlen(data_ptr->accumulative_time) != 0) {
        printf("data_ptr->accumulative_time=%s\n",data_ptr->accumulative_time);
    }

    if(data_ptr->ad9528_buff_len != 0) {
        struct ad9528_lockstate ad9528_lock;
        parse_ad9528_lockstate(data_ptr->ad9528_ppl_lockstatus,&ad9528_lock);
        print_ad9528_lockstate(ad9528_lock);
    }

    if(data_ptr->ad9545_buff_len != 0) {
        struct ad9545_lockstate ad9545_lock;
        parse_ad9545_lockstate(data_ptr->ad9545_ppl_lockstatus,&ad9545_lock);
        print_ad9545_lockstate(ad9545_lock);
    }

    if(data_ptr->pwm_buff_len != 0) {
        struct frequency freq;
        parse_frequency(data_ptr->pwm_check,&freq);
        print_frequency(&freq);
    }

    if(data_ptr->serdes_val_len != 0) {
        struct serdes_val s_val;
        memset(&s_val, 0, sizeof(struct serdes_val));
        std::string val(data_ptr->serdes_val, data_ptr->serdes_val_len);
        parse_serdes(val,&s_val);
        print_serdes(&s_val);
    }

    printf("=====================================================\n");

    return ;
}

static void print_asic_junction_temp(struct asic_juncttemp* temps, int size)
{
    printf("======= ASIC junction Temperature print =================\n");
    for(int i = 0; i < size; i++)
    {
        printf("asicid[%d]=%f\n",temps[i].asic_id,temps[i].temp);
    }
    printf("=====================================================\n");
}

//测试开始升级case
static void test1()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = usb_ota_start_upgrade(OTA_TYPE_SYNC_MCU,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d.\n",ret);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        usb_ota_query_progress(OTA_TYPE_SYNC_MCU,dst_addr_id,&progress);
        xbasic::debug_output("current upgrade progress value is %0.2f.\n",progress);
        if(progress == 1.0) {
            xbasic::debug_output("ota upgrade have completed now!\n");
            break;
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试取消升级case
static void test2()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = usb_ota_start_upgrade(OTA_TYPE_SYNC_MCU,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d.\n",ret);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        usb_ota_query_progress(OTA_TYPE_SYNC_MCU,dst_addr_id,&progress);
        xbasic::debug_output("current upgrade progress value is %0.2f.\n",progress);
        if(progress >= 0.5) {
            xbasic::debug_output("ota upgrade is upgrading,be will to cancle the ota upgrade.\n");
            usb_ota_cancel_upgrade(OTA_TYPE_SYNC_MCU,dst_addr_id);
            break;
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试升级完成case
static void test3()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = usb_ota_start_upgrade(OTA_TYPE_SYNC_MCU,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d.\n",ret);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        usb_ota_query_progress(OTA_TYPE_SYNC_MCU,dst_addr_id,&progress);
        xbasic::debug_output("current upgrade progress value is %0.2f.\n",progress);
        if(progress >= 1.0) {
            xbasic::debug_output("ota upgrade is completed.\n");
            break;
        }
    }
    usb_ota_complete_upgrade(OTA_TYPE_SYNC_MCU,dst_addr_id);

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试实时数据获取case
static void test4()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int time = 1;
    while(true) {
        xbasic::debug_output("read real data.\n");
        struct real_data* data = NULL;
        data=usb_fetch_real_data(dst_addr_id);
        print_real_data(data);
        usb_free_real_data(data);
        data=NULL;

        xbasic::debug_output("read real data time=%d.\n",time);
        if(time >= 5) {
            break;
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试写入cal校准文件
static void test5()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = usb_ota_start_upgrade(OTA_TYPE_CAL_FILE,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d.\n",ret);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        usb_ota_query_progress(OTA_TYPE_CAL_FILE,dst_addr_id,&progress);
        xbasic::debug_output("current upgrade progress value is %0.2f.\n",progress);
        if(progress == 1.0) {
            xbasic::debug_output("ota upgrade have completed now!\n");
            break;
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试写入cal校准文件
static void test6()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = cal_read_file(OTA_TYPE_CAL_FILE,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("cal_read_file() failed,ret=%d.\n",ret);
        goto END;
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

//测试升级完成case + 实时实数同时
static void test7()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr = sys_config->get_data("adapter_server");
    int  dst_addr_id = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = usb_ota_start_upgrade(OTA_TYPE_SYNC_MCU,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d.\n",ret);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        usb_ota_query_progress(OTA_TYPE_SYNC_MCU,dst_addr_id,&progress);
        xbasic::debug_output("current upgrade progress value is %0.2f.\n",progress);
        if(progress >= 1.0) {
            xbasic::debug_output("ota upgrade is completed.\n");
            break;
        }

        struct real_data* data = NULL;
        data=usb_fetch_real_data(dst_addr_id);
        print_real_data(data);
        usb_free_real_data(data);
        data=NULL;
    }
    usb_ota_complete_upgrade(OTA_TYPE_SYNC_MCU,dst_addr_id);

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test8()
{
    xbasic::debug_output("usb lib initialization\n");

    usb_init(NULL);

    xbasic::debug_output("usb lib initialization() successfully.\n");
}

static void usage()
{
    printf("Usage: \n");
    printf("\t otaupgrade <otatype> \n");
    printf("\t read_real <times> \n");
    printf("\t read_cal \n");
    printf("\t ctrl C \t--exit the program.\r\n");

    printf("Example:\n");
    printf("\t otaupgrade SYNC_MCU \n\n\n");
    printf("ota type value as following:\n");
    printf("\t [PGB_M_FPGA PGB_D_FPGA PGB_MCU PPS_M_FPGA \n");
    printf("\t PPS_D_FPGA PPS_MCU SYNC_FPGA SYNC_MCU \n");
    printf("\t ASIC_M_FPGA ASIC_D_FPGA ASIC_MCU CAL_FILE]\n\n");
    printf("\n\t read_real 3 or read_real \n\n\n");
    printf("\t <times> is read count,when this option is ignore,read forever \n\n\n");

    return ;
}

static int get_otatype_param(const char* param)
{
    int ret = -1;
    if(strcmp(param,"PGB_M_FPGA") == 0) {
        ret = OTA_TYPE_PGB_M_FPGA;
    }

    if(strcmp(param,"FTPGB_FPCU_FPGA") == 0) {
        //PGB板主FPGA新版命名方式
        ret = OTA_TYPE_FTPGB_FPCU_FPGA;
    }

    if(strcmp(param,"PGB_D_FPGA") == 0) {
        ret = OTA_TYPE_PGB_D_FPGA;
    }

    if(strcmp(param,"FTPGB_FPMU_FPGA") == 0) {
        //PGB板从FPGA新版命名方式
        ret = OTA_TYPE_FTPGB_FPMU_FPGA;
    }

    if(strcmp(param,"PGB_MCU") == 0) {
        ret = OTA_TYPE_PGB_MCU;
    }

    if(strcmp(param,"FTPGB_MCU") == 0) {
        // FTPGB_MCU 新版本命名
        ret = OTA_TYPE_FTPGB_MCU;
    }

    if(strcmp(param,"PPS_M_FPGA") == 0) {
        ret = OTA_TYPE_PPS_M_FPGA;
    }

    if(strcmp(param,"FTTPS_FCCU_FPGA") == 0) {
        //PPS板主FPGA新版命名方式
        ret = OTA_TYPE_FTTPS_FCCU_FPGA;
    }

    if(strcmp(param,"PPS_D_FPGA") == 0) {
        ret = OTA_TYPE_PPS_D_FPGA;
    }

    if(strcmp(param,"FTTPS_FPSU_FPGA") == 0) {
        //PPS板从FPGA新版本命名方式
        ret = OTA_TYPE_FTTPS_FPSU_FPGA;
    }

    if(strcmp(param,"PPS_MCU") == 0) {
        ret = OTA_TYPE_PPS_MCU;
    }

    if(strcmp(param,"FTTPS_MCU") == 0) {
        //FTTPS_MCU 新版本命名
        ret = OTA_TYPE_FTTPS_MCU;
    }

    if(strcmp(param,"SYNC_FPGA") == 0) {
        ret = OTA_TYPE_SYNC_FPGA;
    }

    if(strcmp(param,"FTSYNC_FXBU_FPGA") == 0) {
        //FTSYNC板FPGA新版本命名方式
        ret = OTA_TYPE_FTSYNC_FXBU_FPGA;
    }

    if(strcmp(param,"SYNC_MCU") == 0) {
        ret = OTA_TYPE_SYNC_MCU;
    }

    if(strcmp(param,"FTSYNC_MCU") == 0) {
        //FTSYNC_MCU新版本命名
        ret = OTA_TYPE_FTSYNC_MCU;
    }

    if(strcmp(param,"ASIC_M_FPGA") == 0) {
        ret = OTA_TYPE_ASIC_M_FPGA;
    }

    if(strcmp(param,"ASIC_D_FPGA") == 0) {
        ret = OTA_TYPE_ASIC_D_FPGA;
    }

    if(strcmp(param,"ASIC_MCU") == 0) {
        ret = OTA_TYPE_ASIC_MCU;
    }

    if(strcmp(param, "FTASIC_MCU") == 0) {
        //FTASIC_MCU 新版本命名
        ret = OTA_TYPE_FTASIC_MCU;
    }

    if(strcmp(param,"CAL_FILE") == 0) {
        ret = OTA_TYPE_CAL_FILE;
    }

    if(strcmp(param,"CPPGB_ZU11_M_FPGA") == 0) {
        ret = OTA_TYPE_CPPGB_ZU11_M_FPGA;
    }

    if(strcmp(param,"CPPGB_S_FPGA") == 0) {
        ret = OTA_TYPE_CPPGB_S_FPGA;
    }

    if(strcmp(param,"CPPGB_MCU") == 0) {
        ret = OTA_TYPE_CPPGB_MCU;
    }

    if(strcmp(param,"CPDPS_M_FPGA") == 0) {
        ret = OTA_TYPE_CPDPS_M_FPGA;
    }

    if(strcmp(param,"CPDPS_S_FPGA") == 0) {
        ret = OTA_TYPE_CPDPS_S_FPGA;
    }

    if(strcmp(param,"CPDPS_MCU") == 0) {
        ret = OTA_TYPE_CPDPS_MCU;
    }

    if(strcmp(param,"CPSYNC_FPGA") == 0) {
        ret = OTA_TYPE_CPSYNC_FPGA;
    }

    if(strcmp(param,"CPSYNC_MCU") == 0) {
        ret = OTA_TYPE_CPSYNC_MCU;
    }

    if(strcmp(param,"CPPEM_FPGA") == 0) {
    }

    if(strcmp(param,"CPPEM_MCU") == 0) {
        ret = OTA_TYPE_CPPEM_MCU;
    }

    if(strcmp(param,"CPRCA_MCU") == 0) {
        ret = OTA_TYPE_CPRCA_MCU;
    }

    if(strcmp(param,"FTTH_MCU") == 0) {
        ret = OTA_TYPE_FTTH_MCU;
    }

    if(strcmp(param,"FTMF_MCU") == 0) {
        ret = OTA_TYPE_FTMF_MCU;
    }

    if(strcmp(param,"CPTH_MCU") == 0) {
        ret = OTA_TYPE_CPTH_MCU;
    }

    if(strcmp(param,"CPMF_MCU") == 0) {
        ret = OTA_TYPE_CPMF_MCU;
    }

    if(strcmp(param,"CPPEM_CPMU_FPGA") == 0) {
        //CPPEM板200T的FPGA
        ret = OTA_TYPE_CPPEM_CPMU_FPGA;
    }

    if(strcmp(param,"CPSYNC_CXBU_FPGA") == 0) {
        //CPSYNC板的FPGA
        ret = OTA_TYPE_CPSYNC_CXBU_FPGA;
    }

    if(strcmp(param,"CPDPS_CCCU_FPGA") == 0) {
        //CPDPS板主FPGA
        ret = OTA_TYPE_CPDPS_CCCU_FPGA;
    }

    if(strcmp(param,"CPDPS_CPSU_FPGA") == 0) {
        //CPDPS板从FPGA
        ret = OTA_TYPE_CPDPS_CPSU_FPGA;
    }

    if(strcmp(param,"CPRCA_CAFU_FPGA") == 0) {
        //CPRCA板FPGA
        ret = OTA_TYPE_CPRCA_CAFU_FPGA;
    }

    if(strcmp(param,"CPDIG_CDCU_FPGA") == 0) {
        //CPDIG板FPGA
        ret = OTA_TYPE_CPDIG_CDCU_FPGA;
    }

    if(strcmp(param,"CPPEM_CPDS_FPGA") == 0) {
        //CPPEM板1ST的FPGA
        ret = OTA_TYPE_CPPEM_CPDS_FPGA;
    }

    if(strcmp(param,"CPPEM_CPGM_FPGA") == 0) {
        //CPPEM板AGFB的FPGA
        ret = OTA_TYPE_CPPEM_CPGM_FPGA;
    }

    return ret;
}

static void ota_upgrade(const char* type_str,const char* version_str)
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr = sys_config->get_data("adapter_server");
    int  dst_addr_id = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    std::string ota_type_str(type_str);
    xbasic::trim(ota_type_str);
    std::transform(ota_type_str.begin(),ota_type_str.end(),ota_type_str.begin(),::toupper); //将ota type参数转换成大写
    int ota_type = get_otatype_param(ota_type_str.c_str());

    std::string version(version_str);
    xbasic::trim(version);

    int ret = usb_ota_start_upgrade(ota_type,version.c_str(),dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("usb_ota_start_upgrade() failed,ret=%d ota_type=%d dst_addr_id=0x%X.\n",ret,ota_type,dst_addr_id);
        goto END;
    }
    xbasic::debug_output("usb_ota_start_upgrade() successfully.\n");

    while(true) {
        sleep(5);
        float progress = 0.0;
        int ret = usb_ota_query_progress(ota_type,dst_addr_id,&progress);
        if(ret == 0)
        {
            // 获取进度成功
            xbasic::debug_output("current upgrade progress value is %f.\n",progress);
        }
        else
        {
            // 获取进度失败
            xbasic::debug_output("get progress failed ret=%d.\n",ret);
            break;
        }

        if(progress >= 1.0) {
            xbasic::debug_output("ota upgrade is completed.\n");
            break;
        }
    }
    usb_ota_complete_upgrade(ota_type,dst_addr_id);

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void read_real_data(int loop = 0)
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    while(true) {
        sleep(5);
        static int count = 1;

        xbasic::debug_output("read real data time=%d.\n",count);
        struct real_data* data = NULL;
        data=usb_fetch_real_data(dst_addr_id);
        print_real_data(data);
        usb_free_real_data(data);
        data=NULL;

        if((loop > 0) && (loop == count))
        {
            break;
        }

        ++count;
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void read_cal_file()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    int ret = cal_read_file(OTA_TYPE_CAL_FILE,"0.0.0.1",dst_addr_id);
    if(ret != 0) {
        xbasic::debug_output("cal_read_file() failed,ret=%d.\n",ret);
        goto END;
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void read_asci_junction_temp()
{
    xbasic::debug_output("usb lib initialization\n");

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    int    dst_addr_id      = sys_config->get_data("usb_device_addr_id",DST_ADDR_ID);

    usb_init(svr_addr.c_str());

    struct asic_juncttemp  temp[96];
    int size = usb_get_asicjunct_temp(&temp[0],96);
    if(size <= 0) {
        xbasic::debug_output("read_asci_junction_temp() failed,size=%d.\n",size);
        goto END;
    }
    print_asic_junction_temp(&temp[0],size);

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void write_asic_reg(int asic_csid,unsigned short regaddr,unsigned char value)
{
    xbasic::debug_output("enter into write_asic_reg()\n");

    int shmid = attach_shm("/home/gb/USB-Device-Mgr/USB-Device-Mgr/bin");
    asic_msg* ptr = get_asic_msg();
    init_asic_msg(ptr);
    ptr->wr_req.asic_csid =  asic_csid;
    ptr->wr_req.cmd = ASIC_WRITE;
    ptr->wr_req.regaddr = regaddr;
    ptr->wr_req.value = value;
    ptr->wr_req.valid = DATA_VALID;

    while(ptr->wr_resp.valid != DATA_VALID)
    {
        sleep(5);
    }

    print_asic_msg(ptr);
    sleep(5);
    ptr->wr_req.valid = DATA_INVALID;
    //detach_shm(shmid);
    ///ptr = NULL;

    xbasic::debug_output("exited write_asic_reg()\n");
    return ;
}

static void read_asic_reg(int asic_csid,unsigned short regaddr)
{
    xbasic::debug_output("enter into read_asic_reg()\n");

    int shmid = attach_shm("/home/gb/USB-Device-Mgr/USB-Device-Mgr/bin");
    asic_msg* ptr = get_asic_msg();
    init_asic_msg(ptr);
    ptr->rd_req.asic_csid =  asic_csid;
    ptr->rd_req.cmd = ASIC_READ;
    ptr->rd_req.regaddr = regaddr;
    ptr->rd_req.valid = DATA_VALID;

    while(ptr->rd_resp.valid != DATA_VALID)
    {
        sleep(5);
    }

    print_asic_msg(ptr);
    sleep(5);
    ptr->rd_req.valid = DATA_INVALID;
    //detach_shm(shmid);
    //ptr = NULL;

    xbasic::debug_output("exited read_asic_reg()\n");
    return ;
}

static void loop_otaupgrade(int count,const char*  ota_type, const char* version)
{
    printf("begin loop_otaupgrade() ota upgrade loop run 500 times\n");
    for(int i= 0 ; i < count; i++)
    {
        ota_upgrade(ota_type,version);
        sleep(5);
    }
    printf("End loop_otaupgrade()....\n");
}

static void debug_cmd_exe()
{
    while(1)
    {
        printf("$usb-dev-mgr[debug:%d]>",xconfig::debug());
        fflush(stdin);
        char cmd_buff[512] = {0};
        while(!fgets(cmd_buff,sizeof(cmd_buff)-4,stdin)) {ssleep(3);} //获得终端输入
        std::string input_str(cmd_buff);
        xbasic::trim(input_str); //去掉前后的空白字符
        transform(input_str.begin(),input_str.end(),input_str.begin(),::tolower); //全部转小写
        std::vector< std::string> vct_param;
        xbasic::split_string(input_str,std::string(" "),&vct_param);
        if(input_str.length()==0)  continue; //敲入了空
        if(vct_param.size() >0 && vct_param[0].length() >0)
        {
            if(vct_param[0] == "?" || vct_param[0] == "help") //帮助
            {
                usage();
            }
            else if(vct_param[0] == "otaupgrade")
            {
                if(vct_param.size() == 3)
                {
                    ota_upgrade(vct_param[1].c_str(),vct_param[2].c_str());
                }
            }
            else if(vct_param[0] == "read_real")
            {
                if(vct_param.size() == 1)
                {
                    //一直读取实时数据
                    read_real_data();
                }
                else
                {
                    //限制次数读取
                    int time = std::stoi(vct_param[1]);
                    read_real_data(time);
                }
            }
            else if(vct_param[0] == "read_cal")
            {
                read_cal_file();
            }
            else if(vct_param[0] == "read_junction")
            {
                read_asci_junction_temp();
            }
            else if(vct_param[0] == "quit") //退出程序
            {
                //exit(0);
                break;
            }
            else
            {
                printf("'%s' --unkonw command.input '?' for help.\n",vct_param[0].c_str());
            }
        }
        ussleep(10*1000);
    }
}

static void test_ftdig(int boardtype, int slot, int index)
{
    xbasic::debug_output("test_ftdig start boardtype:0x%x, slot:0x%x, index:0x%x\n", boardtype, slot, index);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetBoardExist() ##################################################\n");
    int val = 0;
    int ret = DIG_GetBoardExist(static_cast<board_type>(boardtype), slot, &val);
    if(ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardExist failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardExist=%d.\n",val);

    xbasic::debug_output("################################################## DIG_GetPgbSlot() ##################################################\n");
    int val1 = 0;
    ret = DIG_GetPgbSlot(&val1);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetPgbSlot failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetPgbSlot=%d.\n",val1);

    xbasic::debug_output("################################################## DIG_GetSyncSlot() ##################################################\n");
    int val2 = 0;
    ret = DIG_GetSyncSlot(&val2);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetSyncSlot failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetSyncSlot=%d.\n",val2);

    xbasic::debug_output("################################################## DIG_GetAsicSlot() ##################################################\n");
    int val3[4] = {0};
    ret = DIG_GetAsicSlot(val3, 4);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetAsicSlot failed,ret=%d.\n",ret);
    }
    for(int i = 0; i < 4; i++) {
        xbasic::debug_output("test_ftdig() DIG_GetAsicSlot=0x%x.\n",val3[i]);
    }

    xbasic::debug_output("################################################## DIG_GetPpsSlot() ##################################################\n");
    int val4[4] = {0};
    ret = DIG_GetPpsSlot(val4, 4);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetPpsSlot failed,ret=%d.\n",ret);
    }
    for(int j = 0; j < 4; j++) {
        xbasic::debug_output("test_ftdig() DIG_GetPpsSlot=0x%x.\n",val4[j]);
    }

    xbasic::debug_output("################################################## DIG_GetBoardID() ##################################################\n");
    char value[100] = {0};
    size_t value_size = sizeof(value) / sizeof(value[0]);
    size_t length = 0;
    ret = DIG_GetBoardID(static_cast<board_type>(boardtype), slot, value, value_size, &length);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardID failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardID: %s.\n", value);

    xbasic::debug_output("################################################## DIG_GetBoardSN() ##################################################\n");
    char value1[100] = {0};
    size_t value1_size = sizeof(value1) / sizeof(value1[0]);
    size_t length1 = 0;
    ret = DIG_GetBoardSN(static_cast<board_type>(boardtype), slot, value1, value1_size, &length1);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardSN failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardSN: %s.\n", value1);

    xbasic::debug_output("################################################## DIG_GetBoardHwVersion() ##################################################\n");
    char value2[100] = {0};
    size_t value2_size = sizeof(value2) / sizeof(value2[0]);
    size_t length2 = 0;
    ret = DIG_GetBoardHwVersion(static_cast<board_type>(boardtype), slot, value2, value2_size, &length2);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardHwVersion failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardHwVersion: %s.\n", value2);

    xbasic::debug_output("################################################## DIG_GetBoardSoftVersion() ##################################################\n");
    char value3[100] = {0};
    size_t value3_size = sizeof(value3) / sizeof(value3[0]);
    size_t length3 = 0;
    ret = DIG_GetBoardSoftVersion(static_cast<board_type>(boardtype), slot, value3, value3_size, &length3);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardSoftVersion failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardSoftVersion: %s.\n", value3);

    xbasic::debug_output("################################################## DIG_GetBoardTemp() ##################################################\n");
    double value4=0.0;
    ret = DIG_GetBoardTemp(static_cast<board_type>(boardtype), slot, index, &value4);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardTemp failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardTemp=%f.\n",value4);

    xbasic::debug_output("################################################## DIG_GetBoardVol() ##################################################\n");
    double value5=0.0;
    ret = DIG_GetBoardVol(static_cast<board_type>(boardtype), slot, index, &value5);
    if (ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardVol failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardVol=%f.\n",value5);

    xbasic::debug_output("################################################## DIG_GetBoardMutiVol() ##################################################\n");
    struct multiple_volatage mul_vol;
    ret = DIG_GetBoardMutiVol(static_cast<board_type>(boardtype), slot, 1, 100, &mul_vol);
    if(ret != 0) {
        xbasic::debug_output("test_ftdig() DIG_GetBoardMutiVol failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_ftdig() DIG_GetBoardMutiVol find_size=%d.\n",mul_vol.sz);
    for(int i = 0; i < mul_vol.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, volt=%f\n", i, mul_vol.arry[i].m_id, mul_vol.arry[i].m_volatage_value);
    }

    xbasic::debug_output("################################################## DIG_GetClockChipState(9528) ##################################################\n");
    clock_status c_status1;
    ret = DIG_GetClockChipState(static_cast<board_type>(boardtype), slot, CLOCK_9528, &c_status1);
    xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9528) get clock size: %d.\n",c_status1.sz);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9528) failed,ret=%d.\n",ret);
    }
    else
    {
        for(int i = 0; i < c_status1.sz; i++)
        {
            xbasic::debug_output("[%d]: c_status=%d\n", i, c_status1.arry[i]);
        }
    }

    xbasic::debug_output("################################################## DIG_GetClockChipState(9545) ##################################################\n");
    clock_status c_status2;
    ret = DIG_GetClockChipState(static_cast<board_type>(boardtype), slot, CLOCK_9545, &c_status2);
    xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9545) get clock size: %d.\n",c_status2.sz);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9545) failed,ret=%d.\n",ret);
    }
    else
    {
        for(int i = 0; i < c_status2.sz; i++)
        {
            xbasic::debug_output("[%d]: c_status=%d\n", i, c_status2.arry[i]);
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_cpdig(int boardtype, int slot, int index)
{
    xbasic::debug_output("test_cpdig start boardtype:0x%x, slot:0x%x, index:0x%x\n", boardtype, slot, index);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetBoardExist() ##################################################\n");
    int val = 0;
    int ret = DIG_GetBoardExist(static_cast<board_type>(boardtype), slot, &val);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardExist failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardExist=%d.\n",val);

    xbasic::debug_output("################################################## DIG_GetSyncSlot() ##################################################\n");
    int val2 = 0;
    ret = DIG_GetSyncSlot(&val2);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetSyncSlot failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetSyncSlot=%d.\n",val2);

    xbasic::debug_output("################################################## DIG_GetAsicSlot() ##################################################\n");
    int val3[4] = {0};
    ret = DIG_GetAsicSlot(val3, 4);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetAsicSlot failed,ret=%d.\n",ret);
    }
    for(int i = 0; i < 4; i++) {
        xbasic::debug_output("test_cpdig() DIG_GetAsicSlot=0x%x.\n",val3[i]);
    }

    xbasic::debug_output("################################################## DIG_GetBoardID() ##################################################\n");
    char value[100] = {0};
    size_t value_size = sizeof(value) / sizeof(value[0]);
    size_t length = 0;
    ret = DIG_GetBoardID(static_cast<board_type>(boardtype), slot, value, value_size, &length);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardID failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardID: %s.\n", value);

    xbasic::debug_output("################################################## DIG_GetBoardSN() ##################################################\n");
    char value1[100] = {0};
    size_t value1_size = sizeof(value1) / sizeof(value1[0]);
    size_t length1 = 0;
    ret = DIG_GetBoardSN(static_cast<board_type>(boardtype), slot, value1, value1_size, &length1);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardSN failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardSN: %s.\n", value1);

    xbasic::debug_output("################################################## DIG_GetBoardHwVersion() ##################################################\n");
    char value2[100] = {0};
    size_t value2_size = sizeof(value2) / sizeof(value2[0]);
    size_t length2 = 0;
    ret = DIG_GetBoardHwVersion(static_cast<board_type>(boardtype), slot, value2, value2_size, &length2);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardHwVersion failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardHwVersion: %s.\n", value2);

    xbasic::debug_output("################################################## DIG_GetBoardSoftVersion() ##################################################\n");
    char value3[100] = {0};
    size_t value3_size = sizeof(value3) / sizeof(value3[0]);
    size_t length3 = 0;
    ret = DIG_GetBoardSoftVersion(static_cast<board_type>(boardtype), slot, value3, value3_size, &length3);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardSoftVersion failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardSoftVersion: %s.\n", value3);

    xbasic::debug_output("################################################## DIG_GetBoardTemp() ##################################################\n");
    double value4=0.0;
    ret = DIG_GetBoardTemp(static_cast<board_type>(boardtype), slot, index, &value4);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardTemp failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardTemp=%f.\n",value4);

    xbasic::debug_output("################################################## DIG_GetBoardMutiTemp() ##################################################\n");
    struct multiple_temp mul_tmp;
    ret = DIG_GetBoardMutiTemp(static_cast<board_type>(boardtype), slot, 1, 4095, &mul_tmp);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardMutiTemp failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardMutiTemp find_size=%d.\n",mul_tmp.sz);
    for(int i = 0; i < mul_tmp.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, temp=%f\n", i, mul_tmp.arry[i].m_id, mul_tmp.arry[i].m_temp_value);
    }

    xbasic::debug_output("################################################## DIG_GetBoardVol() ##################################################\n");
    double value5=0.0;
    ret = DIG_GetBoardVol(static_cast<board_type>(boardtype), slot, index, &value5);
    if (ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardVol failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardVol=%f.\n",value5);

    xbasic::debug_output("################################################## DIG_GetBoardMutiVol() ##################################################\n");
    struct multiple_volatage mul_vol;
    ret = DIG_GetBoardMutiVol(static_cast<board_type>(boardtype), slot, 1, 100, &mul_vol);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardMutiVol failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardMutiVol find_size=%d.\n",mul_vol.sz);
    for(int i = 0; i < mul_vol.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, volt=%f\n", i, mul_vol.arry[i].m_id, mul_vol.arry[i].m_volatage_value);
    }

    xbasic::debug_output("################################################## DIG_GetBoardInfo() ##################################################\n");
    struct board_basicinfo arr[100];
    memset(arr, 0, sizeof(arr));
    int arr_num = sizeof(arr) / sizeof(arr[0]);
    int real_size = 0;
    ret = DIG_GetBoardInfo(static_cast<board_type>(boardtype), arr, arr_num, &real_size);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetBoardInfo failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_cpdig() DIG_GetBoardInfo find_size=%d.\n",real_size);
    for(int i = 0; i < real_size; i++)
        xbasic::debug_output("board_type:%d\n", i);
        xbasic::debug_output("slot:%d\n", arr[i].slot);
        xbasic::debug_output("hw_ver:%s\n", arr[i].hw_ver);
        xbasic::debug_output("soft_ver:%s\n", arr[i].soft_ver);
    }

    xbasic::debug_output("################################################## DIG_GetClockChipState(9528) ##################################################\n");
    clock_status c_status1;
    ret = DIG_GetClockChipState(static_cast<board_type>(boardtype), slot, CLOCK_9528, &c_status1);
    xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9528) get clock size: %d.\n",c_status1.sz);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9528) failed,ret=%d.\n",ret);
    }
    else
    {
        //xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9528) get clock size: %d.\n",c_status1.sz);
        for(int i = 0; i < c_status1.sz; i++)
        {
            xbasic::debug_output("[%d]: c_status=%d\n", i, c_status1.arry[i]);
        }
    }

    xbasic::debug_output("################################################## DIG_GetClockChipState(9545) ##################################################\n");
    clock_status c_status2;
    ret = DIG_GetClockChipState(static_cast<board_type>(boardtype), slot, CLOCK_9545, &c_status2);
    xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9545) get clock size: %d.\n",c_status2.sz);
    if(ret != 0) {
        xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9545) failed,ret=%d.\n",ret);
    }
    else
    {
        //xbasic::debug_output("test_cpdig() DIG_GetClockChipState(9545) get clock size: %d.\n",c_status2.sz);
        for(int i = 0; i < c_status2.sz; i++)
        {
            xbasic::debug_output("[%d]: c_status=%d\n", i, c_status2.arry[i]);
        }
    }

END:
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_poweroff(int boardtype, int slot, int sw_val, int utp48id)
{
    xbasic::debug_output("test_poweroff start boardtype:0x%x, slot:0x%x, sw_val:%d\n", boardtype, slot, sw_val);
    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    usb_init(svr_addr.c_str());

    std::string poweroff_type = std::string(argv[2]);
    xbasic::trim(poweroff_type);

    if(poweroff_type == std::string("board"))
    {
        DIG_BoardPowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else if(poweroff_type == std::string("fpga"))
    {
        DIG_FPGAPowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else if(poweroff_type == std::string("asic"))
    {
        DIG_ASICPowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else if(poweroff_type == std::string("utp48"))
    {
        DIG_UTP48PowerOff(static_cast<board_type>(boardtype), slot, utp48id, sw_val);
    }
    else if(poweroff_type == std::string("fpga200t"))
    {
        DIG_FPGA200T_PowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else if(poweroff_type == std::string("utp40master"))
    {
        DIG_UTP40_MasterPowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else if(poweroff_type == std::string("k4u60"))
    {
        DIG_FPGA4KU60_PowerOff(static_cast<board_type>(boardtype), slot, sw_val);
    }
    else
    {
        xbasic::debug_output("test_poweroff() poweroff_type:%s is not support \n",poweroff_type.c_str());
    }

END:
    usb_uninit();
    xbasic::debug_output("test_poweroff() usb lib uninitialization\n");
    return ;
}

static void test_temp(int boardtype, int slot)
{
    xbasic::debug_output("test_temp start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetBoardMutiTemp() ##################################################\n");
    struct multiple_temp mul_tmp;
    int ret = DIG_GetBoardMutiTemp(static_cast<board_type>(boardtype), slot, 1, 4094, &mul_tmp);
    if(ret != 0) {
        xbasic::debug_output("test_temp() DIG_GetBoardMutiTemp failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_temp() DIG_GetBoardMutiTemp find_size=%d.\n",mul_tmp.sz);
    for(int i = 0; i < mul_tmp.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, temp=%f\n", i, mul_tmp.arry[i].m_id, mul_tmp.arry[i].m_temp_value);
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_voltage(int boardtype, int slot)
{
    xbasic::debug_output("test_voltage start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetBoardMutiVol() ##################################################\n");
    struct multiple_volatage mul_vol;
    int ret = DIG_GetBoardMutiVol(static_cast<board_type>(boardtype), slot, 1, 4094, &mul_vol);
    if(ret != 0) {
        xbasic::debug_output("test_voltage() DIG_GetBoardMutiVol failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_voltage() DIG_GetBoardMutiVol find_size=%d.\n",mul_vol.sz);
    for(int i = 0; i < mul_vol.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, voltage=%f\n", i, mul_vol.arry[i].m_id, mul_vol.arry[i].m_volatage_value);
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_pwm(int boardtype, int slot)
{
    xbasic::debug_output("test_pwm start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetPWMFrequency() ##################################################\n");
    struct pwm_val p_val;
    int ret = DIG_GetPWMFrequency(static_cast<board_type>(boardtype), slot, &p_val);
    if(ret != 0) {
        xbasic::debug_output("test_pwm() DIG_GetPWMFrequency failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_pwm() DIG_GetPWMFrequency find_size=%d.\n",p_val.sz);
    for(int i = 0; i < p_val.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, frequency=%d\n", i, p_val.arry[i].m_id, p_val.arry[i].m_frequency);
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_serdes(int boardtype, int slot)
{
    xbasic::debug_output("test_serdes start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetSerdesState() ##################################################\n");
    struct serdes_val s_val;
    int ret = DIG_GetSerdesState(static_cast<board_type>(boardtype), slot, &s_val);
    if(ret != 0) {
        xbasic::debug_output("test_serdes() DIG_GetSerdesState failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_serdes() DIG_GetSerdesState find_size=%d.\n",s_val.sz);
    for(int i = 0; i < s_val.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, state=%d\n", i, s_val.arry[i].m_id, s_val.arry[i].m_state);
    }

    xbasic::debug_output("################################################## DIG_GetMutiSerdesState() ##################################################\n");
    struct serdes_val s_val1;
    ret = DIG_GetMutiSerdesState(static_cast<board_type>(boardtype), slot, 1, 48, &s_val1);
    if(ret != 0) {
        xbasic::debug_output("test_serdes() DIG_GetMutiSerdesState failed,ret=%d.\n",ret);
    }
    xbasic::debug_output("test_serdes() DIG_GetMutiSerdesState find_size=%d.\n",s_val1.sz);
    for(int i = 0; i < s_val1.sz; i++)
    {
        xbasic::debug_output("[%d]: id=%d, state=%d\n", i, s_val1.arry[i].m_id, s_val1.arry[i].m_state);
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_utp_register(int boardtype, int slot)
{
    xbasic::debug_output("test_utp_register start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetUtpRegister() ##################################################\n");
    struct utp_register_val val;
    int ret = DIG_GetUtpRegister(static_cast<board_type>(boardtype), slot, 1, &val);
    if(ret != 0) {
        xbasic::debug_output("test_utp_register() DIG_GetUtpRegister failed,ret=%d.\n",ret);
    }
    else
    {
        for(int i = 0; i < MAX_UTP_REGISTER_NUM; ++i)
        {
            xbasic::debug_output("test_utp_register() DIG_GetUtpRegister utp_chipid:%d valid:%d reg_ch:%d reg_addr:0x%x reg_data:0x%x\n",val.chip_id, val.register_val[i].valid, val.register_val[i].reg_ch, val.register_val[i].reg_addr, val.register_val[i].reg_data);
        }
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_utp_register_m(int boardtype, int slot)
{
    xbasic::debug_output("test_utp_register_m() start boardtype:0x%x, slot:0x%x\n", boardtype, slot);

    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetMutiUtpRegister() ##################################################\n");
    int one_sz = sizeof(struct utp_register_val);
    int sz = 148;
    xbasic::debug_output("test_utp_register_m() one_sz:%d sz:%d\n",one_sz, sz);
    int begin_id = 0; int begin_id1 = 148; begin_id += 20;

    struct utp_register_val *arry = (struct utp_register_val*)malloc(148*sizeof(struct utp_register_val));
    memset(arry, 0, sizeof(arry[0]) * 148);

    int ret = DIG_GetMutiUtpRegister(static_cast<board_type>(boardtype), slot, begin_id, begin_id19, arry, sizeof(arry)/sizeof(arry[0]));
    if(ret != 0) {
        xbasic::debug_output("test_utp_register_m() DIG_GetMutiUtpRegister failed,ret=%d.\n",ret);
    }
    else
    {
        // 外层循环遍历每一个芯片
        for(int i = 0; i < MAX_UTP_REGISTER_NUM; ++i)
        {
            // 内层循环遍历该芯片上的寄存器
            xbasic::debug_output("test_utp_register_m() DIG_GetMutiUtpRegister utp_chipid:%d valid:%d reg_ch:%d reg_addr:0x%x reg_data:0x%x\n",arry[i].chip_id, arry[i].register_val[i].valid, arry[i].register_val[i].reg_ch, arry[i].register_val[i].reg_addr, arry[i].register_val[i].reg_data);
        }
    }

    // free(arry);
    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_utp102_register(int boardtype, int slot)
{
    xconfig    *sys_config = xconfig::get_instance();
    std::string svr_addr    = sys_config->get_data("adapter_server");
    xbasic::debug_output("##################################################  usb_init()  ##################################################\n");
    usb_init(svr_addr.c_str());

    xbasic::debug_output("################################################## DIG_GetUsp102Register() ##################################################\n");
    struct utp102_register_val val;
    int ret = DIG_GetUsp102Register(static_cast<board_type>(boardtype), slot, 1, &val);
    if(ret != 0) {
        xbasic::debug_output("test_utp102_register() DIG_GetUsp102Register failed,ret=%d.\n",ret);
    }
    else
    {
        for(int i = 0; i < 148; ++i)
        {
            xbasic::debug_output("test_utp102_register() DIG_GetUsp102Register utp_chipid:%d valid:%d reg_ch:%d reg_addr:0x%x reg_data:0x%x\n",val.chip_id, val.register_val[i].valid, val.register_val[i].reg_ch, val.register_val[i].reg_addr, val.register_val[i].reg_data);
        }
    }

    usb_uninit();
    xbasic::debug_output("usb lib uninitialization\n");
    return ;
}

static void test_utp102_register_m(int boardtype, int slot)
{
    xbasic::debug_output("test_utp102_register_m() start boardtype:0x%x, slot:0x%x\n", boardtype, slot
