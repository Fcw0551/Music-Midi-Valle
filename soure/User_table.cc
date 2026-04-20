#include "../include/User_table.hpp"
#include <cassert>
#include <cstdio>

// SQL 宏定义
#define INSERT_USER "INSERT INTO users (user_id, username, password, avatar) VALUES ('%s', '%s', '%s', '%s');"
#define SELECT_USER "SELECT user_id, username, avatar, created_at FROM users WHERE username='%s' AND password='%s';"
#define SELECT_USER_BY_USER_NAME "SELECT user_id, username, avatar, created_at FROM users WHERE username='%s';"
#define SELECT_USER_BY_USER_ID "SELECT user_id, username, avatar, created_at FROM users WHERE user_id='%s';"
#define UPDATE_USER_AVATAR "UPDATE users SET avatar='%s' WHERE user_id='%s';"
#define SELECT_USER_LOGIN "SELECT user_id, role, avatar FROM users WHERE username='%s' AND password='%s'"
//管理员sql宏定义
#define SELECT_IS_SUPER_ADMIN "SELECT role FROM users WHERE user_id='%s' AND role='super_admin'"
#define SELECT_ALL_USERS_COUNT "SELECT COUNT(*) FROM users WHERE username LIKE '%%%s%%'"
#define SELECT_ALL_USERS_LIST "SELECT user_id, username, role, created_at FROM users WHERE username LIKE '%%%s%%' ORDER BY created_at DESC LIMIT %d OFFSET %d"
#define DELETE_USER_ADMIN "DELETE FROM users WHERE user_id='%s'"


//创建mysql句柄
user_table::user_table(const std::string &host, const std::string &user_name,
                       const std::string &password, const std::string &db_name, uint16_t port) {
    _mysql = mysql_util::create(host, user_name, password, db_name, port);
    assert(_mysql);
}
//释放mysql句柄
user_table::~user_table() {
    mysql_util::destroy(_mysql);
    _mysql = nullptr;
}


//用于register
bool user_table::signup(Json::Value &user) {
    if (user["user_id"].isNull() || user["username"].isNull() || user["password"].isNull()) {
        DBG_LOG("INSERT: user_id, username or password is null");
        return false;
    }
    std::string avatar = user.get("avatar", "").asString();
    char sql_buffer[2048] = {0};
    int ret = snprintf(sql_buffer, sizeof(sql_buffer), INSERT_USER,
                      user["user_id"].asCString(),
                      user["username"].asCString(),
                      user["password"].asCString(),
                      avatar.c_str());
    if (ret < 0) {
        DBG_LOG("INSERT: snprintf error");
        return false;
    }
    std::unique_lock<std::mutex> lock(_mutex);
    bool ok = mysql_util::exec(_mysql, sql_buffer);
    if (!ok) {
        DBG_LOG("INSERT: mysql exec error");
        return false;
    }
    return true;
}


//用户登录，查询是否有用户信息
bool user_table::login(Json::Value &user) {
    std::string username = user["username"].asString();
    std::string password = user["password"].asString();

    char sql[512];
    snprintf(sql, sizeof(sql), SELECT_USER_LOGIN, username.c_str(), password.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("login: exec error");
        return false;
    }

    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) {
        DBG_LOG("login: store_result failed");
        return false;
    }

    if (mysql_num_rows(res) == 0) {
        mysql_free_result(res);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    user["user_id"] = row[0] ? row[0] : "";
    user["role"]   = row[1] ? row[1] : "user";
    user["avatar"] = row[2] ? row[2] : "";   // 新增：填充头像路径

    mysql_free_result(res);
    return true;
}

// 通过用户名查询用户信息，然后填充到json当中
bool user_table::get_by_user_name(const std::string &user_name, Json::Value &user) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), SELECT_USER_BY_USER_NAME, user_name.c_str());
    DBG_LOG("执行SQL: %s", sql_buffer);

    std::unique_lock<std::mutex> lock(_mutex);
    bool ret = mysql_util::exec(_mysql, sql_buffer);
    if (!ret) {
        DBG_LOG("GET BY USER_NAME: mysql exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET BY USER_NAME: no result");
        return false;
    }
    int row_num = mysql_num_rows(res);
    if (row_num != 1) {
        DBG_LOG("GET BY USER_NAME: user not found or multiple rows");
        mysql_free_result(res);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    user["user_id"] = row[0] ? row[0] : "";
    user["username"] = row[1] ? row[1] : "";
    user["avatar"] = row[2] ? row[2] : "";
    user["created_at"] = row[3] ? row[3] : "";
    mysql_free_result(res);
    return true;
}


// 通过 user_id 查询用户信息
bool user_table::get_by_user_id(const std::string &user_id, Json::Value &user) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), SELECT_USER_BY_USER_ID, user_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    bool ret = mysql_util::exec(_mysql, sql_buffer);
    if (!ret) {
        DBG_LOG("GET BY USER_ID: mysql exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET BY USER_ID: no result");
        return false;
    }
    int row_num = mysql_num_rows(res);
    if (row_num != 1) {
        DBG_LOG("GET BY USER_ID: user not found or multiple rows");
        mysql_free_result(res);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    user["user_id"] = row[0] ? row[0] : "";
    user["username"] = row[1] ? row[1] : "";
    user["avatar"] = row[2] ? row[2] : "";
    user["created_at"] = row[3] ? row[3] : "";
    mysql_free_result(res);
    return true;
}


// 更新用户头像，给哪个user_id更新
bool user_table::update_avatar(const std::string &user_id, const std::string &avatar_path) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), UPDATE_USER_AVATAR, avatar_path.c_str(), user_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    return mysql_util::exec(_mysql, sql_buffer);
}


