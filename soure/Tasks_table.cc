#include "../include/Tasks_table.hpp"
#include <cassert>
#include <cstdio>
#include <algorithm>

// SQL 宏定义
#define INSERT_TASK "INSERT INTO tasks (task_id, user_id, midi_path, ref_midi_path, ref_audio_path, status) VALUES ('%s', '%s', '%s', '%s', '%s', 'pending')"
#define UPDATE_TASK_PROCESSING "UPDATE tasks SET status='processing' WHERE task_id='%s';"
#define UPDATE_TASK_COMPLETED "UPDATE tasks SET status='completed', wav_path='%s', finished_at=NOW() WHERE task_id='%s';"
#define UPDATE_TASK_FAILED "UPDATE tasks SET status='failed', error_msg='%s', finished_at=NOW() WHERE task_id='%s';"
#define SELECT_TASK_BY_ID "SELECT status, wav_path, error_msg FROM tasks WHERE task_id='%s';"
#define SELECT_TASKS_BY_USER "SELECT task_id, status, wav_path, created_at, finished_at FROM tasks WHERE user_id='%s' ORDER BY created_at DESC LIMIT %d OFFSET %d;"
#define SELECT_TASK_COUNT_BY_USER "SELECT COUNT(*) FROM tasks WHERE user_id='%s';"
// 查询单个任务详情
#define SELECT_TASK_BY_ID "SELECT task_id, user_id, status, wav_path, error_msg, created_at, finished_at FROM tasks WHERE task_id='%s'"

//super admin
// 查询指定用户所有任务的文件路径
#define SELECT_TASK_FILES_BY_USER "SELECT task_id, midi_path, ref_midi_path, ref_audio_path, wav_path FROM tasks WHERE user_id='%s'"

// 删除指定用户的所有任务记录
#define DELETE_TASKS_BY_USER "DELETE FROM tasks WHERE user_id='%s'"

#define COUNT_ALL_TASKS "SELECT COUNT(*) FROM tasks"
#define COUNT_TODAY_TASKS "SELECT COUNT(*) FROM tasks WHERE DATE(created_at) = CURDATE()"
#define COUNT_TASKS_BY_STATUS "SELECT status, COUNT(*) FROM tasks GROUP BY status"

#define COUNT_USER_TASK_STATS "SELECT COUNT(DISTINCT u.user_id) FROM users u LEFT JOIN tasks t ON u.user_id = t.user_id WHERE u.username LIKE '%%%s%%'"
#define SELECT_USER_TASK_STATS "SELECT u.user_id, u.username, COUNT(t.task_id) AS task_count FROM users u LEFT JOIN tasks t ON u.user_id = t.user_id WHERE u.username LIKE '%%%s%%' GROUP BY u.user_id ORDER BY task_count DESC LIMIT %d OFFSET %d"

task_table::task_table(const std::string &host, const std::string &user_name,
                       const std::string &password, const std::string &db_name, uint16_t port) {
    _mysql = mysql_util::create(host, user_name, password, db_name, port);
    assert(_mysql);
}

task_table::~task_table() {
    mysql_util::destroy(_mysql);
    _mysql = nullptr;
}


// 插入新任务（task 中需包含 task_id, user_id, midi_path, ref_midi_path, ref_audio_path）
//创建时间mysql自动处理了
bool task_table::insert(const Json::Value &task) {
    if (task["task_id"].isNull() || task["user_id"].isNull()) {
        DBG_LOG("INSERT: missing task_id or user_id");
        return false;
    }
    std::string task_id = task["task_id"].asString();
    std::string user_id = task["user_id"].asString();
    std::string midi_path = task.get("midi_path", "").asString();
    std::string ref_midi_path = task.get("ref_midi_path", "").asString();
    std::string ref_audio_path = task.get("ref_audio_path", "").asString();

    char sql_buffer[2048] = {0};
    int ret = snprintf(sql_buffer, sizeof(sql_buffer), INSERT_TASK,
                       task_id.c_str(), user_id.c_str(),
                       midi_path.c_str(), ref_midi_path.c_str(), ref_audio_path.c_str());
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

// 更新任务状态为 processing
bool task_table::set_processing(const std::string &task_id) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), UPDATE_TASK_PROCESSING, task_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    return mysql_util::exec(_mysql, sql_buffer);
}

// 更新任务状态为 completed，并记录生成的音频路径
bool task_table::set_completed(const std::string &task_id, const std::string &wav_path) {
    char sql_buffer[2048] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), UPDATE_TASK_COMPLETED, wav_path.c_str(), task_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    return mysql_util::exec(_mysql, sql_buffer);
}


