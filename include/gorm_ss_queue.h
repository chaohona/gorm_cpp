#ifndef _GORM_SS_QUEUE_H__
#define _GORM_SS_QUEUE_H__

// 单写单读(只支持一个线程写，一个线程读)，队列
// 使用原子操作，比锁方式快3倍
// 测试下来atomic操作比锁快6倍(不出现争抢锁的情况下)

#include "gorm_define.h"
#include "gorm_error.h"
#include "gorm_sys_inc.h"
#include "gorm_log.h"

template<class T, uint64 TOTALNUM>
class GORM_SSQueue
{
public:
    GORM_SSQueue():m_readIdx(0), m_writeIdx(0), m_nowNum(0)
    {
    }
    inline bool Put(const T& x)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == TOTALNUM)
            return false;
        int idx = this->m_writeIdx % TOTALNUM;
        this->m_data[idx] = x;
        ++this->m_writeIdx;
        ++this->m_nowNum;
        return true;
    }
    inline bool Put(const T& x, int64 &nowNum)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == TOTALNUM)
            return false;
        int idx = this->m_writeIdx % TOTALNUM;
        this->m_data[idx] = x;
        ++this->m_writeIdx;
        nowNum = ++this->m_nowNum;
        return true;
    }
    // 将元素放在开始
    inline bool PutPre(const T&x)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == TOTALNUM)
            return false;
        --this->m_readIdx;
        int idx = this->m_readIdx % TOTALNUM;
        this->m_data[idx] = x;
        ++this->m_nowNum;

        return true;
    }
    inline bool Take(T &out, int64 &leftNum)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == 0)
            return false;
        int idx = this->m_readIdx % TOTALNUM;
        out = m_data[idx];

        ++this->m_readIdx;
        leftNum = --this->m_nowNum;

        return true;
    }
    inline bool Take(T &out)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == 0)
            return false;
        int idx = this->m_readIdx % TOTALNUM;
        out = m_data[idx];

        ++this->m_readIdx;
        --this->m_nowNum;

        return true;
    }
    inline bool Take(T *outList, int inLen, int64 &num)
    {
        unique_lock<mutex> lck(this->mtx);
        if (m_nowNum == 0)
            return false;
        // TODO 改成拷贝的方式
        num = m_nowNum;
        int getNum = inLen;
        if (getNum > m_nowNum)
        {
            getNum = m_nowNum;
            m_nowNum = 0;
        }
        else
        {
            m_nowNum -= getNum;
        }
        for(int i=0; i<getNum; i++)
        {
            outList[i] = m_data[this->m_readIdx % TOTALNUM];
            ++this->m_readIdx;
        }

        return true;
    }
    inline bool Full()
    {
        unique_lock<mutex> lck(this->mtx);
        return m_nowNum == TOTALNUM;
    }
    inline bool Empty()
    {
        unique_lock<mutex> lck(this->mtx);
        return m_nowNum == 0;
    }
public:
    T m_data[TOTALNUM];
    uint64 m_readIdx;
    uint64 m_writeIdx;
    atomic<int64> m_nowNum;
    mutex           mtx; 
};



#endif
