#include "gorm.h"
#include "gorm_utils.h"

using namespace gorm;

int main()
{
	// 初始化gorm驱动
	if (0 != GORM_Wrap::Instance()->Init((char*)"gorm-client.yml"))
	{
		cout << "gorm init failed" << endl;
		return -1;
	}

	ThreadSleepSeconds(1);


	int iRet = 0;

	vector<GORM_ClientTableCurrency*> vectorResult = GORM_ClientTableCurrency::GetVector(iRet, 1605015790);
	cout << vectorResult.size() << endl;
	return 0;
/*
	for (int i=0; i<100000; i++)
{
	GORM_ClientTablePtuser tablePtUser;	
	GORM_PB_Table_ptuser pbPtUser;
	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 1:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 2:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 3:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 4:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 5:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 6:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 7:"<< iRet << endl;

	pbPtUser.set_ptid("123456");
	pbPtUser.set_pttype(123);	
	tablePtUser.SetPbMsg(&pbPtUser);
	iRet = tablePtUser.SaveToDB();
	cout << "Result 8:"<< iRet << endl;
}
	return 0;

	auto result1 = GORM_ClientTableUser::Get(iRet, 123);
	cout << "Result 3:"<< iRet << endl;


	GORM_ClientTableUser tableUser;
	GORM_PB_Table_user pbUser;
	pbUser.set_userid(123);
	tableUser.SetPbMsg(&pbUser);
	iRet = tableUser.SaveToDB();
	cout << "Result 4:" << iRet << endl;

	GORM_ClientTableUser tableUser1;
	pbUser.set_userid(123456);
	tableUser1.SetPbMsg(&pbUser);
	iRet = tableUser1.SaveToDB();
	cout << "Result 5:"<< iRet << endl;

	auto result = GORM_ClientTableUser::Get(iRet, 123);
	cout << "Result 6:"<< iRet << endl;
*/
	return 0;
}
