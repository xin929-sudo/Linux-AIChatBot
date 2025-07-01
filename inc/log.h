#ifndef _LOG_H
#define _LOG_H
#include"./blocking_queue.h"
#include<pthread.h>
#include<fstream>
#include<string>
class Log {
    public:
        static Log *get_instance() {
            static Log instance;
            return &instance;
        }
        // 异步写入线程
        static void *flush_log_thread(void *args) {
            Log::get_instance()->async_write_log();
        }
        
        bool init(const char * file_name, int close_log , int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

        void write_log(int level, const char *format, ...);

        void flush(void);
    private:
        Log();
        ~Log();
        // 异步写入实现
        void *async_write_log() {
            std::string single_log;
            while (m_log_queue->pop(single_log))// 从阻塞队列中取日志
            {
                m_mutex.lock(); // 加锁保护文件操作
                fputs(single_log.c_str(),m_file); // 写入文件
                m_mutex.unlock(); // 解锁
            }
            
        }
        char dir_name[128]; // 路径名
        char log_name[128]; // Log 文件名
        long long m_count; // 日志行数记录
        int m_split_lines; // 日志最大行数
        int m_log_buf_size; // 日志缓冲区大小
        int m_today;        // 记录当前是哪天
        locker m_mutex;    //互斥锁
        char * m_buf;
        FILE * m_file; // 文件输入
        block_queue<std::string> *m_log_queue; // 阻塞队列
        bool m_is_async;  // 同步/异步模式设置
        bool m_close_log;  // 是否关闭log
};
// 0: DEBUG - 调试信息
// 1: INFO - 一般信息
// 2: WARN - 警告信息
// 3: ERROR - 错误信息
#define LOG_DEBUG(format, ...)  {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...)   {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...)   {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...)  {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}


#endif