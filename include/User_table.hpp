#pragma once 

#include "Fcw_Util.hpp"

#include <mutex>
#include <string>
#include <json/json.h>
#include <mysql/mysql.h>

// 唯一超级管理员的 user_id
static const std::string SUPER_ADMIN_ID = "a46d50b4-8794-4fc6-a7a6-bbd9982d49d3";  


class user_table {
public:
    user_table(const std::string &host, const std::string &user_name,
               const std::string &password, const std::string &db_name, uint16_t port = 3306);
    ~user_table();

    // 注册新用户（user 中应包含 user_id, username, password, 可选 avatar）
    bool signup(Json::Value &user);

    // 登录验证，成功时返回用户信息（user_id, username, avatar, created_at）
    bool login(Json::Value &user);

    // 通过用户名查询用户信息
    bool get_by_user_name(const std::string &user_name, Json::Value &user);

    // 通过 user_id 查询用户信息
    bool get_by_user_id(const std::string &user_id, Json::Value &user);

    //获取头像路径
    std::string get_avatar_path(const std::string &user_id);

    // 更新用户头像路径
    bool update_avatar(const std::string &user_id, const std::string &avatar_path);

    //验证用户是否存在
    bool user_exists(const std::string &user_id);

    // 验证用户密码是否正确（用于修改密码时校验旧密码）
    bool verify_password(const std::string &user_id, const std::string &password);
    
    // 更新用户密码（直接写入哈希后的密码）
    bool update_password(const std::string &user_id, const std::string &new_password);


//-------------------------------------super admin-----------------------------------------------


// 检查用户是否为超级管理员
bool is_super_admin(const std::string &user_id);

// 获取所有用户列表（分页，支持用户名搜索）—— 仅超管可调用
bool get_all_users(int page, int limit, const std::string &keyword, Json::Value &result);

// 删除用户（仅超管可调用，且不能删除唯一的超管）
bool delete_user_admin(const std::string &target_user_id, const std::string &operator_user_id);

//管理员获取用户数据
// 统计所有用户数量
int count_all_users();

// 管理员设置用户密码（手动指定新密码，返回是否成功）
bool set_user_password_admin(const std::string &user_id, const std::string &new_password);

private:
    MYSQL *_mysql;
    std::mutex _mutex;
};

