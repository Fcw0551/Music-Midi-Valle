#pragma once 

#include "Fcw_Util.hpp"
#include <mutex>
#include <string>
#include <json/json.h>
#include <mysql/mysql.h>
#include <unistd.h>      
#include <sys/stat.h>
class task_table {
public:
    task_table(const std::string &host, const std::string &user_name,
               const std::string &password, const std::string &db_name, uint16_t port = 3306);
    ~task_table();

    // 插入新任务（task 中需包含 task_id, user_id, midi_path, ref_midi_path, ref_audio_path）
    bool insert(const Json::Value &task);

    // 更新任务状态为 processing
    bool set_processing(const std::string &task_id);

    // 更新任务状态为 completed，并记录生成的音频路径
    bool set_completed(const std::string &task_id, const std::string &wav_path);

    // 更新任务状态为 failed，并记录错误信息
    bool set_failed(const std::string &task_id, const std::string &error_msg);

    // 查询任务状态（返回 JSON: status, audio_url, error_msg）
    bool get_status(const std::string &task_id, Json::Value &result);
    
   

    // 分页查询用户的所有任务（返回 JSON: total, page, limit, list）
    bool get_user_tasks(const std::string &user_id, int page, int limit, Json::Value &result);

    //轮询查状态
    bool get_task_by_id(const std::string &task_id, Json::Value &task);

    //超级管理员删除任务
    bool delete_tasks_by_user(const std::string &user_id);

    // 统计所有任务数量
    int count_all_tasks();
    
    // 统计今日新增任务数量
    int count_today_tasks();
    
    // 获取各状态任务数量（pending, processing, completed, failed）
    bool get_status_counts(Json::Value &result);

    // 获取每个用户的任务统计（分页、搜索）
    bool get_user_task_stats(int page, int limit, const std::string &keyword, Json::Value &result);

    // 删除单个任务（管理员专用，同时删除磁盘文件）
    bool delete_single_task_admin(const std::string &task_id);

private:
    MYSQL *_mysql;
    std::mutex _mutex;
};