//获取头像路径
std::string user_table::get_avatar_path(const std::string &user_id) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), "SELECT avatar FROM users WHERE user_id='%s';", user_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql_buffer)) {
        DBG_LOG("GET AVATAR: mysql exec error");
        return "";
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET AVATAR: no result");
        return "";
    }
    int row_num = mysql_num_rows(res);
    if (row_num != 1) {
        DBG_LOG("GET AVATAR: user not found");
        mysql_free_result(res);
        return "";
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    std::string avatar = (row[0] && row[0][0] != '\0') ? row[0] : "";
    mysql_free_result(res);
    return avatar;
}




//验证用户密码是否正确（用于修改密码时校验旧密码）
bool user_table::verify_password(const std::string &user_id, const std::string &password) {
    std::string hashed = route_util::sha256(password);
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT 1 FROM users WHERE user_id='%s' AND password='%s'",
             user_id.c_str(), hashed.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) return false;
    MYSQL_RES *res = mysql_store_result(_mysql);
    bool valid = (res && mysql_num_rows(res) > 0);
    if (res) mysql_free_result(res);
    return valid;
}
//修改密码
bool user_table::update_password(const std::string &user_id, const std::string &new_password) {
    if (new_password.length() < 6) return false;
    std::string hashed = route_util::sha256(new_password);
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET password='%s' WHERE user_id='%s'",
             hashed.c_str(), user_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    return mysql_util::exec(_mysql, sql);
}





//鉴定某个用户是否存在， 后续超级管理员删除之后，需要检查
bool user_table::user_exists(const std::string &user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT 1 FROM users WHERE user_id='%s'", user_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) return false;
    MYSQL_RES *res = mysql_store_result(_mysql);
    bool exists = (res && mysql_num_rows(res) > 0);
    if (res) mysql_free_result(res);
    return exists;
}












//---------super admin------------------

// ---------- 检查是否为超级管理员 ----------
bool user_table::is_super_admin(const std::string &user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), SELECT_IS_SUPER_ADMIN, user_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) return false;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return false;
    bool is_super = (mysql_num_rows(res) > 0);
    mysql_free_result(res);
    return is_super;
}

// ---------- 获取所有用户（分页，搜索）----------
bool user_table::get_all_users(int page, int limit, const std::string &keyword, Json::Value &result) {
    if (page < 1) page = 1;
    if (limit <= 0) limit = 20;
    if (limit > 100) limit = 100;
    int offset = (page - 1) * limit;
    
    char escaped_kw[256];
    mysql_real_escape_string(_mysql, escaped_kw, keyword.c_str(), keyword.length());
    
    // 总数查询
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql), SELECT_ALL_USERS_COUNT, escaped_kw);
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, count_sql)) {
        DBG_LOG("get_all_users: count exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    int total = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);
    
    // 列表查询
    char list_sql[1024];
    snprintf(list_sql, sizeof(list_sql), SELECT_ALL_USERS_LIST, escaped_kw, limit, offset);
    if (!mysql_util::exec(_mysql, list_sql)) {
        DBG_LOG("get_all_users: list exec error");
        return false;
    }
    res = mysql_store_result(_mysql);
    if (!res) return false;
    
    Json::Value users(Json::arrayValue);
    int row_num = mysql_num_rows(res);
    for (int i = 0; i < row_num; ++i) {
        row = mysql_fetch_row(res);
        Json::Value item;
        item["user_id"]    = row[0] ? row[0] : "";
        item["username"]   = row[1] ? row[1] : "";
        item["role"]       = row[2] ? row[2] : "user";
        item["created_at"] = row[3] ? row[3] : "";
        users.append(item);
    }
    mysql_free_result(res);
    
    result["total"] = total;
    result["page"]  = page;
    result["limit"] = limit;
    result["list"]  = users;
    return true;
}

// ---------- 删除用户（管理员专用）----------
bool user_table::delete_user_admin(const std::string &target_user_id, const std::string &operator_user_id) {
    // 1. 操作者必须是超级管理员
    if (!is_super_admin(operator_user_id)) {
        DBG_LOG("delete_user_admin: operator not super admin");
        return false;
    }
    
    // 2. 禁止删除自己
    if (target_user_id == operator_user_id) {
        DBG_LOG("delete_user_admin: cannot delete yourself");
        return false;
    }
    
    // 3. 禁止删除唯一的超级管理员
    if (target_user_id == SUPER_ADMIN_ID) {
        DBG_LOG("delete_user_admin: cannot delete the unique super admin");
        return false;
    }
    
    // 4. 执行删除
    char sql[256];
    snprintf(sql, sizeof(sql), DELETE_USER_ADMIN, target_user_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("delete_user_admin: exec error for %s", target_user_id.c_str());
        return false;
    }
    return true;
}

//获取用户总数
int user_table::count_all_users() {
    const char* sql = "SELECT COUNT(*) FROM users";
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) return 0;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    int count = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);
    return count;
}

//给用户重新设置密码
bool user_table::set_user_password_admin(const std::string &user_id, const std::string &new_password) {
    // 密码长度校验（与注册时一致，至少6位）
    if (new_password.length() < 6) {
        DBG_LOG("set_user_password_admin: password too short");
        return false;
    }

    std::string hashed = route_util::sha256(new_password);

    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET password='%s' WHERE user_id='%s'",
             hashed.c_str(), user_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("set_user_password_admin: update failed for user_id=%s", user_id.c_str());
        return false;
    }
    return true;
}