// 更新任务状态为 failed，并记录错误信息
bool task_table::set_failed(const std::string &task_id, const std::string &error_msg) {
    // 转义单引号防止 SQL 注入
    std::string escaped = error_msg;
    size_t pos = 0;
    while ((pos = escaped.find("'", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\'");
        pos += 2;
    }
    char sql_buffer[2048] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), UPDATE_TASK_FAILED, escaped.c_str(), task_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    return mysql_util::exec(_mysql, sql_buffer);
}

// 查询任务状态（返回 JSON: status, audio_url, error_msg）
bool task_table::get_status(const std::string &task_id, Json::Value &result) {
    char sql_buffer[1024] = {0};
    snprintf(sql_buffer, sizeof(sql_buffer), SELECT_TASK_BY_ID, task_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    bool ret = mysql_util::exec(_mysql, sql_buffer);
    if (!ret) {
        DBG_LOG("GET STATUS: mysql exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET STATUS: no result");
        return false;
    }
    int row_num = mysql_num_rows(res);
    if (row_num != 1) {
        DBG_LOG("GET STATUS: task not found or multiple rows");
        mysql_free_result(res);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    result["status"] = row[0] ? row[0] : "";
    result["audio_url"] = row[1] ? Json::Value(row[1]) : Json::nullValue;
    result["error_msg"] = row[2] ? Json::Value(row[2]) : Json::nullValue;
    mysql_free_result(res);
    return true;
}

// 分页查询用户的所有任务（返回 JSON: total, page, limit, list）
bool task_table::get_user_tasks(const std::string &user_id, int page, int limit, Json::Value &result) {
    if (page < 1) page = 1;
    if (limit <= 0) limit = 10;
    if (limit > 50) limit = 50;
    int offset = (page - 1) * limit;

    // 查询总数
    char count_sql[1024] = {0};
    snprintf(count_sql, sizeof(count_sql), SELECT_TASK_COUNT_BY_USER, user_id.c_str());
    bool ret;{
        std::unique_lock<std::mutex> lock(_mutex);
        ret = mysql_util::exec(_mysql, count_sql);
    }
    if (!ret) {
        DBG_LOG("GET USER TASKS: count exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET USER TASKS: no count result");
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    int total = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);

    // 查询列表
    char list_sql[2048] = {0};
    snprintf(list_sql, sizeof(list_sql), SELECT_TASKS_BY_USER, user_id.c_str(), limit, offset);
    {
        std::unique_lock<std::mutex> lock(_mutex);
        ret = mysql_util::exec(_mysql, list_sql);
    }
    if (!ret) {
        DBG_LOG("GET USER TASKS: list exec error");
        return false;
    }
    res = mysql_store_result(_mysql);
    if (res == nullptr) {
        DBG_LOG("GET USER TASKS: no list result");
        return false;
    }
    Json::Value tasks(Json::arrayValue);
    int row_num = mysql_num_rows(res);
    for (int i = 0; i < row_num; ++i)
    {
        row = mysql_fetch_row(res);
        Json::Value item;
        item["task_id"] = row[0] ? row[0] : "";
        item["status"] = row[1] ? row[1] : "";
        item["audio_url"] = row[2] ? Json::Value(row[2]) : Json::nullValue;
        item["created_at"] = row[3] ? row[3] : "";
        item["finished_at"] = row[4] ? Json::Value(row[4]) : Json::nullValue;
        tasks.append(item);
    }
    mysql_free_result(res);

    result["total"] = total;
    result["page"] = page;
    result["limit"] = limit;
    result["list"] = tasks;
    return true;
}


//轮询查状态函数
bool task_table::get_task_by_id(const std::string &task_id, Json::Value &task) {
    char sql[512];
    snprintf(sql, sizeof(sql), SELECT_TASK_BY_ID, task_id.c_str());
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) return false;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    task["task_id"] = row[0] ? row[0] : "";
    task["user_id"] = row[1] ? row[1] : "";
    task["status"] = row[2] ? row[2] : "";
    task["wav_path"] = row[3] ? row[3] : "";
    task["error_msg"] = row[4] ? row[4] : "";
    task["created_at"] = row[5] ? row[5] : "";
    task["finished_at"] = row[6] ? row[6] : "";
    mysql_free_result(res);
    return true;
}








//-------------super admin---------------
bool task_table::delete_tasks_by_user(const std::string &user_id) {
    // 1. 查询该用户所有任务的文件路径
    char sql[512];
    snprintf(sql, sizeof(sql), SELECT_TASK_FILES_BY_USER, user_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("delete_tasks_by_user: query files exec error");
        return false;
    }

    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) {
        DBG_LOG("delete_tasks_by_user: store_result failed");
        return false;
    }

    // 遍历结果，删除磁盘上的文件
    int row_num = mysql_num_rows(res);
    for (int i = 0; i < row_num; ++i) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::string task_id = row[0] ? row[0] : "";

        // 删除各个文件
        if (row[1]) remove(row[1]); // midi_path
        if (row[2]) remove(row[2]); // ref_midi_path
        if (row[3]) remove(row[3]); // ref_audio_path
        if (row[4]) remove(row[4]); // wav_path

        // 尝试删除任务目录（可能为空）
        std::string task_dir = "../uploads/" + task_id;
        rmdir(task_dir.c_str());
    }
    mysql_free_result(res);

    // 2. 批量删除任务记录
    snprintf(sql, sizeof(sql), DELETE_TASKS_BY_USER, user_id.c_str());
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("delete_tasks_by_user: delete records exec error");
        return false;
    }

    return true;
}

