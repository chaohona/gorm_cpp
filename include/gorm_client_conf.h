#ifndef _GORM_CLIENT_CONF_H__
#define _GORM_CLIENT_CONF_H__

#include <yaml-cpp/yaml.h>

#include "gorm_singleton.h"
#include "gorm_sys_inc.h"
#include "gorm_type.h"
#include "gorm_define.h"
#include "gorm_inc.h"

namespace gorm{

#define GORM_SERVER_MAX_NUM 128     
#define GORM_MAX_CONN_NUM   128    // 可以和服务器建立的最大连接数
#define GORM_REQUEST_TT 10000
#define GORM_CONN_NUM 128
class GORM_ServerInfo
{
public:
    string serverAddress = string("127.0.0.1");   // 服务器host
    uint16 serverPort = 8806;                   // 服务器port
    bool   mainSvr = false;                     // 是否是主服务器,为主责优先使用
};

class GORM_Log;
class GORM_ClientConfig: public GORM_Singleton<GORM_ClientConfig>
{
public:
    GORM_ClientConfig(){}
    ~GORM_ClientConfig(){}
    bool Init(const char *cfgFile, GORM_Log *log = nullptr);
    // 获取一个新的地址
    GORM_ServerInfo *GetNextServer();
   
private:
    int ParaseServerList(YAML::Node &node);

public:
    int                 gormSvrNum = 0; // gorm服务器的总数
    int                 nowUsedIdx = 0; // 当前使用的地址在gormSvrList中的索引
    GORM_ServerInfo     gormSvrList[GORM_SERVER_MAX_NUM];   // gorm服务器地址列表
    int                 connNum = GORM_CONN_NUM;      // 客户端和服务器建立连接的个数
    int                 requestTT = GORM_REQUEST_TT;  // 请求超时时间，默认为10秒
    GORM_Log            *logHandle = nullptr;
	mutex cmtx;
};

}

#endif
