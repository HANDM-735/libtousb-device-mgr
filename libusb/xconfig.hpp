#ifndef XCONFIG_H
#define XCONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "xbasic.hpp"
#include "libjson/cjsonobject.h"

#define APP_VERSION  "1.2.0.1"    // 程序版本号

// 内存全局配置单例类
class xconfig
{
public:
    xconfig() {}

    // C++11 线程安全单例获取
    static xconfig *get_instance()
    {
        static xconfig s_instance;
        return &s_instance;
    }

    static int debug()       { return xconfig::get_instance()->get_data("debug", 0); }        // 获取日志调试等级
    static int self_id()     { return xconfig::get_instance()->get_data("self_id", 0); }        // 获取本机节点ID

public:
    // 字符串类型 写入配置
    void set_data(std::string key, std::string value)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mx_map_data);    // 写锁
        m_map_data[key] = value;
    }

    // 字符串类型 读取配置
    std::string get_data(std::string key)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mx_map_data);   // 读锁
        boost::unordered_map<std::string,std::string>::iterator iter = m_map_data.find(key);
        return (iter==m_map_data.end())?"":(iter->second);
    }

    // 整型 写入配置
    void set_data(std::string key, int value)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mx_map_data);    // 写锁
        m_map_data[key] = boost::str(boost::format("%d")%value);
    }

    // 整型 读取配置（带默认值）
    int get_data(std::string key, int def_value)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mx_map_data);   // 读锁
        boost::unordered_map<std::string,std::string>::iterator iter = m_map_data.find(key);
        return (iter==m_map_data.end())?def_value:(int)strtol(iter->second.c_str(),NULL,0);
    }

protected:
    boost::unordered_map<std::string,std::string>  m_map_data;       // 键值配置表
    boost::shared_mutex                             m_mx_map_data;  // 配置表读写互斥锁
};

// INI文件配置解析类
class ini_config
{
public:
    ini_config() {}

    // 绑定INI配置文件路径
    bool set_file(std::string file_path)
    {
        m_file_path = file_path;
        try
        {
            m_tree_ini.clear();
            boost::property_tree::ini_parser::read_ini(m_file_path, m_tree_ini);
            return true;
        }
        catch (...) // 文件读取异常，初始化空配置
        {
            xbasic::debug_output("< cfg > config file: %s read error!\n",m_file_path.c_str());
        }
        return false;
    }

    // 读取字符串配置（带默认值）
    std::string get_data(std::string key, std::string def_value="") // 例：SYSTEM.gateway_id
    {
        try
        {
            std::string value = m_tree_ini.get<std::string>(key);
            return value;
        }
        catch (...) {}
        return def_value;
    }

    // 读取整型配置（带默认值）
    int get_data(std::string key, int def_value=0)
    {
        try
        {
            std::string value = m_tree_ini.get<std::string>(key);
            return atoi(value.c_str());
        }
        catch (...) {}
        return def_value;
    }

    // 写入字符串配置，可选是否立即落盘
    int set_data(std::string key, std::string value, bool save=true)
    {
        try
        {
            m_tree_ini.put<std::string>(key,value);
            if(save) boost::property_tree::ini_parser::write_ini(m_file_path,m_tree_ini);
        }
        catch (...) { return -1; }
        return 0;
    }

    // 写入整型配置，可选是否立即落盘
    int set_data(std::string key, int value, bool save=true)
    {
        try
        {
            m_tree_ini.put<int>(key,value);
            if(save) boost::property_tree::ini_parser::write_ini(m_file_path,m_tree_ini);
        }
        catch (...) { return -1; }
        return 0;
    }

    // 强制保存所有配置到磁盘文件
    int save_data()
    {
        try
        {
            boost::property_tree::ini_parser::write_ini(m_file_path,m_tree_ini);
        }
        catch (...) { return -1; }
        return 0;
    }

protected:
    std::string                          m_file_path;    // 配置文件路径
    boost::property_tree::ptree          m_tree_ini;     // INI树形结构
};

