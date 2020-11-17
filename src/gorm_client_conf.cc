#include "gorm_client_conf.h"
#include "gorm_log.h"

namespace gorm{

bool GORM_ClientConfig::Init(const char *cfgFile, GORM_Log *log)
{
    unique_lock<mutex> lck(this->cmtx);
    if (log == nullptr)
        log = GORM_DefaultLog::Instance();
    this->logHandle = log;

    try
    {
        YAML::Node node = YAML::LoadFile(cfgFile);
        for(auto c=node.begin(); c!=node.end(); c++)
        {
            string content = c->first.as<string>();
            if (content == "gorm-server-list")
            {
                if (0 != this->ParaseServerList(c->second))
                {
                    GORM_CUSTOM_LOGE(log, "parse gorm config failed ::gorm-server-list");
                    return false;
                }
            }
            else if(content == "conn-num")
            {
                this->connNum = c->second.as<int>();
                if (this->connNum <= 0)
                    this->connNum = 1;
                if (this->connNum > GORM_MAX_CONN_NUM)
                    this->connNum = GORM_MAX_CONN_NUM;
            }
            else if (content == "request-tt")
            {
                this->requestTT = c->second.as<int>();
                if (this->requestTT < 1)
                {
                    this->requestTT = GORM_REQUEST_TT;
                }
            }
        }

        if (this->gormSvrNum == 0)
        {
            GORM_CUSTOM_LOGE(logHandle, "gorm config error, no gorm server.");
            return false;
        }
    }
    catch(exception &e)
    {
        GORM_CUSTOM_LOGE(logHandle, "parse gorm config file got exception, file:%s, exception:%s", cfgFile, e.what());
        return false;
    }
    return true;
}

int GORM_ClientConfig::ParaseServerList(YAML::Node &node)
{
    this->gormSvrNum = node.size();
    if (this->gormSvrNum > GORM_SERVER_MAX_NUM)
        this->gormSvrNum = GORM_SERVER_MAX_NUM;

    for(int i=0; i<this->gormSvrNum; i++)
    {
        GORM_ServerInfo *svrInfo = &this->gormSvrList[i];
        auto cfgInfo = node[i];
        if (!cfgInfo["host"] || !cfgInfo["port"])
        {
            GORM_CUSTOM_LOGE(logHandle, "invalid config ::gorm-server-list::index(%d)", i);
            return GORM_ERROR;
        }

        svrInfo->serverAddress = cfgInfo["host"].as<string>();
        svrInfo->serverPort = cfgInfo["port"].as<int>();
        if (cfgInfo["main"] && cfgInfo["main"].as<string>() == "true")
            svrInfo->mainSvr = true;
    }
    return GORM_OK;
}

GORM_ServerInfo *GORM_ClientConfig::GetNextServer()
{
    unique_lock<mutex> lck(this->cmtx);
    return &this->gormSvrList[0];
}

}