// 统计所有任务数量
int task_table::count_all_tasks() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, COUNT_ALL_TASKS)) return 0;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    int count = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);
    return count;
}

// 统计今日新增任务数量
int task_table::count_today_tasks() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, COUNT_TODAY_TASKS)) return 0;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    int count = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);
    return count;
}

// 获取各状态任务数量（pending, processing, completed, failed）
bool task_table::get_status_counts(Json::Value &result) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, COUNT_TASKS_BY_STATUS)) return false;
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return false;
    
    int rows = mysql_num_rows(res);
    for (int i = 0; i < rows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::string status = row[0] ? row[0] : "unknown";
        int count = row[1] ? std::stoi(row[1]) : 0;
        result[status] = count;
    }
    mysql_free_result(res);
    return true;
}

// 获取每个用户的任务统计（分页、搜索）
bool task_table::get_user_task_stats(int page, int limit, const std::string &keyword, Json::Value &result) {
    if (page < 1) page = 1;
    if (limit <= 0) limit = 20;
    if (limit > 100) limit = 100;
    int offset = (page - 1) * limit;

    char escaped_kw[256];
    mysql_real_escape_string(_mysql, escaped_kw, keyword.c_str(), keyword.length());

    std::unique_lock<std::mutex> lock(_mutex);

    // 1. 查询总数
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql), COUNT_USER_TASK_STATS, escaped_kw);
    if (!mysql_util::exec(_mysql, count_sql)) {
        DBG_LOG("get_user_task_stats: count exec error");
        return false;
    }
    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    int total = row[0] ? std::stoi(row[0]) : 0;
    mysql_free_result(res);

    // 2. 查询列表
    char list_sql[1024];
    snprintf(list_sql, sizeof(list_sql), SELECT_USER_TASK_STATS, escaped_kw, limit, offset);
    if (!mysql_util::exec(_mysql, list_sql)) {
        DBG_LOG("get_user_task_stats: list exec error");
        return false;
    }
    res = mysql_store_result(_mysql);
    if (!res) return false;

    Json::Value list(Json::arrayValue);
    int row_num = mysql_num_rows(res);
    for (int i = 0; i < row_num; ++i) {
        row = mysql_fetch_row(res);
        Json::Value item;
        item["user_id"] = row[0] ? row[0] : "";
        item["username"] = row[1] ? row[1] : "";
        item["task_count"] = row[2] ? std::stoi(row[2]) : 0;
        list.append(item);
    }
    mysql_free_result(res);

    result["total"] = total;
    result["page"] = page;
    result["limit"] = limit;
    result["list"] = list;
    return true;
}


//管理员删除某个用户的某个任务
bool task_table::delete_single_task_admin(const std::string &task_id) {
    // 1. 查询该任务的所有文件路径
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT midi_path, ref_midi_path, ref_audio_path, wav_path FROM tasks WHERE task_id='%s'",
        task_id.c_str());

    std::unique_lock<std::mutex> lock(_mutex);
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("delete_single_task_admin: query error for %s", task_id.c_str());
        return false;
    }

    MYSQL_RES *res = mysql_store_result(_mysql);
    if (!res) return false;
    if (mysql_num_rows(res) == 0) {
        mysql_free_result(res);
        DBG_LOG("delete_single_task_admin: task %s not found", task_id.c_str());
        return false;   // 任务不存在
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    std::string midi_path = row[0] ? row[0] : "";
    std::string ref_midi_path = row[1] ? row[1] : "";
    std::string ref_audio_path = row[2] ? row[2] : "";
    std::string wav_path = row[3] ? row[3] : "";
    mysql_free_result(res);

    // 2. 删除磁盘文件（如果存在）
    if (!midi_path.empty()) remove(midi_path.c_str());
    if (!ref_midi_path.empty()) remove(ref_midi_path.c_str());
    if (!ref_audio_path.empty()) remove(ref_audio_path.c_str());
    if (!wav_path.empty()) remove(wav_path.c_str());

    // 尝试删除任务专属目录（uploads/task_id/），即使非空也会失败，但不影响主要功能
    std::string task_dir = "../uploads/" + task_id;
    rmdir(task_dir.c_str());

    // 3. 删除数据库记录
    snprintf(sql, sizeof(sql), "DELETE FROM tasks WHERE task_id='%s'", task_id.c_str());
    if (!mysql_util::exec(_mysql, sql)) {
        DBG_LOG("delete_single_task_admin: delete record error for %s", task_id.c_str());
        return false;
    }

    return true;
}