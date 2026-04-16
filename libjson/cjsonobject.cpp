#include "cjsonobject.h"

#define cJSON_GetErrorPtr()  source  //cJSON_GetErrorPtr()不支持多线程

cjson_object::cjson_object() : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    // m_pJsonData = cJSON_CreateObject();
}

cjson_object::cjson_object(const std::string& strJson) : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    parse(strJson);
}

cjson_object::cjson_object(const cjson_object* pJsonObject) : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    if(pJsonObject) parse(pJsonObject->to_string());
}

cjson_object::cjson_object(const cjson_object& oJsonObject) : m_pJsonData(NULL), m_pExternJsonDataRef(NULL)
{
    parse(oJsonObject.to_string());
}

cjson_object::~cjson_object()
{
    clear();
}

cjson_object& cjson_object::operator=(const cjson_object& oJsonObject)
{
    parse(oJsonObject.to_string().c_str());
    return(*this);
}

bool cjson_object::operator==(const cjson_object& oJsonObject) const
{
    return(this->to_string() == oJsonObject.to_string());
}

bool cjson_object::add_empty_sub_object(const std::string& strKey)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }

    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateObject();
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty object error!");
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add_empty_sub_array(const std::string& strKey)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }

    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateArray();
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty array error!");
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    m_listKeys.clear();
    return(true);
}

bool cjson_object::get_key(std::string& strKey)
{
    if (is_array()) return(false);
    if (m_listKeys.size() == 0)
    {
        cJSON* pFocusData = NULL;
        if (m_pJsonData != NULL)        pFocusData = m_pJsonData;
        else if (m_pExternJsonDataRef != NULL) pFocusData = m_pExternJsonDataRef;
        else return(false);
        cJSON *c = pFocusData->child;
        while (c)
        {
            m_listKeys.push_back(c->string);
            c = c->next;
        }
        m_itKey = m_listKeys.begin();
    }
    if (m_itKey == m_listKeys.end())
    {
        strKey = "";
        m_itKey = m_listKeys.begin();
        return(false);
    }
    else
    {
        strKey = *m_itKey;
        ++m_itKey;
        return(true);
    }
}

