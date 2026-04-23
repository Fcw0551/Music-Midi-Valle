
#include "../include/Fcw_HttpServer.hpp"
#include "../include/Fcw_Routes.hpp"
#include "../include/User_table.hpp"
#include "../include/Tasks_table.hpp"
#define HOST "127.0.0.1"
#define MYSQLPORT 3306
#define USER "wcf"
#define MYSQLPASSWORD "Aa1750551"
#define DBNAME "music_db"

#define REDISPORT 6379 
#define REDISPASSWORD "Aa1750551"
//mysql
user_table* g_user_db = nullptr;
task_table* g_task_db = nullptr;

//redis
redisContext *g_redis_ctx = nullptr;


int main() {
    g_redis_ctx = redis_util::create(HOST, REDISPORT, REDISPASSWORD);
    if (!g_redis_ctx)
    {
        DBG_LOG("Redis 连接失败");
    }
    user_table user_db(HOST, USER, MYSQLPASSWORD, DBNAME, MYSQLPORT);
    task_table task_db(HOST, USER, MYSQLPASSWORD, DBNAME, MYSQLPORT);
    g_user_db = &user_db;
    g_task_db = &task_db;
    HttpServer server(8080);
    server.setThreadCount(1);           //设置工作线程数量
    server.setBaseDir(WWWROOT);       // 静态资源根目录
    registerRoutes(server);             // 注册路由
    server.listen();
    return 0;
}