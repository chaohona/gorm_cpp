#include "gorm_client_event.h"
#include "gorm_client_thread.h"
#include "gorm_table_field_map_define.h"

namespace gorm{

GORM_ClientEvent::GORM_ClientEvent()
{}


GORM_ClientEvent::~GORM_ClientEvent()
{
}

void GORM_ClientEvent::Init(GORM_Log *logHandle, shared_ptr<GORM_Epoll> epoll, shared_ptr<GORM_Thread>& thread)
{
    this->m_pEpoll = epoll;
    cbIdSeedAtomic = 0;
    pendingSendMsgNum = 0;
    sendChannelUsedIdx = 0;
    // 一个连接只有一个读缓冲区，申请大一点
    GORM_ClientThread *clientThread = dynamic_cast<GORM_ClientThread*>(thread.get());
    readingBuffer = clientThread->m_pMemPool->GetData(1024*1024);
    readPos = readingBuffer->m_uszData;
    beginReadPos = readPos;
}

int GORM_ClientEvent::Close()
{
    uint64 now = GORM_GetNowMS();
    GORM_ClientMsg *clientMsg = nullptr;
    auto n = this->clientWaitRspMsgMap.begin();
    
    GALOG_DEBUG("gorm, close connection with gorm server, wait for rsp map size is:%d", this->clientWaitRspMsgMap.size());
    GORM_Event::Close();
    for (int j=0; j<6000; j++)
    {
        for (int i=0; i<1000 * 10; i++)
        {
            if (GORM_OK == Reconnect(this->m_szIP, this->m_uiPort))
            {
                // 重连之后重发未得到响应的消息
                this->DoAfterReconnect();
                return GORM_OK;
            }
            GALOG_DEBUG("gorm, reconnect with gorm server, failed, try again...");
            // 关闭reconnect中的句柄
            GORM_Event::Close();
            ThreadSleepMilliSeconds(10);
        }
        ThreadSleepSeconds(1);
        this->NetLoopCheck(0);
    }

    GALOG_DEBUG("gorm, close connection with gorm server.");
    return GORM_OK;
}

bool clientMsgCompare(GORM_ClientMsg* a, GORM_ClientMsg* b)
{
    return a->cbId > b->cbId;
}

int GORM_ClientEvent::DoAfterReconnect()
{
    if (this->clientWaitRspMsgMap.size() == 0)
        return GORM_OK;
    // 获取clientWaitRspMsgMap最小值
    GORM_ClientMsg *clientMsg = nullptr;
    int maxReqId = -1;
    auto n = this->clientWaitRspMsgMap.begin();
    uint64 now = GORM_GetNowMS();
    vector<GORM_ClientMsg*> tmpSortV;
    int index = 0;
    while(n != this->clientWaitRspMsgMap.end())
    {   
        clientMsg = n->second;
        ++n;
        if (clientMsg != nullptr)
        {   
            clientMsg->requestTMS = now;
            tmpSortV.push_back(clientMsg);
            continue;
        }
    }
    this->clientWaitRspMsgMap.clear();
    sort(tmpSortV.begin(), tmpSortV.end(), clientMsgCompare);
    for(int i=0; i<tmpSortV.size(); i++)
    {
        if (!this->waitSendingChannel.PutPre(tmpSortV[i]))
        {
            GALOG_ERROR("gorm, resend request failed, reqid:%d", clientMsg->cbId);
            continue;
        }
        GALOG_DEBUG("gorm, resend request success, reqid:%d", clientMsg->cbId);
    }

    if (GORM_OK != this->SendHandShakeMsg())
    {
        GORM_CUSTOM_LOGE(logHandle, "send hand shake message failed");
        return GORM_ERROR;
    }
    return GORM_OK;
}

int GORM_ClientEvent::Reconnect(const char *szIP, uint16 uiPort)
{
    readPos = readingBuffer->m_uszData;
    beginReadPos = readPos;
    sendingRequest = nullptr;
    needReadLen = 0;
    sendingPos = nullptr;

    int iFD = socket(AF_INET, SOCK_STREAM, 0);
    if (iFD < 0)
    {
        GORM_CUSTOM_LOGE(logHandle, "create socket failed:%s,%d, errno:%d,errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return GORM_ERROR;
    }
    this->m_iFD = iFD;
    sockaddr_in server_address;
#ifdef _WIN32
    memset(&server_address, 0, sizeof(server_address));
#else
    bzero(&server_address, sizeof(server_address));
#endif
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, szIP, &server_address.sin_addr);
    server_address.sin_port = htons(uiPort);

    this->m_Status = GORM_CONNECT_CONNECTING;
    do
    {
        int iRet = connect(iFD, (sockaddr*)&server_address, sizeof(server_address));
        if (iRet != 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            GORM_CUSTOM_LOGE(logHandle, "connecto to server failed, server address %s:%d, errno:%d, errmsg:%s", 
                    szIP, uiPort, errno, strerror(errno));
                    
            return GORM_CONN_FAILED;
        }
        break;
    }while(true);
    this->m_Status = GORM_CONNECT_CONNECTED;

    int iRet = GORM_Socket::SetNonBlocking(iFD);
    if (iRet != GORM_OK)
    {
        GORM_CUSTOM_LOGE(logHandle, "set socket nonblock failed:%s,%d, errno:%d, errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return iRet;
    }
    iRet = GORM_Socket::SetTcpNoDelay(iFD);
    if (iRet != GORM_OK)
    {
        GORM_CUSTOM_LOGE(logHandle, "set socket tcpnodelay failed:%s,%d, errno:%d, errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return iRet;
    }
    iRet = GORM_Socket::SetTcpKeepAlive(iFD);
    if (iRet != GORM_OK)
    {
        GORM_CUSTOM_LOGE(logHandle, "set socket SetTcpKeepAlive failed:%s,%d, errno:%d, errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return iRet;
    }
    iRet = GORM_Socket::SetLinger(iFD, 0);
    if (iRet != GORM_OK)
    {
        GORM_CUSTOM_LOGE(logHandle, "set socket SetLinger failed:%s,%d, errno:%d, errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return iRet;
    }

	this->m_pEpoll->AddEventRW(this);
    GALOG_DEBUG("gorm, connect with gorm succes.");

    return GORM_OK;
}


int GORM_ClientEvent::ConnectToServer(const char *szIP, uint16 uiPort)
{
    this->m_uiPort = uiPort;
    strncpy(m_szIP, szIP, NET_IP_STR_LEN);
    if (GORM_OK != this->Reconnect(szIP, uiPort))
    {
        return GORM_ERROR;
    }

    if (GORM_OK != this->SendHandShakeMsg())
    {
        GORM_CUSTOM_LOGE(logHandle, "send hand shake message failed:%s,%d, errno:%d, errmsg:%s", szIP, uiPort, errno, strerror(errno));
        return GORM_ERROR;
    }

    return GORM_OK;
}

void GORM_ClientEvent::NetLoopCheck(uint64 loopIndex)
{
    // 超时检查
    uint64 now = GORM_GetNowMS();
    GORM_ClientMsg *clientMsg = nullptr;
    auto n = this->clientWaitRspMsgMap.begin();
    
    //GALOG_DEBUG("gorm, wait for rsp map size is:%d", this->clientWaitRspMsgMap.size());
    while(n != this->clientWaitRspMsgMap.end())
    {   
        clientMsg = n->second;
        if (clientMsg == nullptr)
        {   
            if (n->first > 1)
                GALOG_DEBUG("gorm, erase message from wait rsp map, caused by nullptr, reqid:%d", n->first);
            n = this->clientWaitRspMsgMap.erase(n);
            continue;
        }
        // TODO 超时时间可配，加上统计日志
        else if (now - clientMsg->requestTMS > 3000)
        {
            GALOG_DEBUG("gorm, erase message from wait rsp map, caused by tt, reqid:%d", n->first);
            n = this->clientWaitRspMsgMap.erase(n);
            clientMsg->rspCode.code = GORM_REQEUST_TT;
            this->PutResponseToList(clientMsg->cbId, clientMsg);
            continue;
        }
        else
            ++n;
   }
}

int GORM_ClientEvent::PutResponseToList(uint32 cbId, GORM_ClientMsg *reqMsg)
{
    GORM_ClientThread *clientThread = dynamic_cast<GORM_ClientThread*>(this->myThread.get());
    int64 nowNum = 0;

    clientThread->responseMsgList.Put(reqMsg, nowNum);
    if (nowNum > 10)
        GALOG_DEBUG("gorm, response list is:%lld", nowNum);
    uint64 now = GORM_GetNowMS();
    GALOG_DEBUG("gorm, response network, reqid:%d, time:%llu", reqMsg->cbId, now-reqMsg->requestTMS);
    return GORM_OK;
}

void GORM_ClientEvent::WorkLoopCheck()
{

}

void GORM_ClientEvent::SetThread(shared_ptr<GORM_Thread> &thread)
{
    this->myThread = thread;
}

int GORM_ClientEvent::Write()
{
    for(;;)
    {
        this->GetNextSendingRequest();
        // 没有需要发送的消息
        if (this->sendingRequest == nullptr)
        {
            this->m_pEpoll->DelEventWrite(this);
            return GORM_OK;
        }

        unique_lock<mutex> lck(this->sendingRequest->mtx); // 加个锁，保证安全的获取到待发送消息中的数据
        int needSendLen = this->sendingRequest->reqMemData->m_sUsedSize - (this->sendingPos - this->sendingRequest->reqMemData->m_uszData);
        int iWriteLen = write(this->m_iFD, this->sendingPos, needSendLen);
        if (iWriteLen == 0)
        {
            return GORM_ERROR;
        }
        // TODO 断线重连流程
        if (iWriteLen < 0)
        {
            return GORM_ERROR;
        }
        needSendLen -= iWriteLen;
        this->sendingPos += iWriteLen;
        // 发送完一个消息
        if (needSendLen == 0)
        {
            this->OneRequestFinishSend();
            break;
        }
        break;
    }
    return GORM_OK;
}


#define GORM_SOCKET_READ_MAX_BUFF 1024*127
// TODO 和gorm server 断线之后将本地等待响应的消息返回给上层
int GORM_ClientEvent::Read()
{
    // TODO 判断是否可读，是否是断线
    // 1、判断是否可读，如果不可读则返回GORM_EAGAIN
    // 2、如果可读判断是否读取了一整个消息
    int iNeedRead = 0;
    int iNowReadLen = 0;
    bool bCouldContinue = true;
    do
    {
        iNowReadLen = recv(m_iFD, readPos, readingBuffer->m_uszEnd-readPos, 0);
        if (iNowReadLen == 0)
        {
            GORM_CUSTOM_LOGE(logHandle, "read from server failed");
            this->Close();
            return GORM_ERROR;
        }
        if (iNowReadLen < 0)
        {
            if (errno == EINTR)
            {
                GORM_CUSTOM_LOGD(logHandle, "recv from redis not ready - eintr");
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                GORM_CUSTOM_LOGD(logHandle, "recv from redis not ready - eagain");
                return GORM_EAGAIN;
            }
            GORM_CUSTOM_LOGE(logHandle, "redis read failed:%d, errmsg:%s", errno, strerror(errno));
            this->Close();
            return GORM_ERROR;
        }
        // 读取的数据小于一次可读的最大值，说明数据已经被读完了
        if (iNowReadLen < GORM_SOCKET_READ_MAX_BUFF && iNowReadLen < (readingBuffer->m_uszEnd-readPos))
        {
            bCouldContinue = false;
        }
        readPos += iNowReadLen;
        do{
            if (needReadLen == 0 )
            {
                if (GORM_OK != this->BeginReadNextMsg())
                {
                    GORM_CUSTOM_LOGE(logHandle, "parse rsp message failed, close connect with gorm server.");
                    return GORM_ERROR;
                }
            }
            // 获取到至少一条消息
            if (needReadLen != 0 && needReadLen <= readPos-beginReadPos)
            {
                if (GORM_OK != OneRspFinishRead())
                {
                    GORM_CUSTOM_LOGE(logHandle, "parse rsp message failed, close connect with gorm server.");
                    return GORM_ERROR;
                }
                beginReadPos = beginReadPos += needReadLen;
                needReadLen = 0;
                continue;   // 开始解析下一条消息
            }
            else
            {
                break;
            }
        }while(true);
        
        if (!bCouldContinue)
            break;
    }while(true);
    return GORM_OK;
}

// 开始处理一条新的消息
int GORM_ClientEvent::BeginReadNextMsg()
{
    int gotMsgLen = readPos-beginReadPos;
    if (needReadLen == 0 && gotMsgLen > 4)
    {
        needReadLen = GORM_GetMsgLen(beginReadPos);
        if (needReadLen > GORM_MAX_REQUEST_LEN)
        {
            GORM_CUSTOM_LOGE(logHandle, "response is too large.");
            return GORM_ERROR;
        }
        // 需要的长度比当前剩余长度大
        if (needReadLen > (readingBuffer->m_uszEnd-beginReadPos))
        {
            // 消息长度比缓冲区大则，重新申请缓冲区
            if (needReadLen > readingBuffer->m_sCapacity)
            {
                GORM_MemPoolData *pOldData = readingBuffer;
                readingBuffer = this->myThread->m_pMemPool->GetData(beginReadPos, gotMsgLen, needReadLen);
                pOldData->Release();
            }
            else // 将消息拷贝到开始的地方
            {
                memmove(readingBuffer->m_uszData, beginReadPos, gotMsgLen);
            }
            beginReadPos = readingBuffer->m_uszData;
            readPos = beginReadPos + gotMsgLen;
        }
    }

    return GORM_OK;
}


// 读取到至少一条完整消息了，开始解析
int GORM_ClientEvent::OneRspFinishRead()
{
    // 1、获取到消息id，并找到对应的请求
    // 2、解包，
    // 3、调用回调,或者放到接收队列等待业务线程使用
    int reqCmd;
    uint32 cbID;
    char preErrCode;
    uint8 preFlag;
    GORM_GetRspHeader(this->beginReadPos+GORM_MSG_LEN_BYTE, reqCmd, cbID, preErrCode, preFlag);
    // 找到对应的请求
    GORM_ClientMsg *rspMsg = nullptr;
    int ret = this->GetReqMsgForResponse(cbID, rspMsg);
    if (ret != GORM_OK)
    {
        return ret;
    }

    // 请求超时，本地数据已经被删除了，或者不关心响应
    if (rspMsg == nullptr)
    {
        //GALOG_DEBUG("gorm, can not got request info for msg, reqid:%d", cbID);
        if(reqCmd == 2)
        {
            return HandShakeResult(preErrCode);
        }
        // 服务器将要升级
        if (reqCmd == 1001)
        {
            this->BeginToUpgrade();
            return GORM_OK;
        }
        if (needReadLen <= GORM_RSP_MSG_HEADER_LEN)
        {
            // 心跳
            // 为了不依赖于pb代码，这个地方使用数字
            if (reqCmd == 1)
            {
            }

            return GORM_OK;
        }
        return GORM_OK;
    }

    // 如果有错误则直接返回
    if (preErrCode != GORM_OK)
    {
        rspMsg->rspCode.code = preErrCode;
        return this->PutResponseToList(cbID, rspMsg);
    }

    rspMsg->reqCmd = reqCmd;
    // 解包
    ret = rspMsg->ParseRsp(this->beginReadPos+GORM_RSP_MSG_HEADER_LEN, needReadLen-GORM_RSP_MSG_HEADER_LEN);
    if(GORM_OK != ret)
    {
        rspMsg->rspCode.code = ret;
    }

    return this->PutResponseToList(cbID, rspMsg);
}

int GORM_ClientEvent::SendHandShakeMsg()
{
    // 准备被发送的消息
    int packMsgResult = GORM_OK;
    GORM_ClientMsg *handShakeMsg = GORM_GetHandShakeMessage(packMsgResult);
    if (handShakeMsg == nullptr || packMsgResult != GORM_OK)
    {
        GORM_CUSTOM_LOGE(logHandle, "begin to send hand shake message, pack message failed.");
        return GORM_ERROR;
    }
    this->PreSendRequest(handShakeMsg);
    
    this->m_pEpoll->AddEventWrite(this);
    return GORM_OK;
}

int GORM_ClientEvent::HandShakeResult(char preErrCode)
{
    auto workThread = dynamic_cast<GORM_ClientThread*>(this->myThread.get());
    if (workThread == nullptr){
        return GORM_ERROR;
    }
    if (preErrCode == GORM_OK)
    {
        workThread->SetStartStatus(GORM_ClientStartStatus_Success);
    }
    else
    {
        workThread->SetStartStatus(GORM_ClientStartStatus_Vesion_Failed);
    }
    
    return GORM_OK;
}

void GORM_ClientEvent::BeginToUpgrade()
{
    GALOG_INFO("gorm, get server upgrade cmd.");
    this->upgradeFlag = 1;
    this->upgradeMS = GORM_GetNowMS();
    // 开始不发送消息了
    this->DelWrite();
    this->myThread->BeginToUpgrade();
}


}
