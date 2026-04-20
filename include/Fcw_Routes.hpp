#include <string>
#include <sstream>
#include <json/json.h>
#include "Fcw_HttpServer.hpp"
#include "Fcw_Http.hpp"
#include "Fcw_Util.hpp"
#include "Tasks_table.hpp"
#include "User_table.hpp"

extern user_table* g_user_db;
extern task_table* g_task_db;
extern redisContext *g_redis_ctx;


#define WWWROOT "../wwwroot/"

// 辅助函数：将请求内容转为字符串（用于调试响应）
static std::string RequestStr(const HttpRequest &req);

// 业务函数实现

// 注册所有路由
void registerRoutes(HttpServer &server);


// 注册函数
void handle_register(const HttpRequest &req, HttpResponse *rsp);
//登录函数
void handle_login(const HttpRequest &req, HttpResponse *rsp);
//供客户端获取当前登录用户的任务列表，并支持分页浏览。
void handle_get_tasks(const HttpRequest &req, HttpResponse *rsp);

//前端轮询查状态
void handle_get_task_status(const HttpRequest &req, HttpResponse *rsp);
// 用户上传头像
// POST /api/user/avatar
void handle_upload_avatar(const HttpRequest &req, HttpResponse *rsp);
//用户修改密码
void handle_change_password(const HttpRequest &req, HttpResponse *rsp);


//------------------超级管理员接口------------------
// 管理员进行分页展示接口   GET /admin/users?page=1&limit=20&keyword=xxx 
void handle_admin_get_users(const HttpRequest &req, HttpResponse *rsp);
// 管理员删除某个用户       DELETE /admin/users?user_id=xxx
void handle_admin_delete_user(const HttpRequest &req, HttpResponse *rsp);
// 管理员获取用户数据
void handle_admin_stats(const HttpRequest &req, HttpResponse *rsp);
// 管理员获取任务数据
void handle_admin_task_stats(const HttpRequest &req, HttpResponse *rsp);
//管理员获取某个用户的任务数据
void handle_admin_user_tasks(const HttpRequest &req, HttpResponse *rsp);
//管理员删除某个用户的某个任务
void handle_admin_delete_single_task(const HttpRequest &req, HttpResponse *rsp);
//重设密码
// POST /admin/user/set-password
void handle_admin_set_password(const HttpRequest &req, HttpResponse *rsp);




//客户推送任务
void handle_submit_task(const HttpRequest &req,  HttpResponse *rsp);
// 图片提取 MIDI 接口
void handle_convert_images(const HttpRequest &req, HttpResponse *rsp);