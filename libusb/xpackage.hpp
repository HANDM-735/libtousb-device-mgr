#ifndef X_PACKAGE_H
#define X_PACKAGE_H

#include "xbasic.hpp"
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include "libjson/jsonobject.h"
#include "xconfig.hpp"

#define INVA_DATA    (int)0xAAAAAAAA    // 无效数据
#define INVA_SDATA   "<NULL>"           // 无效数据
#define PROTO_VERSION 0                 // 协议版本

// 数据定义类
class xdatadef
{
public:
    typedef struct _DATAPRO
    {
        int     cmd;        // 命令字
        int     tid;        // 类型ID
        char    name[32];   // 数据名称
        char    ename[32];  // 数据英文名称
        char    dtype;      // 数据类型(P:固有属性数据/C:配置数据/R:实时数据)
        char    vtype;      // 数值类型(S:字符串/I:整型/B:字节流/J:JSON)
    }DATAPRO,*PDATAPRO;

public:
    static DATAPRO *get_datapro(int tid,int cmd =0x01)
    {
        static boost::mutex s_mux_datapro;  // 锁
        static boost::unordered_map<int,DATAPRO *> s_map_datapro;
        boost::mutex::scoped_lock lock(s_mux_datapro);

        if(s_map_datapro.size()==0)
        {
            std::string datadef_file = (std::string)(xbasic::get_module_path())+"datadef.dat";
            FILE *file_fd = fopen(datadef_file.c_str(),"r");
            if(!file_fd) return NULL;

            char read_buff[1024] = {0};
            while(fgets(read_buff,sizeof(read_buff)-1,file_fd))
            {
                std::string line_str(read_buff);
                xbasic::trim(line_str); // 去掉前后的空白字符
                std::vector<std::string> vct_cell;
                int cell_num = xbasic::split_string(line_str,",",&vct_cell);
                if(cell_num >=6)
                {
                    DATAPRO *datapro = new DATAPRO;
                    memset(datapro,0,sizeof(DATAPRO));

                    datapro->cmd = (int)strtol(vct_cell[0].c_str(),NULL,0);
                    datapro->tid = (int)strtol(vct_cell[1].c_str(),NULL,0);
                    memcpy(datapro->name,vct_cell[2].c_str(),vct_cell[2].length()>sizeof(datapro->name)-1?vct_cell[2].length():sizeof(datapro->name)-1);
                    memcpy(datapro->ename,vct_cell[3].c_str(),vct_cell[3].length()>sizeof(datapro->ename)-1?vct_cell[3].length():sizeof(datapro->ename)-1);
                    datapro->dtype = vct_cell[4][0];
                    datapro->vtype = vct_cell[5][0];

                    s_map_datapro.insert(std::make_pair((int)((datapro->cmd<<16)|datapro->tid),datapro));
                }
            }
        }

        int find_cmd = cmd;
        if(find_cmd ==0x02 || find_cmd ==0x05) find_cmd =0x01; //write和alarm报文与read报文共用TLV定义
        boost::unordered_map<int,DATAPRO *>::iterator iter = s_map_datapro.find((int)((find_cmd<<16)|tid));
        if(iter == s_map_datapro.end()) return NULL;
        return iter->second;
    }
};

// TLV数据单元类
class xtvl
{
public:
    xtvl(unsigned char tid,unsigned short length,const void *value,unsigned char cmd =0x01)
    {
        m_data_pro = xdatadef::get_datapro(tid,cmd);
        set_data(tid,length,value);
    }

    ~xtvl() {}

    xtvl *clone() {return new xtvl(m_tid,m_value.length(),m_value.data());} //克隆一个新对象

    void set_cmd(unsigned char cmd) //设置命令字
    {
        char old_vtype = (m_data_pro?m_data_pro->vtype:0);
        m_data_pro = xdatadef::get_datapro(m_tid,cmd);
        if(old_vtype!=m_data_pro->vtype) set_data(m_tid,m_value.length(),m_value.data()); //值类型有改变，重新解析
    }

