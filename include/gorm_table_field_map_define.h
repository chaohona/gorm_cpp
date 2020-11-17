#ifndef GORM_TABLE_FIELD_MAP_DEFINE_H__
#define GORM_TABLE_FIELD_MAP_DEFINE_H__

#include "gorm_sys_inc.h"
#include "gorm_inc.h"
#include "gorm_msg_helper.h"
#include "gorm_client_msg.h"

namespace gorm{
	int GORM_InitTableSchemaInfo(PB_MSG_PTR pMsgPtr);
	GORM_ClientMsg *GORM_GetHandShakeMessage(int &result);
}

#endif