// JSON文件配置解析类
class xjson_config
{
public:
    // 数值类型枚举
    enum VALUE_TYPE  {V_STRING=0x01,V_INT,V_FLOAT};

    xjson_config() {}
    xjson_config(std::string file_path) { set_file(file_path); }

    // 从JSON字符串快速构造配置对象
    static xjson_config from_string(std::string js_string)
    {
        xjson_config js_config;
        js_config.set_data(js_string);
        return js_config;
    }

    // 读取文件，返回原始JSON字符串
    static std::string get_file_json(std::string file_path)
    {
        xjson_config js_config(file_path);
        return (js_config.m_js_config.is_empty())?"":js_config.m_js_config.to_string();
    }

    // 加载JSON配置文件
    bool set_file(std::string file_path)
    {
        if(!m_js_config.is_empty()) m_js_config.clear();
        m_file_path = file_path;

        FILE *file_fd = fopen(m_file_path.c_str(),"r");
        if(!file_fd) return false;

        fseek(file_fd,0L,SEEK_END);         // 定位文件末尾
        int file_len=ftell(file_fd);        // 获取文件总长度
        fseek(file_fd,0L,SEEK_SET);         // 回到文件开头

        char *buff = new char[file_len+4];
        int read_len = fread(buff,1,file_len,file_fd);
        fclose(file_fd);

        std::string json_str(buff,read_len);
        m_js_config.parse(json_str);
        delete[] buff;

        if(m_js_config.is_empty()) return false;
        return true;
    }

    // 保存配置到指定文件
    bool save(std::string file_path)
    {
        if(file_path.length() ==0) file_path = m_file_path;
        std::string js_string = m_js_config.to_formatted_string();
        if(js_string.length()<=0) return false;

        FILE *file_fd = fopen(file_path.c_str(),"w+");
        if(!file_fd) return false;

        fwrite(js_string.c_str(),js_string.length(),1,file_fd);
        fclose(file_fd);
        return true;
    }

    // 直接解析JSON字符串
    bool set_data(std::string js_data)
    {
        return m_js_config.parse(js_data);
    }

    // 字符串类型赋值
    bool set_value(std::string key,std::string value_s)
    {
        return set_value(key,"",value_s,0,V_STRING);
    }

    // 整型类型赋值
    bool set_value(std::string key,int value_i)
    {
        return set_value(key,"",value_i,0,V_INT);
    }

    // 浮点类型赋值
    bool set_value(std::string key,float value_f)
    {
        return set_value(key,"",value_f,0,V_FLOAT);
    }

    // 读取字符串配置
    std::string value(std::string key)
    {
        std::string last_key;
        cjson_object &js_node = value_node(key,last_key);
        if(js_node.is_empty() || last_key.length()==0) return ""; // 父级节点不存在
        return js_node(last_key);
    }

    // 读取整型配置
    int value_int(std::string key)
    {
        std::string value_s = value(key);
        return atoi(value_s.c_str());
    }

    // 读取浮点配置
    float value_float(std::string key)
    {
        std::string value_s = value(key);
        return (float)atof(value_s.c_str());
    }

    // 获取JSON数组长度
    int array_size(std::string key)
    {
        std::string last_key;
        cjson_object &js_node = value_node(key,last_key);
        if(js_node.is_empty() || last_key.length()==0) return -1;

        cjson_object &js_last_node = js_node[last_key];
        if(js_last_node.is_empty() || !js_last_node.is_array()) return -1;
        return js_last_node.get_array_size();
    }

    // 获取数组指定索引节点内容
    std::string index_node(std::string key,int index)
    {
        std::string last_key;
        cjson_object &js_node = value_node(key,last_key);
        if(js_node.is_empty() || last_key.length()==0) return "";

        cjson_object &js_last_node = js_node[last_key];
        if(js_last_node.is_empty() || !js_last_node.is_array()) return "";

        int arr_size = js_last_node.get_array_size();
        if(index >=arr_size) return "";

        cjson_object &js_item = js_last_node[index];
        std::string js_item_str = (js_item.is_empty())?"":js_item.to_string();
        return js_item_str;
    }