cjson_object& cjson_object::operator[](const std::string& strKey)
{
    std::map<std::string, cjson_object*>::iterator iter;
    iter = m_mapJsonObjectRef.find(strKey);
    if (iter == m_mapJsonObjectRef.end())
    {
        cJSON* pJsonStruct = NULL;
        if (m_pJsonData != NULL)
        {
            if (m_pJsonData->type == cJSON_Object)
                pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            if (m_pExternJsonDataRef->type == cJSON_Object)
                pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
        if (pJsonStruct == NULL)
        {
            cjson_object* pJsonObject = new cjson_object();
            m_mapJsonObjectRef.insert(std::pair<std::string, cjson_object*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            cjson_object* pJsonObject = new cjson_object(pJsonStruct);
            m_mapJsonObjectRef.insert(std::pair<std::string, cjson_object*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

cjson_object& cjson_object::operator[](unsigned int uiWhich)
{
    std::map<unsigned int, cjson_object*>::iterator iter;
    iter = m_mapJsonArrayRef.find(uiWhich);
    if (iter == m_mapJsonArrayRef.end())
    {
        cJSON* pJsonStruct = NULL;
        if (m_pJsonData != NULL)
        {
            if (m_pJsonData->type == cJSON_Array)
                pJsonStruct = cJSON_GetArrayItem(m_pJsonData, uiWhich);
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            if (m_pExternJsonDataRef->type == cJSON_Array)
                pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, uiWhich);
        }
        if (pJsonStruct == NULL)
        {
            cjson_object* pJsonObject = new cjson_object();
            m_mapJsonArrayRef.insert(std::pair<unsigned int, cjson_object*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            cjson_object* pJsonObject = new cjson_object(pJsonStruct);
            m_mapJsonArrayRef.insert(std::pair<unsigned int, cjson_object*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

std::string cjson_object::operator()(const std::string& strKey) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(std::string(""));
    if(pJsonStruct->type == cJSON_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if (pJsonStruct->type == cJSON_Number)
    {
        char szNumber[128] = {0};
        if(pJsonStruct->valuedouble == (double)pJsonStruct->valueint)
        {
            if ((int64)pJsonStruct->valueint <= (int64)INT_MAX && (int64)pJsonStruct->valueint >= (int64)INT_MIN)
                sprintf(szNumber, "%d", (int32)pJsonStruct->valueint);
            else
                sprintf(szNumber, "%lld", (int64)pJsonStruct->valueint);
        }
        else
        {
            if (fabs(pJsonStruct->valuedouble) < 1.0e-6 || fabs(pJsonStruct->valuedouble) > 1.0e9)
                sprintf(szNumber, "%e", pJsonStruct->valuedouble);
            else
                sprintf(szNumber, "%f", pJsonStruct->valuedouble);
        }
        return(std::string(szNumber));
    }
    else if (pJsonStruct->type == cJSON_False)
    {
        return(std::string("false"));
    }
    else if (pJsonStruct->type == cJSON_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

std::string cjson_object::operator()(unsigned int uiWhich) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, uiWhich);
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, uiWhich);
    }
    if (pJsonStruct == NULL)  return(std::string(""));
    if (pJsonStruct->type == cJSON_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if (pJsonStruct->type == cJSON_Number)
    {
        char szNumber[128] = {0};
        if(pJsonStruct->valuedouble == (double)pJsonStruct->valueint)
        {
            if ((int64)pJsonStruct->valueint <= (int64)INT_MAX && (int64)pJsonStruct->valueint >= (int64)INT_MIN)
                sprintf(szNumber, "%d", (int32)pJsonStruct->valueint);
            else
                sprintf(szNumber, "%lld", (int64)pJsonStruct->valueint);
        }
        else
        {
            if (fabs(pJsonStruct->valuedouble) < 1.0e-6 || fabs(pJsonStruct->valuedouble) > 1.0e9)
                sprintf(szNumber, "%e", pJsonStruct->valuedouble);
            else
                sprintf(szNumber, "%f", pJsonStruct->valuedouble);
        }
        return(std::string(szNumber));
    }
    else if (pJsonStruct->type == cJSON_False)
    {
        return(std::string("false"));
    }
    else if (pJsonStruct->type == cJSON_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

bool cjson_object::parse(const std::string& strJson)
{
    clear();
    m_pJsonData = cJSON_Parse(strJson.c_str());
    if (m_pJsonData == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    return(true);
}

void cjson_object::clear()
{
    m_pExternJsonDataRef = NULL;
    if (m_pJsonData != NULL)
    {
        cJSON_Delete(m_pJsonData);
        m_pJsonData = NULL;
    }

    for (std::map<unsigned int, cjson_object*>::iterator iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); ++iter)
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonArrayRef.clear();

    for (std::map<std::string, cjson_object*>::iterator iter = m_mapJsonObjectRef.begin(); iter != m_mapJsonObjectRef.end(); ++iter)
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonObjectRef.clear();
    m_listKeys.clear();
}

bool cjson_object::is_empty() const
{
    if (m_pJsonData != NULL) return(false);
    else if (m_pExternJsonDataRef != NULL) return(false);
    return(true);
}

bool cjson_object::is_array() const
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)        pFocusData = m_pJsonData;
    else if (m_pExternJsonDataRef != NULL) pFocusData = m_pExternJsonDataRef;
    if (pFocusData == NULL) return(false);
    if (pFocusData->type == cJSON_Array) return(true);
    else return(false);
}

bool cjson_object::is_object() const
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)        pFocusData = m_pJsonData;
    else if (m_pExternJsonDataRef != NULL) pFocusData = m_pExternJsonDataRef;
    if (pFocusData == NULL) return(false);
    if (pFocusData->type == cJSON_Object) return(true);
    else return(false);
}

std::string cjson_object::to_string() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if (m_pJsonData != NULL)        pJsonString = cJSON_PrintUnformatted(m_pJsonData);
    else if (m_pExternJsonDataRef != NULL) pJsonString = cJSON_PrintUnformatted(m_pExternJsonDataRef);
    if (pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}

std::string cjson_object::to_formatted_string() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if (m_pJsonData != NULL)        pJsonString = cJSON_Print(m_pJsonData);
    else if (m_pExternJsonDataRef != NULL) pJsonString = cJSON_Print(m_pExternJsonDataRef);
    if (pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}

bool cjson_object::get(const std::string& strKey, cjson_object& oJsonObject) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    char* pJsonString = cJSON_Print(pJsonStruct);
    std::string strJsonData = pJsonString;
    free(pJsonString);
    if (oJsonObject.parse(strJsonData)) return(true);
    else return(false);
}

bool cjson_object::get(const std::string& strKey, std::string& strValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL)  return(false);
    char sValue[24] = {0};
    if (pJsonStruct->type == cJSON_Number)
    {
        if(pJsonStruct->valuedouble == (double)pJsonStruct->valueint)
        {
            sprintf(sValue,"%d",(int)pJsonStruct->valueint);
        }
        else if (fabs(floor(pJsonStruct->valuedouble) - pJsonStruct->valuedouble) <= DBL_EPSILON)
        {
            sprintf(sValue, "%.0f", pJsonStruct->valuedouble);
        }
        else
            sprintf(sValue, "%g", pJsonStruct->valuedouble);
        strValue =sValue;
        return(true);
    }
    else if (pJsonStruct->type != cJSON_String)
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool cjson_object::get(const std::string& strKey, int32& iValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    iValue = (int32)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(const std::string& strKey, uint32& uiValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    uiValue = (uint32)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(const std::string& strKey, int64& llValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    llValue = (int64)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(const std::string& strKey, uint64& ullValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object) pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    ullValue = (uint64)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(const std::string& strKey, bool& bValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type > cJSON_True) return(false);
    bValue = (pJsonStruct->type==cJSON_False?false:true);
    return(true);
}

bool cjson_object::get(const std::string& strKey, float& fValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    fValue = (float)(pJsonStruct->valuedouble);
    return(true);
}

bool cjson_object::get(const std::string& strKey, double& dValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    dValue = pJsonStruct->valuedouble;
    return(true);
}

bool cjson_object::get(const std::string& strKey, float& fValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    fValue = (float)(pJsonStruct->valuedouble);
    return(true);
}

bool cjson_object::get(const std::string& strKey, double& dValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Object)
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
    }
    if (pJsonStruct == NULL) return(false);
    if (pJsonStruct->type != cJSON_Number) return(false);
    dValue = pJsonStruct->valuedouble;
    return(true);
}

bool cjson_object::add(const std::string& strKey, const cjson_object& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_Parse(oJsonObject.to_string().c_str());
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    std::map<std::string, cjson_object*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((int32)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((uint32)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, bool bValue, bool bValueAgain)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::add(const std::string& strKey, double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::del(const std::string& strKey)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON_DeleteItemFromObject(pFocusData, strKey.c_str());
    std::map<std::string, cjson_object*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_listKeys.clear();
    return(true);
}

bool cjson_object::replace(const std::string& strKey, const cjson_object& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_Parse(oJsonObject.to_string().c_str());
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    std::map<std::string, cjson_object*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((int32)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((uint32)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, bool bValue, bool bValueAgain)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(const std::string& strKey, double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Object)
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct);
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) == NULL)
    {
        return(false);
    }
    return(true);
}

int cjson_object::get_array_size()
{
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            return(cJSON_GetArraySize(m_pJsonData));
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            return(cJSON_GetArraySize(m_pExternJsonDataRef));
        }
    }
    return(0);
}

bool cjson_object::get(int iWhich, cjson_object& oJsonObject) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    char* pJsonString = cJSON_Print(pJsonStruct);
    std::string strJsonData = pJsonString;
    free(pJsonString);
    if (oJsonObject.parse(strJsonData))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

bool cjson_object::get(int iWhich, std::string& strValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    char sValue[24] = {0};
    if (pJsonStruct->type == cJSON_Number)
    {
        if(pJsonStruct->valuedouble == (double)(pJsonStruct->valueint)) //整型值
            sprintf(sValue, "%d", (int)pJsonStruct->valueint);
        else if(floor(pJsonStruct->valuedouble) - pJsonStruct->valuedouble <= DBL_EPSILON) //浮点型值
            sprintf(sValue, "%.0f", pJsonStruct->valuedouble);
        else
            sprintf(sValue, "%g", pJsonStruct->valuedouble);
        strValue = sValue;
        return (true);
    }
    else if (pJsonStruct->type != cJSON_String)
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool cjson_object::get(int iWhich, int32& iValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    iValue = (int32)(pJsonStruct->valueint);
    return(true);
}

bool cjson_object::get(int iWhich, uint32& uiValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    uiValue = (uint32)(pJsonStruct->valueint);
    return(true);
}

bool cjson_object::get(int iWhich, int64& llValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    llValue = (int64)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(int iWhich, uint64& ullValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    ullValue = (uint64)pJsonStruct->valueint;
    return(true);
}

bool cjson_object::get(int iWhich, bool& bValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type > cJSON_True)
    {
        return(false);
    }
    bValue = (pJsonStruct->type==cJSON_False?false:true);
    return(true);
}

bool cjson_object::get(int iWhich, float& fValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    fValue = (float)(pJsonStruct->valuedouble);
    return(true);
}

bool cjson_object::get(int iWhich, double& dValue) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type == cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type != cJSON_Number)
    {
        return(false);
    }
    dValue = pJsonStruct->valuedouble;
    return(true);
}

bool cjson_object::add(const cjson_object& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_Parse(oJsonObject.to_string().c_str());
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    unsigned int uiLastIndex = (unsigned int)cJSON_GetArraySize(pFocusData) - 1;
    for (std::map<unsigned int, cjson_object*>::iterator iter = m_mapJsonArrayRef.begin();
         iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->first >= uiLastIndex)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

bool cjson_object::add(const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((int32)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((uint32)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(int iAnywhere, bool bValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add(double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArray(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(const cjson_object& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_Parse(oJsonObject.to_string().c_str());
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    for (std::map<unsigned int, cjson_object*>::iterator iter = m_mapJsonArrayRef.begin();
         iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter++);
    }
    return(true);
}

bool cjson_object::add_as_first(const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((int32)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((uint32)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(int iAnywhere, bool bValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::add_as_first(double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    int iArraySizeBeforeAdd = cJSON_GetArraySize(pFocusData);
    cJSON_AddItemToArrayHead(pFocusData, pJsonStruct);
    int iArraySizeAfterAdd = cJSON_GetArraySize(pFocusData);
    if (iArraySizeAfterAdd == iArraySizeBeforeAdd)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::del(int iWhich)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON_DeleteItemFromArray(pFocusData, iWhich);
    for (std::map<unsigned int, cjson_object*>::iterator iter = m_mapJsonArrayRef.begin();
         iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->first >= (unsigned int)iWhich)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

bool cjson_object::replace(int iWhich, const cjson_object& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_Parse(oJsonObject.to_string().c_str());
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + cJSON_GetErrorPtr();
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    std::map<unsigned int, cjson_object*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber(uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((int32)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((uint32)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, bool bValue, bool bValueAgain)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

bool cjson_object::replace(int iWhich, double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (pFocusData->type != cJSON_Array)
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = cJSON_CreateNumber((double)dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct);
    if (cJSON_GetArrayItem(pFocusData, iWhich) == NULL)
    {
        return(false);
    }
    return(true);
}

cjson_object::cjson_object(cJSON* pJsonData) : m_pJsonData(NULL), m_pExternJsonDataRef(pJsonData)
{

}