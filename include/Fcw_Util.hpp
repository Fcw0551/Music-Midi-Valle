#pragma once 
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <mysql/mysql.h>        //mysql
#include <json/json.h>
#include "Fcw_Log.hpp"
#include <chrono>
#include <random>
#include <hiredis/hiredis.h>    //redis
//#include "User_table.hpp"
//#include "Tasks_table.hpp"
class user_table;
class task_table;

extern user_table* g_user_db;
extern task_table* g_task_db;
extern redisContext *g_redis_ctx;


class HttpResponse;
class HttpRequest;
extern std::unordered_map<int, std::string> _statu_msg ;
class Util
{
public: 
    // 字符串分割函数,将src字符串按照sep字符进⾏分割，得到的各个字串放到arry中，最终返回字串的数量
    static size_t split(const std::string &src, const std::string &sep,std::vector<std::string> *arry);
    // 读取⽂件的所有内容，将读取的内容放到⼀个Buffer中
    static bool readFile(const std::string &filename, std::string *buf);
    // 向⽂件写⼊数据
    static bool writeFile(const std::string &filename, const std::string&buf);
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产⽣歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀% C++ -> C%2B%2B
    // 不编码的特殊字符： RFC3986⽂档规定 . - _ ~ 字⺟，数字属于绝对不编码字符
    // RFC3986⽂档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
    static std::string urlEncode(const std::string url, bool convert_space_to_plus);
    static char HEXTOI(char c);
    static std::string urlDecode(const std::string url, bool convert_plus_to_space);
    // 响应状态码的描述信息获取
    static std::string statuDesc(int statu);


    // 根据⽂件后缀名获取⽂件mime，设置文件类型
    static std::string extMime(const std::string &filename);
    // 判断⼀个⽂件是否是⼀个⽬录
    static bool isDirectory(const std::string &filename);
    static bool isRegular(const std::string &filename);
    // http请求的资源路径有效性判断
    //  /index.html --- 前边的/叫做相对根⽬录 映射的是某个服务器上的⼦⽬录
    //  想表达的意思就是，客⼾端只能请求相对根⽬录中的资源，其他地⽅的资源都不予理会
    //  /../login, 这个路径中的..会让路径的查找跑到相对根⽬录之外，这是不合理的，不安全的
    static bool validPath(const std::string &path);
};




class mysql_util {
public:
    // 创建并连接数据库，返回 MYSQL* 句柄
    static MYSQL* create(const std::string &host, const std::string &user_name,
                         const std::string &password, const std::string &db_name,
                         uint16_t port = 3306);
    // 关闭连接并释放句柄
    static void destroy(MYSQL *mysql);
    // 执行 SQL 语句（无结果集）
    static bool exec(MYSQL *mysql, const char *sql);
};













class redis_util {
public:
    // 连接到 Redis 服务器，返回上下文指针
    static redisContext* create(const std::string &host, int port, const std::string &password = "");
    
    // 断开连接并释放资源
    static void destroy(redisContext *ctx);
    
    // 向列表左侧推入一个元素（LPUSH）
    static bool lpush(redisContext *ctx, const std::string &key, const std::string &value);
    
    //从列表右侧阻塞弹出元素，供测试使用
    static std::string brpop(redisContext *ctx, const std::string &key, int timeout_sec = 0);
};











class route_util
{
public:
    //生成 UUID
    static std::string generate_uuid();
    //密码哈希,使用 SHA-256
    static std::string sha256(const std::string &str);
    //用于路由函数做响应构建的
    static void send_json_response(HttpResponse *rsp, int http_status, int biz_code,const std::string &msg, const Json::Value &data = Json::nullValue);
    //用于获取token，返回给前端做会话管理，默认1小时 TODO
    static std::string generate_token(const std::string &user_id, int expire_seconds = 3600);
    //验证token辅助函数
    static std::string verify_token_and_get_user_id(const std::string& token);
    //验证token是否合法
    static std::string authenticate(const HttpRequest &req, HttpResponse *rsp);
    // 生成唯一任务ID：毫秒时间戳 + 6位随机数
    static std::string generate_task_id();
    // 获取小写文件扩展名（不含点）
    static std::string get_file_extension(const std::string& filename);
    // 保存二进制内容到文件
    static bool save_file(const std::string& content, const std::string& path);
    //超级管理员中间件
    static bool require_super_admin(const HttpRequest &req, HttpResponse *rsp);
};