#ifndef _GORM_CLIENT_REQUEST_H__
#define _GORM_CLIENT_REQUEST_H__

#include "gorm_mempool.h"
#include "gorm_sys_inc.h"
#include "gorm_msg_helper.h"
#include "gorm_table.h"

namespace gorm{

class GORM_ClientMsg;

using GORM_CbFun = int (*)(uint32, int, shared_ptr<GORM_ClientTable>);
using GORM_GetCbFunc = void (*)(GORM_ClientMsg *clientMsg);


// 回调函数
class GORM_ClientMsg
{
public:
    ~GORM_ClientMsg();
public:
    void Reset();
    // 到这里说明响应本身没有问题，需要解析返回数据包体的内容了
    int ParseRsp(char *msgBeginPos, int msgLen);
    int PackReq();
    void Wait();    // 发送完等待响应,默认等待10秒，没响应则认为失败
    void Signal();    // 接收到响应之后通知结束等待
    void ProcCallBack();
private:
    int PackReqInsert();
    int PackReqDelete();
    int PackReqGet();
    int PackReqUpdate();
    int PackReplace();
    int PackGetByNonPrimaryKey();
    int PackHandShake();
    int MakeSendBuff();

    int ParseRspGet(char *msgBeginPos, int msgLen);
    int ParseRspGetByNonPrimaryKey(char *msgBeginPos, int msgLen);
public:
    GORM_REQUEST_NEED_CB_FLAG  needCBFlag = GORM_REQUEST_NEED_CB;
    int                 region = 0;
    int                 logicZone = 0;
    int                 physicZone = 0;
    int                 reqCmd = 0;                 // 请求的协议号
    int                 tableId = 0;                    // 请求的表id
    int                 limitNum = 0;
    int                 verPolicy = NOCHECKDATAVERSION_AUTOINCREASE;
    uint32              reqFlag = GORM_ResultFlag_RETURN_CODE;
    int                 refTableIndex = -1;                // 根据表下标获取数据
    uint32               cbId = 0;                      // 请求序列号，用于回调
    GORM_MemPoolData    *reqMemData = nullptr;          // 组装好的需要发送的消息
    PB_MSG_PTR          pbReqMsg = nullptr;             // 请求的get等pb请求的pb消息
    PB_MSG_PTR          pbRspMsg = nullptr;             // 响应的get等pb消息
    GROM_ResponseCode   rspCode;                        // 请求的错误信息
    GORM_FieldsOpt      *fieldOpt = nullptr;
    GORM_Log            *logHandle = nullptr;
    GORM_CbFun          cbFunc = nullptr;
    GORM_GetCbFunc      getCBFunc = nullptr;
    shared_ptr<GORM_ClientTable>    reqTable = nullptr;
    mutex               mtx;    // 针对消息的锁，主要是多线程同步消息的内容使用，不会出现多线程争用此锁的情况
};

}
#endif