    void set_data(unsigned char tid,unsigned short length,const void *value) //设置数据
    {
        if(m_tid !=tid)
        {
            m_tid = tid;
            if(m_data_pro) m_data_pro = xdatadef::get_datapro(m_tid,m_data_pro->cmd);
        }

        std::string new_value;
        if(value !=NULL) new_value = (length>0)?std::string((char *)value,length):std::string((char *)value);
        if(new_value ==m_value) return; //值相同

        m_value = new_value;
        if(m_data_pro==NULL || m_data_pro->vtype=='S') update_data(new_value); //按数组类型存储
    }

    std::string get_data() {return m_value;}
    int get_data_to_int() {return atoi(m_value.c_str());} //将字符串格式化为整型

    int update_data(std::string new_value) //将新单元插入到数组中(相同索引的替换)
    {
        int set_num =0;
        std::vector<std::string> vct_cell_new;
        std::string::size_type brackets_s = new_value.find("[");
        std::string::size_type brackets_e = new_value.find("]");

        if(brackets_s!=std::string::npos && brackets_e!=std::string::npos && brackets_s<brackets_e) //是连续数组结构
        {
            std::vector<std::string> vct_cell_tmp,vct_head;
            if(xbasic::split_string(new_value.substr(brackets_s+1,brackets_e-brackets_s-1),",",&vct_cell_tmp)<2) return 0; //连续数组字符串格式:"a[n:x1;x2;...xn"
            if(xbasic::split_string(vct_cell_tmp[0],"[",&vct_head)<2) return 0;

            int count_value = xbasic::split_string(vct_cell_tmp[1],";",&vct_cell_new); //项值,多个以";"分隔
            int start_addr = atoi(vct_head[0].c_str()),addr_num = atoi(vct_head[1].c_str()); //起始地址和数据个数

            for(int i=0;i<addr_num && i<count_value;i++)
            {
                set_cell(boost::str(boost::format("%d")%(start_addr+i)),vct_cell_new[i]);
                set_num++;
            }
        }
        else //是离散数组结构
        {
            int count_cell_new = xbasic::split_string(new_value,",",&vct_cell_new); //数组中字符串以逗号分隔
            for(int i=0;i<count_cell_new;i++)
            {
                std::vector<std::string> vct_value;
                int count_value = xbasic::split_string(vct_cell_new[i],":",&vct_value); //单元中项和值以冒号分隔
                if(count_value <=0) continue;

                if(count_value <=1)
                    set_cell("",vct_value[0]);
                else
                    set_cell(vct_value[0],vct_value[1]);
                set_num++;
            }
        }
        return set_num;
    }

    std::string get_cell_value(std::string cell_name) //在数组中获取指定单元的数据值
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mux_arr); //读锁
        for(unsigned int i=0;i<m_arr_value.size();i++)
        {
            std::vector<std::string> vct_value;
            int count_value = xbasic::split_string(m_arr_value[i],":",&vct_value); //单元中项和值以冒号分隔
            if(count_value<=0 || vct_value[0]!=cell_name) continue;

            return (count_value>1?vct_value[1]:"");
        }
        return INVA_SDATA;
    }

    int get_all_cells(std::vector<std::string> &arr_value) //获得整个数组的值到容器
    {
        arr_value.clear();
        boost::shared_lock<boost::shared_mutex> lock(m_mux_arr); //读锁
        for(unsigned int i=0;i<m_arr_value.size();i++) arr_value.push_back(m_arr_value[i]);
        return arr_value.size();
    }

    void set_cell(std::string cell_name,std::string cell_value) //在数组中设置指定单元的数据值
    {
        int ifind =-1;
        std::string cell = (cell_name.length()<=0?(cell_value):(cell_name+":"+cell_value));

        boost::unique_lock<boost::shared_mutex> lock(m_mux_arr); //写锁
        for(unsigned int i=0;i<m_arr_value.size() && ifind==-1;i++)
        {
            std::vector<std::string> vct_value;
            int count_value = xbasic::split_string(m_arr_value[i],":",&vct_value); //单元中项和值以冒号分隔
            if(count_value <=1) continue;

            if(cell_name.length()==0)
                ifind = (int)i; //是需要的数据项，待更新
            else if(vct_value[0]==cell_name) //是需要的数据项
            {
                if(vct_value[1]==cell_value) return; //值相同
                else ifind = (int)i; //值不同，待更新
            }
        }

        if(ifind >=0)
            m_arr_value[ifind] = cell; //更新该值
        else
            m_arr_value.push_back(cell); //不存在，新增

        m_value = boost::algorithm::join(m_arr_value,","); //修改总字符串的值
    }

