#include "gorm_wrap.h"
#include "gorm_client_conf.h"
#include "gorm_error.h"
#include "gorm_client_thread.h"
#include "gorm_utils.h"
#include "gorm_client_msg.h"

namespace gorm{

GORM_Wrap::GORM_Wrap()
{
    this->seqIdx = 0;
    
}
GORM_Wrap::~GORM_Wrap()
{
    this->memPool = nullptr;
    if (this->msgList != nullptr)
    {
        delete []this->msgList;
    }
}

#define GORM_INIT_FAILED_LOG()\
GORM_CUSTOM_LOGD(logHandle, "gorm client init failed...");

// TODO 阻塞，知道工作线程启动起来为止
int GORM_Wrap::Init(const char *cfgPath, GORM_Log *logHandle)
{
    GORM_CUSTOM_LOGI(logHandle, "gorm client init begin...");
    this->msgList = new GORM_ClientMsg[GORM_MAX_CLIENT_REQUEST_POOL];
    GORM_ClientMsg *tmp = this->msgList;
    for (int i=0; i<GORM_MAX_CLIENT_REQUEST_POOL; i++)
    {
        this->clientMsgPool.Put(tmp++);
    }
    if (logHandle == nullptr)
        logHandle = GORM_DefaultLog::Instance();
    this->logHandle = logHandle;

    // 
    this->memPool = make_shared<GORM_MemPool>();

    // 解析配置文件
    if (!GORM_ClientConfig::GetSelfSafe()->Init(cfgPath, this->logHandle))
    {
        GORM_CUSTOM_LOGE(logHandle, "parse gorm client config failed, config file:%s", cfgPath);
        GORM_INIT_FAILED_LOG();
        return GORM_ERROR;
    }

    // 启动工作线程
    if (GORM_OK != GORM_ClientThreadPool::Instance()->CreateThread(this->logHandle, 1))
    {
        GORM_CUSTOM_LOGE(logHandle, "create gorm client thread failed.");
        GORM_INIT_FAILED_LOG();
        return GORM_ERROR;
    }
    // 每隔10毫秒检查一次和客户端连接线程是否已经启动完成，或者和gorm连接失败，最多等待1分钟，没有结果则返回失败
    for(int i=0; i<60*100; i++)
    {
        int startStatus = GORM_ClientThreadPool::Instance()->ClientThreadStartStatus();
        if (GORM_ClientStartStatus_Init == startStatus)
        {
            ThreadSleepMilliSeconds(10);
            continue;
        }
        else if (GORM_ClientStartStatus_Failed == startStatus)
        {
            GORM_CUSTOM_LOGE(logHandle, "gorm client thread start failed.");
            GORM_INIT_FAILED_LOG();
            return GORM_ERROR;
        }
        else if (GORM_ClientStartStatus_Vesion_Failed == startStatus)
        {
            GORM_CUSTOM_LOGE(logHandle, "gorm client thread start failed.");
            GORM_INIT_FAILED_LOG();
            return GORM_VERSION_NOT_MATCH;
        }
    }
    GORM_CUSTOM_LOGI(logHandle, "gorm client init success...");
	return GORM_OK;
}

int GORM_Wrap::Stop(bool force)
{
    GORM_ClientThreadPool::Instance()->Stop();

    return GORM_OK;
}

int GORM_Wrap::OnUpdate()
{
    GORM_ClientMsg *clientMsg = nullptr;
    // 每个循环最多处理1000个请求
    for(int i=0; i<1000; i++)
    {
        if (GORM_OK != GORM_ClientThreadPool::Instance()->GetResponse(clientMsg))
        {
            return GORM_ERROR;
            break;
        }
        if (clientMsg == nullptr)
        {
            break;
        }
        // 处理回调函数
        clientMsg->ProcCallBack();      
    }
    return GORM_OK;
}

}
