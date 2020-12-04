#ifndef _GORM_LOG_H__
#define _GORM_LOG_H__
#include "gorm_singleton.h"
#include "gorm_sys_inc.h"
#include "gorm_define.h"
#include "gamesh/galog/gamesh_log_help.h"

#define LOG_MAX_LEN 1024 /* max length of log message */

namespace gorm {

class GORM_DefaultLog;

#define GORM_CUSTOM_LOGE(logHandle, ...)                                               \
if (logHandle != nullptr && logHandle->m_iLogLevel>=gorm::GORM_LOG_LEVEL_ERROR)                 \
logHandle->Error(__FILE__, __LINE__,__VA_ARGS__);

#define GORM_CUSTOM_LOGI(logHandle, ...)                                               \
if (logHandle != nullptr && logHandle->m_iLogLevel>=gorm::GORM_LOG_LEVEL_INFO)                  \
logHandle->Info(__FILE__, __LINE__,__VA_ARGS__);

#define GORM_CUSTOM_LOGD(logHandle, ...)                                               \
if (logHandle != nullptr && logHandle->m_iLogLevel>=gorm::GORM_LOG_LEVEL_DEBUG)                 \
logHandle->Debug(__FILE__, __LINE__,__VA_ARGS__);

#define GORM_CUSTOM_LOGW(logHandle, ...)                                               \
if (logHandle != nullptr && logHandle->m_iLogLevel>=gorm::GORM_LOG_LEVEL_WARNING)               \
logHandle->Error(__FILE__, __LINE__,__VA_ARGS__);

#define GORM_CUSTOM_STDERR(logHandle, ...)     \
if (logHandle != nullptr)                      \
logHandle->StdErr(__FILE__, __LINE__,__VA_ARGS__);

class GORM_Log
{
public:
    virtual ~GORM_Log();

    virtual bool Init() = 0;

    virtual void Debug(const char *szFile, int iLine, const char *szArgs, ...) = 0;
    virtual void Info(const char *szFile, int iLine, const char *szArgs, ...) = 0;
    virtual void Error(const char *szFile, int iLine, const char *szArgs, ...) = 0;
    virtual void StdErr(const char *szFile, int iLine, const char *szArgs, ...) = 0;
    
    virtual void SetLogLevel(gorm::GORM_LOG_LEVEL logLevel);

public:
    gorm::GORM_LOG_LEVEL m_iLogLevel;
};

// 默认输出到标准错误
class GORM_DefaultLog: public GORM_Log
{
public:
    virtual ~GORM_DefaultLog();
    virtual bool Init();

    virtual void Debug(const char *szFile, int iLine, const char *szArgs, ...);
    virtual void Info(const char *szFile, int iLine, const char *szArgs, ...);
    virtual void Error(const char *szFile, int iLine, const char *szArgs, ...);
    virtual void StdErr(const char *szFile, int iLine, const char *szArgs, ...);

    static GORM_DefaultLog* Instance();
private:
    static GORM_DefaultLog *pInstance;
    mutex m_Mutex;
};


}

#endif

