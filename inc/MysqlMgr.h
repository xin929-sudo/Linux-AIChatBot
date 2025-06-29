#ifndef _MYSQL_H
#define _MYSQL_H

#include<mysql-cppconn-8/mysql/jdbc.h>
#include<queue>
#include<string>
#include<mutex>
#include<condition_variable>
#include<thread>
#include<atomic>
#include"../inc/Singleton.h"
#include<memory>
#include <iostream>
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize);	
    void checkConnection();
	std::unique_ptr<SqlConnection> getConnection();
	void returnConnection(std::unique_ptr<SqlConnection> con); 
	
	void Close();
    ~MySqlPool(); 


private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolSize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
};

class MysqlMgr : public Singleton<MysqlMgr> {

    friend class Singleton<MysqlMgr>;
public:
    // bool CheckUsername(const std::string& username); // 检查用户是否存在
    bool InsertUser(const std::string& username, const std::string& password); // 插入用户
    bool CheckUserPassword(const std::string& username,const std::string& password); // 验证用户密码
    ~MysqlMgr();
    
private:
    const std::string Mysql_host = "172.18.0.3";
    const std::string Mysql_port = "3306";
    const std::string Mysql_user = "root";
    const std::string Mysql_passwd = "123456";
    const std::string Mysql_schema = "ai_chat";
    MysqlMgr();
	std::unique_ptr<MySqlPool> m_pool;
};

#endif