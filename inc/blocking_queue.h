#ifndef _BLOCKING_QUEUE_H
#define _BLOCKING_QUEUE_H
#include"./locker.h"
#include<pthread.h>
#include<stdlib.h>
#include<sys/time.h>
template<class T>
class block_queue{
    public:
        // 初始化
        block_queue( int max_size = 1000) {
            if(max_size <= 0) {
                exit(-1);
            }
            m_max_size = max_size;
            m_array = new T[max_size];
            m_size = 0;
            m_front = -1;
            m_back = -1;
        }
        //析构函数
        ~block_queue() {
            m_mutex.lock();
            if( m_array != NULL) {
                delete [] m_array;
            }
            m_mutex.unlock();
        }
        // 清空
        void clear()
        {
            m_mutex.lock();
            m_size = 0;
            m_front = -1;
            m_back = -1;
            m_mutex.unlock();
        }    
        //判断队列是否满了
        bool full() 
        {
            m_mutex.lock();
            if (m_size >= m_max_size)
            {

                m_mutex.unlock();
                return true;
            }
            m_mutex.unlock();
            return false;
        }
        //判断队列是否为空
        bool empty() 
        {
            m_mutex.lock();
            if (0 == m_size)
            {
                m_mutex.unlock();
                return true;
            }
            m_mutex.unlock();
            return false;
        } 
        //取回队首元素
        bool front(T &value) 
        {
            m_mutex.lock();
            if (0 == m_size)
            {
                m_mutex.unlock();
                return false;
            }
            value = m_array[m_front];
            m_mutex.unlock();
            return true;
        }
        //取回队尾元素
        bool back(T &value) 
        {
            m_mutex.lock();
            if (0 == m_size)
            {
                m_mutex.unlock();
                return false;
            }
            value = m_array[m_back];
            m_mutex.unlock();
            return true;
        }
        // 任务数量
        int size() 
        {
            int tmp = 0;

            m_mutex.lock();
            tmp = m_size;

            m_mutex.unlock();
            return tmp;
        }
        // 最大任务数量
        int max_size()
        {
            int tmp = 0;

            m_mutex.lock();
            tmp = m_max_size;

            m_mutex.unlock();
            return tmp;
        }
        //往队列添加元素，需要将所有使用队列的线程先唤醒
        //当有元素push进队列,相当于生产者生产了一个元素
        //若当前没有线程等待条件变量,则唤醒无意义
        bool push(const T &item)
        {

            m_mutex.lock();
            if (m_size >= m_max_size) // 放不进去
            {

                m_cond.broadcast();
                m_mutex.unlock();
                return false;
            }
            // 还可以放
            m_back = (m_back + 1) % m_max_size;
            m_array[m_back] = item;

            m_size++;

            m_cond.broadcast();
            m_mutex.unlock();
            return true;
        }
        //pop时,如果当前队列没有元素,将会等待条件变量
        bool pop(T &item)
        {

            m_mutex.lock();
            while (m_size <= 0)
            {
                
                if (!m_cond.wait(m_mutex.get()))
                {
                    m_mutex.unlock();
                    return false;
                }
            }

            m_front = (m_front + 1) % m_max_size;
            item = m_array[m_front];
            m_size--;
            m_mutex.unlock();
            return true;
        }
        //增加了超时处理
        bool pop(T &item, int ms_timeout)
        {
            struct timespec t = {0, 0};
            struct timeval now = {0, 0};
            gettimeofday(&now, NULL);
            m_mutex.lock();
            if (m_size <= 0)
            {
                t.tv_sec = now.tv_sec + ms_timeout / 1000;
                t.tv_nsec = (ms_timeout % 1000) * 1000;
                if (!m_cond.timewait(m_mutex.get(), t))
                {
                    m_mutex.unlock();
                    return false;
                }
            }

            if (m_size <= 0)
            {
                m_mutex.unlock();
                return false;
            }

            m_front = (m_front + 1) % m_max_size;
            item = m_array[m_front];
            m_size--;
            m_mutex.unlock();
            return true;
        }
    private:
        locker m_mutex; // 互斥量，用于加锁操作
        cond m_cond; // 条件变量，避免忙等待

        T* m_array; // 存任务
        int m_size; // 当前任务数量
        int m_max_size; // 最大任务数量
        int m_front; // 头指针
        int m_back; // 尾指针

};



#endif