public:
    // 将串行TLV数据格式化为TLV对象数组
    static int parse_tlv(unsigned char *tlv_data,int data_len,std::list<boost::shared_ptr<xtvl>> &list_tlv)
    {
        int iread=0;
        while(iread<data_len)
        {
            unsigned char tid = tlv_data[iread];
            unsigned short length = xbasic::read_little_endian(&tlv_data[iread+1],2);

            if(iread+3+length >data_len) return list_tlv.size(); //数据长度错误
            boost::shared_ptr<xtvl> tlv(new xtvl(tid,length,&tlv_data[iread+3]));
            list_tlv.push_back(tlv);
            iread +=(3+length);
        }
        return list_tlv.size();
    }

    // 将JSON数据格式化为TLV对象数组
    static int parse_from_json(cjson_object &tlv_json,std::list<boost::shared_ptr<xtvl>> &list_tlv)
    {
        int data_size = (tlv_json.is_empty())?0:tlv_json.get_array_size();
        if(data_size <=0) return true;

        for(int i=0;i<data_size;i++)
        {
            cjson_object &js_item = tlv_json[i];
            std::string tid_str = js_item["tid"];
            std::string key_str = js_item["key"];
            std::string value_str = js_item["value"];

            unsigned char tid = (unsigned char)atoi(tid_str.c_str());
            std::string value = (key_str.length()<=0?value_str:(key_str+":"+value_str));

            boost::shared_ptr<xtvl> tlv_find;
            for(boost::shared_ptr<xtvl> &iter:list_tlv)
            {
                if(iter->m_tid ==tid) {tlv_find = iter;break;} //寻找对应的tlv数据
            }

            if(tlv_find !=NULL) {tlv_find->update_data(value);continue;} //没有找到就新增
            boost::shared_ptr<xtvl> new_data(new xtvl(tid,value.length(),value.data()));
            list_tlv.push_back(new_data);
        }
        return list_tlv.size();
    }

    // 将TLV对象数组串行化为TLV数据流格式
    static std::string serial(std::list<boost::shared_ptr<xtvl>> &list_tlv)
    {
        int iwrite=0;
        unsigned char tlv_buff[8192] = {0};

        for(boost::shared_ptr<xtvl> &iter:list_tlv)
        {
            if(iwrite+3+iter->m_value.length() >sizeof(tlv_buff)) break; //缓存空间不足

            tlv_buff[iwrite++] = iter->m_tid;
            xbasic::write_little_endian(&tlv_buff[iwrite],iter->m_value.length(),2);
            iwrite +=2;

            if(iter->m_value.length()>0) memcpy(&tlv_buff[iwrite],iter->m_value.data(),iter->m_value.length());
            iwrite +=iter->m_value.length();
        }
        return std::string((char *)tlv_buff,iwrite);
    }

    // 将TLV对象数组串行化为JSON数据格式
    static cjson_object serial_to_json(std::list<boost::shared_ptr<xtvl>> &list_tlv)
    {
        cjson_object tlv_json("[]");
        for(boost::shared_ptr<xtvl> &iter:list_tlv)
        {
            xdatadef::DATAPRO *data_pro = iter->m_data_pro;
            if(!data_pro || data_pro->vtype=='S') //数组类型
            {
                std::vector<std::string> arr_value;
                int arr_size = iter->get_all_cells(arr_value); //获得整个数组的值

                for(int i=0;i<arr_size;i++)
                {
                    std::vector<std::string> vct_value;
                    if(xbasic::split_string(arr_value[i],":",&vct_value)<=0) continue; //单元中项和值以":"分隔

                    std::string cell_str;
                    if(vct_value.size()<=1)
                        cell_str = boost::str(boost::format("{\"tid\":%1%,\"value\":\"%2%\"}")%(int)(iter->m_tid)%(vct_value[0]));
                    else
                        cell_str = boost::str(boost::format("{\"tid\":%1%,\"key\":\"%2%\",\"value\":\"%3%\"}")%(int)(iter->m_tid)%(vct_value[0])%(vct_value[1]));

                    cjson_object tlv_cell(cell_str);
                    tlv_json.add(tlv_cell);
                }
            }
            else if(data_pro->vtype =='J') //JSON类型
            {
                cjson_object js_value(boost::str(boost::format("{\"tid\":%1%}")%(int)(iter->m_tid)));
                js_value.add("value",iter->m_value);
                tlv_json.add(js_value);
            }
            else if(data_pro->vtype =='B') //二进制类型
            {
                char *new_hexstr = new char[iter->m_value.length()*2+1];
                xbasic::hex_to_str((unsigned char *)iter->m_value.data(),new_hexstr,iter->m_value.length());
                new_hexstr[iter->m_value.length()*2] = '\0';

                cjson_object tlv_cell(boost::str(boost::format("{\"tid\":%1%,\"value\":\"%2%\"}")%(int)(iter->m_tid)%(new_hexstr)));
                tlv_json.add(tlv_cell);
                delete [] new_hexstr;
            }
            else //按字符串类型
            {
                cjson_object tlv_cell(boost::str(boost::format("{\"tid\":%1%,\"value\":\"%2%\"}")%(int)(iter->m_tid)%(iter->m_value)));
                tlv_json.add(tlv_cell);
            }
        }
        return tlv_json.to_string();
    }

    // 比较两个数据单元是否相等
    static bool is_equal(xtvl *tlv1,xtvl *tlv2)
    {
        return (tlv1->m_tid !=tlv2->m_tid && tlv1->m_value !=tlv2->m_value);
    }