    // 读取JSON数组全部值到字符串容器
    int array_value(std::string key,std::vector<std::string> &vct_value)
    {
        vct_value.clear();
        std::string last_key;
        cjson_object &js_node = value_node(key,last_key);
        if(js_node.is_empty() || last_key.length()==0) return -1;

        cjson_object &js_last_node = js_node[last_key];
        if(js_last_node.is_empty() || !js_last_node.is_array()) return -1;

        int arr_size = js_last_node.get_array_size();
        for(int i=0;i<arr_size;i++)
        {
            cjson_object &js_item = js_last_node[i];
            std::string js_item_str = (js_item.is_empty())?"":js_item.to_string();
            if(js_item_str.length()<=0) continue;

            vct_value.push_back(js_item_str);
        }
        return vct_value.size();
    }

protected:
    // 解析层级Key，返回倒数第二层节点
    cjson_object &value_node(std::string key,std::string &last_key)
    {
        std::vector<std::string> vct_key;
        int key_num = xbasic::split_string(key,".",&vct_key);

        if(key_num==1)
        {
            last_key=vct_key[0];
            return m_js_config;
        }

        cjson_object &js_temp = m_js_config[vct_key[0]];
        if(js_temp.is_empty()) return js_temp; // 父级节点不存在

        for(int i=1;i<key_num;i++)
        {
            if(i==key_num-1)
            {
                last_key=vct_key[i];
                return js_temp;
            }
            js_temp = js_temp[vct_key[i]];
            if(js_temp.is_empty()) return js_temp;
        }
        return js_temp;
    }

    // 通用多级JSON节点赋值接口
    bool set_value(std::string key,std::string value_s,int value_i,float value_f,int value_type=V_STRING)
    {
        std::string js_string = m_js_config.to_string();
        cjson_object js_tmp_cfg(js_string);

        std::vector<std::string> vct_key;
        int key_num = xbasic::split_string(key,".",&vct_key);
        do
        {
            if(key_num==1) // 一级节点
            {
                js_tmp_cfg.del(vct_key[0]);
                if(value_type == V_INT)
                    js_tmp_cfg.add(vct_key[0],value_i);
                else if(value_type == V_FLOAT)
                    js_tmp_cfg.add(vct_key[0],value_f);
                else
                    js_tmp_cfg.add(vct_key[0],value_s);
                break;
            }

            cjson_object &js_temp = js_tmp_cfg[vct_key[0]];
            if(js_temp.is_empty()) { js_tmp_cfg.add_empty_sub_object(vct_key[0]); js_temp = js_tmp_cfg[vct_key[0]]; }

            for(int i=1;i<key_num;i++)
            {
                if(i==key_num-1) // 最后一级赋值节点
                {
                    js_temp.del(vct_key[i]);
                    if(value_type == V_INT)
                        js_temp.add(vct_key[i],value_i);
                    else if(value_type == V_FLOAT)
                        js_temp.add(vct_key[i],value_f);
                    else
                        js_temp.add(vct_key[i],value_s);
                    break;
                }

                cjson_object &js_get_temp = js_temp[vct_key[i]];
                if(js_get_temp.is_empty())
                {
                    js_temp.add_empty_sub_object(vct_key[i]);
                    js_temp = js_temp[vct_key[i]];
                }
                else
                {
                    js_temp = js_get_temp;
                }
            }
        }while(0);

        js_string = js_tmp_cfg.to_formatted_string();
        m_js_config.parse(js_string);
        return true;
    }

protected:
    std::string       m_file_path;
    cjson_object      m_js_config;
};

#endif // XCONFIG_H