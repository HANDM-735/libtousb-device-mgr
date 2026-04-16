#ifndef CJSON_OBJECT_HPP_
#define CJSON_OBJECT_HPP_

#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <errno.h>

#ifndef _WINDOWS //WINDOWS
#include <unistd.h>
#endif

#include <limits.h>
#include <math.h>
#include <float.h>
#include <string>
#include <map>
#include <list>

#ifdef __cplusplus
extern "C" {
#endif
#include "cjson.h"
#ifdef __cplusplus
}
#endif

typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

class cjson_object
{
    // method of ordinary json object or json array
public:
    cjson_object();
    cjson_object(const std::string& strJson);
    cjson_object(const cjson_object* pJsonObjects);
    cjson_object(const cjson_object& oJsonObjects);
    virtual ~cjson_object();

    cjson_object& operator=(const cjson_object& oJsonObjects);
    bool parse(const std::string& strJson);
    void clear();
    bool is_empty() const;
    bool is_array() const;
    bool is_object() const;
    std::string to_string() const;
    std::string to_formatted_string() const;

    std::string get_err_msg() const {return m_strErrMsg;}

    bool add_empty_object(const std::string& strKey);
    bool add_empty_sub_array(const std::string& strKey);

    cjson_object& operator[](sslocal://flow/file_open?url=const+std%3A%3Astring%26+strKey&flow_extra=eyJsaW5rX3R5cGUiOiJjb2RlX2ludGVycHJldGVyIn0=);
    std::string operator()(const std::string& strKey) const;

    bool get(const std::string& strKey, std::string& strValue) const;
    bool get(const std::string& strKey, int32& iValue) const;
    bool get(const std::string& strKey, uint32& uiValue) const;
    bool get(const std::string& strKey, int64& iValue) const;
    bool get(const std::string& strKey, uint64& uiValue) const;
    bool get(const std::string& strKey, bool& bValue) const;
    bool get(const std::string& strKey, float& fValue) const;
    bool get(const std::string& strKey, double& dValue) const;

    bool add(const std::string& strKey, const cjson_object& oJsonObjects);
    bool add(const std::string& strKey, const std::string& strValue);
    bool add(const std::string& strKey, int32 iValue);
    bool add(const std::string& strKey, uint32 uiValue);
    bool add(const std::string& strKey, int64 iValue);
    bool add(const std::string& strKey, uint64 uiValue);
    bool add(const std::string& strKey, bool bValue);
    bool add(const std::string& strKey, bool bValueAgain);
    bool add(const std::string& strKey, float fValue);
    bool add(const std::string& strKey, double dValue);

    bool replace(const std::string& strKey, const cjson_object& oJsonObjects);
    bool replace(const std::string& strKey, const std::string& strValue);
    bool replace(const std::string& strKey, int32 iValue);
    bool replace(const std::string& strKey, uint32 uiValue);
    bool replace(const std::string& strKey, uint64 uiValue);
    bool replace(const std::string& strKey, bool bValue, bool bValueAgain);
    bool replace(const std::string& strKey, float fValue);
    bool replace(const std::string& strKey, double dValue);

    // method of json array
public:
    int get_array_size();
    cjson_object& operator[](sslocal://flow/file_open?url=unsigned+int+uiWhich&flow_extra=eyJsaW5rX3R5cGUiOiJjb2RlX2ludGVycHJldGVyIn0=);
    std::string get(int iWhich, cjson_object& oJsonObjects) const;
    std::string get(int iWhich, const std::string& strValue) const;
    bool get(int iWhich, int32& iValue) const;
    bool get(int iWhich, uint32& uiValue) const;
    bool get(int iWhich, int64& iValue) const;
    bool get(int iWhich, uint64& uiValue) const;
    bool get(int iWhich, bool& bValue) const;
    bool get(int iWhich, float& fValue) const;
    bool get(int iWhich, double& dValue) const;

    bool add(int iWhich, const cjson_object& oJsonObjects);
    bool add(int iWhich, const std::string& strValue);
    bool add(int iWhich, int32 iValue);
    bool add(int iWhich, uint32 uiValue);
    bool add(int iWhich, int64 iValue);
    bool add(int iWhich, uint64 uiValue);
    bool add(int iWhich, bool bValue);
    bool add_as_first(const cjson_object& oJsonObjects);
    bool add_as_first(const std::string& strValue);
    bool add_as_first(int32 iValue);
    bool add_as_first(uint32 uiValue);
    bool add_as_first(int64 iValue);
    bool add_as_first(uint64 uiValue);
    bool add_as_first(bool bValue);
    bool add_as_first(float fValue);
    bool add_as_first(double dValue);

    bool del(int iWhich);

    bool replace(int iWhich, const cjson_object& oJsonObjects);
    bool replace(int iWhich, const std::string& strValue);
    bool replace(int iWhich, int32 iValue);
    bool replace(int iWhich, uint32 uiValue);
    bool replace(int iWhich, uint64 uiValue);
    bool replace(int iWhich, bool bValue, bool bValueAgain);
    bool replace(int iWhich, float fValue);
    bool replace(int iWhich, double dValue);

private:
    cJSON* pJsonData;

    CJSON *
    std::string
    std::map<unsigned int, cjson_object*>
    std::map<std::string, cjson_object*>
    std::list<std::string>
    std::string::const_iterator
};

#endif // CJSONHELPER_HPP_ //