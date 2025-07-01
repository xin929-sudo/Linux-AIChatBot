#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"
#include "timer.h"
// #include "MysqlMgr.h"
#include "chatbot.h"

#define MAX_FD 65536            // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000  // 监听的最大的事件数量
const int TIMESLOT = 10;         //最小超时单位

class webserver
{
private:
    bool Initthreadpool();
    bool InitSocket();
    bool Initepoll();
    bool Inittimer();
    bool InitLoger();
    void trig_mode();
    bool AddClient();
    void deal_timer(util_timer *timer, int sockfd);
    void adjust_timer(util_timer *timer);
    bool dealwithsignal(bool &timeout, bool &stop_server);

    http_conn* users = nullptr;
    int m_log_write;
    int m_close_log;

    // 线程池
    threadpool< http_conn >* pool = nullptr;

    //触发模式
    int m_TrigMode;
    int m_ListenTrigMode;
    int m_ConntTrigMode;


    //定时器
    client_data *users_timer=nullptr; // 用户管理timer，方便在timer_utils里面更新。
    Timer_Utils timer_utils;

private:
    int port;
    int listenfd;
    int epollfd;
    bool stop_server;
    bool timeout;
    int m_pipefd[2];
    epoll_event events[ MAX_EVENT_NUMBER ];

    
    

public:
    webserver(int port,int LOGWrite,int TrigMode);
    ~webserver();
    void start();
};
#endif