public:
    unsigned char                 m_tid;        // 数据项
    std::string                   m_value;      // 数据值
    xdatadef::DATAPRO           *m_data_pro;    // 数据属性定义

protected:
    std::vector<std::string>     m_arr_value;   // 数组类型的数据值
    boost::shared_mutex          m_mux_arr;     // 数组读写锁
};

// 报文基类
class xpacket
{
public:
    xpacket() {}
    virtual xpacket *clone() {return new xpacket();} //克隆对象

public:
    virtual void reset() {}                                 //重置包
    virtual bool is_empty() {return false;}                 //是否是空包
    virtual bool need_confirm() {return false;}            //需要确认并重试的包
    virtual bool type_confirm() {return false;}             //是确认包
    virtual int flag_confirm() {return 0;}                  //获取确认报文的确认标志
    virtual bool is_confirm(xpacket *pack) {return (flag_confirm()==0? false : (flag_confirm()==pack->flag_confirm()));} //是否是确认的该包

    virtual bool parse_from_bin(unsigned char *pack_data,int pack_len) {return true;}  //从BIN解析数据包
    virtual bool parse_from_json(std::string &json_data) {return true;}              //从JSON解析数据包
    virtual std::string serial_to_bin() {return std::string("");}                   //串行化成BIN
    virtual std::string serial_to_json() {return std::string("");}                   //串行化成JSON
};

#endif