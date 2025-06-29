
#include"../inc/MysqlMgr.h"

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
    :url_(url),user_(user),pass_(pass),schema_(schema),poolSize_(poolSize),b_stop_(false)
{
    try{
        //创建连接池
        for(int i = 0; i < poolSize_; i++) {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
            auto* con = driver->connect(url_,user_,pass_);
            con->setSchema(schema_);
            //获得当前时间戳
            auto currentTime = std::chrono::system_clock::now().time_since_epoch();
            long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
            pool_.push(std::unique_ptr<SqlConnection>(new SqlConnection(con, timestamp)));
        }
        _check_thread = std::thread([this] {
			

            while (!b_stop_)
            {
		
				checkConnection();
				
                
                std::this_thread::sleep_for(std::chrono::seconds(60));
				
            }
            
        });
        _check_thread.detach();
    }
    catch (sql::SQLException& e) {
        //处理异常
        std::cout << "mysql pool init failed,error is " << e.what() << std::endl;
    }
}	

void MySqlPool::checkConnection(){
    std::lock_guard<std::mutex> guard(mutex_);
    int poolsize = pool_.size();
		// 获取当前时间戳
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		// 将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
			// Defer defer([this, &con]() {
				
			// });

			if (timestamp - con->_last_oper_time < 5) {
                pool_.push(std::move(con));
				continue;
			}
			
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
                pool_.push(std::move(con));
				//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// 重新创建连接并替换旧的连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp;
                pool_.push(std::move(con));
			}
		}
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection(){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this]{
        std::cout << pool_.size() << std::endl;
        if(b_stop_) return true;
        return !pool_.empty(); //池子里面有东西就返回
    });
    if(b_stop_) return nullptr;

    std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
    pool_.pop();
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con){
    std::unique_lock<std::mutex> lock(mutex_);
    if(b_stop_) return ;

    pool_.push(std::move(con));
    cond_.notify_one();
}

void MySqlPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
}

MySqlPool::~MySqlPool(){
    std::unique_lock<std::mutex> lock(mutex_);
    while(!pool_.empty()) {
        std::cout << "线程池 释放" << std::endl;
        pool_.pop();
    }
   
}

MysqlMgr::MysqlMgr(){

    m_pool.reset(new MySqlPool(Mysql_host + ":" + Mysql_port,Mysql_user,Mysql_passwd,Mysql_schema,5));

}

MysqlMgr::~MysqlMgr(){
    m_pool->Close();
    std::cout << "MysqlMgr destory" << std::endl;
}

bool MysqlMgr::InsertUser(const std::string& username, const std::string& password){
    auto con = m_pool->getConnection(); // 取连接
    if(con == nullptr) {
        return false;
    }
    try {

        //开始事务
		con->_con->setAutoCommit(false); 
        // 准备查询用户名是否重复
		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("SELECT 1 FROM user_table WHERE username = ?"));
        // 绑定参数
        pstmt_name->setString(1, username);
        
        // 执行查询
		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());
        auto name_exist = res_name->next();
		if (name_exist) {
			con->_con->rollback();
			std::cout << "name " << username << " exist" << std::endl; // 用户名存在
            m_pool->returnConnection(std::move(con)); // 还连接
			return false;
		}
       

        // 执行插入
        std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement( "INSERT INTO user_table (username, password) VALUES (?, ?)" ));
        // 绑定参数
        pstmt_insert->setString(1, username);
        pstmt_insert->setString(2, password);
        //执行插入
		pstmt_insert->executeUpdate();
		// 提交事务
		con->_con->commit();
		std::cout << "newuser insert into user success" << std::endl;
        std::cout << "Inserted user: " << username << std::endl;
       
        m_pool->returnConnection(std::move(con)); // 还连接
        return true;
    }
    catch (sql::SQLException& e) {
        m_pool->returnConnection(std::move(con)); // 还连接
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
 }

bool MysqlMgr::CheckUserPassword(const std::string& username,const std::string& password){
    auto con = m_pool->getConnection(); // 取连接
    if(con == nullptr) {
        return false;
    }
    try {

        // 准备查询用户名密码
		std::unique_ptr<sql::PreparedStatement> pstmt_pwd(con->_con->prepareStatement("SELECT password FROM user_table WHERE username = ?"));
        // 绑定参数
        pstmt_pwd->setString(1, username);
        
        // 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt_pwd->executeQuery());
        std::string origin_pwd = "";
        // 遍历结果集
        while (res->next())
        {
            origin_pwd = res->getString("password");
            // 输出查询到的密码
            std::cout << "Password:" << origin_pwd << std::endl;
            break;
        }
        m_pool->returnConnection(std::move(con)); // 还连接
        
        if(password != origin_pwd) {
            return false;
        }
        return true;
    }
    catch (sql::SQLException& e) {
        m_pool->returnConnection(std::move(con)); // 还连接
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
 }