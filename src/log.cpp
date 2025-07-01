#include"../inc/log.h"
#include<time.h>
#include <cstring>
#include <stdarg.h>
Log::Log() {
    m_count = 0;          // 日志行数计数器初始化为0
    m_is_async = false;   // 默认同步模式
    m_buf = nullptr;      // 缓冲区指针初始化为空
}
Log::~Log() {
    if(m_file != NULL) {
        fclose(m_file); // 关闭文件
    }
    delete [] m_buf; // 释放缓冲区
}
bool Log::init(const char * file_name, int close_log , int log_buf_size, int split_lines, int max_queue_size){
    // 异步模式初始化
    if(max_queue_size > 0) {
        m_is_async = true;
        m_log_queue = new block_queue<std::string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid,NULL,flush_log_thread,NULL);
    }
    // 基本参数设置
    time_t t= time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    
    const char *p = strrchr(file_name,'/'); // 查找最后一个'/'
    char log_full_name[256] = {0};

    if(p == NULL) {
        // 没有路径，直接在当前目录
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", 
                my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {
        // 有路径，分离目录和文件名
        strcpy(log_name, p + 1);                    // 提取文件名
        strncpy(dir_name, file_name, p - file_name + 1);  // 提取目录
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", 
                dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }
    
    m_today = my_tm.tm_mday;

    m_file = fopen(log_full_name, "a");
    if (m_file == NULL) {
        return false;
    }
    return true;
}


void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    m_count++;

    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256];
        fclose(m_file);
        char tail[16]  {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_file = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    std::string log_str;
    m_mutex.lock();

    //写入的具体时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    if (m_is_async && !m_log_queue->full()) { // 异步模式下放入阻塞队列，线程处理IO
        m_log_queue->push(log_str);
    }
    else { // 同步模式直接在主线程完成IO
        m_mutex.lock();
        fputs(log_str.c_str(), m_file);
        m_mutex.unlock();
    }
    va_end(valst);
}

void Log::flush() {
    m_mutex.lock();
    fflush(m_file);
    m_mutex.unlock();
}
