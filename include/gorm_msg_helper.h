#ifndef _GROM_MSG_HELPER_H__
#define _GROM_MSG_HELPER_H__

#include "gorm_sys_inc.h"
#include "gorm_define.h"
#include "gorm_error.h"
#include "gorm_inc.h"
#include "gorm_mempool.h"
#include "gorm_log.h"
#include <google/protobuf/message.h>

namespace gorm{

using PB_MSG_PTR = google::protobuf::Message *;

#define GORM_MAX_DB_ERR_INFO 1024
struct GROM_ResponseCode
{
public:
    void Reset()
    {
        code = GORM_OK;
        dbError = 0;
        dbErrorInfo[0] = '\0';
    }
public:
    int code;               // 请求成功还是失败
    int dbError;            // 如果由于db原因导致的失败，则db测的错误码
    char dbErrorInfo[GORM_MAX_DB_ERR_INFO]; // db测错误的详细信息
};


//4字节长度 | 1字节操作类型 | 3字节请求编号 | 1字节flag | 请求内容
#define GORM_REQ_MSG_HEADER_LEN (4+1+3+1)
//4字节长度 | 3字节请求编号 | 1字节错误码 | 1字节flag | 响应内容
#define GORM_RSP_MSG_HEADER_LEN (4+3+1+1+1)

#define GORM_MAX_REQUEST_LEN (8*1024*1024)    // 请求的长度不能超过8M
#define GORM_MSG_LEN_BYTE   4               // 请求协议中代表长度的字节数


// 长度包括4个字节的长度本身
inline uint32 GORM_GetMsgLen(char *szMsg)
{
    return (uint32(szMsg[0])<<24 & 0xFF000000) | (uint32(szMsg[1])<<16 & 0xFF0000) | (uint32(szMsg[2])<<8 & 0xFF00) | (szMsg[3]&0xFF);
}

inline void GORM_SetMsgLen(char *szMsg, uint32 uiLen)
{
    szMsg[0] = uiLen>>24;
    szMsg[1] = uiLen>>16;
    szMsg[2] = uiLen>>8;
    szMsg[3] = uiLen;
}


inline uint32 GORM_GetReqType(char *szMsg)
{
    return uint32(szMsg[0])&0xFF;
}


inline uint32 GORM_GetReqID(char *szMsg)
{
    return (uint32(szMsg[0])<<16 & 0xFF0000) | (uint32(szMsg[1])<<8 & 0xFF00) | (uint32(szMsg[2])&0xFF);
}


// 返回跳过消息头的消息体地址
inline char* GORM_GetReqHeader(IN char *szMsg, OUT int &iReqCmd, OUT uint32 &iReqID, OUT uint8 &flag)
{
    iReqCmd = int(szMsg[0]);
    iReqID = (uint32(szMsg[1])<<16 & 0xFF0000) | (uint32(szMsg[2])<<8 & 0xFF00) | (uint32(szMsg[3])&0xFF);
    flag = uint8(szMsg[4]);

    return szMsg+5;
}


inline void GORM_SetReqHeaderSeqId(char *szMsg, uint32 uiSeqId)
{
    szMsg[5] = uiSeqId>>16;
    szMsg[6] = uiSeqId>>8;
    szMsg[7] = uiSeqId;
}


inline void GORM_SetReqHeader(IN char *szMsg, uint32 uiLen, int iReqCmd, uint32 iReqId, uint8 flag)
{
    szMsg[0] = char(uiLen>>24);
    szMsg[1] = char(uiLen>>16);
    szMsg[2] = char(uiLen>>8);
    szMsg[3] = char(uiLen);

    szMsg[4] = iReqCmd;

    szMsg[5] = iReqId>>16;
    szMsg[6] = iReqId>>8;
    szMsg[7] = iReqId;

    szMsg[8] = flag;
}


#define GORM_SET_REQ_PRE_HEADER(MSG,LEN,REQCMD,REQID,FLAG)\
GORM_SetReqHeader(MSG,LEN,REQCMD,REQID,FLAG)

inline void GORM_SetRspHeader(char *szMsg, uint32 uiLen, uint8 reqCmd, uint32 iReqId, char cErrCode, uint8 flag)
{
    szMsg[0] = char(uiLen>>24);
    szMsg[1] = char(uiLen>>16);
    szMsg[2] = char(uiLen>>8);
    szMsg[3] = char(uiLen);

    szMsg[4] = reqCmd;

    szMsg[5] = iReqId>>16;
    szMsg[6] = iReqId>>8;
    szMsg[7] = iReqId;

    szMsg[8] = cErrCode;

    szMsg[9] = flag;
}


inline char *GORM_GetRspHeader(char *szMsg, OUT int &reqCmd, OUT uint32 &iReqId, OUT char &cErrCode, OUT uint8 &flag)
{
    reqCmd = int(szMsg[0]);
    iReqId = (uint32(szMsg[1])<<16 & 0xFF0000) | (uint32(szMsg[2])<<8 & 0xFF00) | (szMsg[3]&0xFF);
    cErrCode = szMsg[4];
    flag = szMsg[5];
    return szMsg+6;
}


}
#endif

