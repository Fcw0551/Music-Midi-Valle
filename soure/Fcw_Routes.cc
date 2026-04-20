#include "../include/Fcw_Routes.hpp"
// 辅助函数：将请求内容转为字符串（用于调试响应）
static std::string RequestStr(const HttpRequest &req) {
    std::stringstream ss;
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &it : req._params) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}



// 业务函数实现


// 注册所有路由
void registerRoutes(HttpServer &server)
{
    server.Post("/api/login", handle_login);
    server.Post("/api/register", handle_register);
    server.Get("/api/tasks", handle_get_tasks);
    server.Post("/api/tasks", handle_submit_task);
    server.Get("/api/tasks/status", handle_get_task_status);
    server.Get("/admin/users", handle_admin_get_users);
    server.Delete("/admin/users", handle_admin_delete_user);
    server.Post("/api/convert/images", handle_convert_images);
    server.Get("/admin/stats", handle_admin_stats);
    server.Get("/admin/tasks/stats", handle_admin_task_stats);
    server.Get("/admin/users/tasks", handle_admin_user_tasks);
    server.Delete("/admin/task", handle_admin_delete_single_task);
    server.Post("/admin/user/set-password", handle_admin_set_password);
    server.Post("/api/user/avatar", handle_upload_avatar);
    server.Post("/api/user/change-password", handle_change_password);
}

//注册函数
void handle_register(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 解析请求体 JSON
    Json::Value req_json;
    Json::Reader reader;
    if (!reader.parse(req._body, req_json)) {
        route_util::send_json_response(rsp, 400, 2, "JSON 格式错误");
        return;
    }

    // 2. 提取用户名和密码
    if (req_json["username"].isNull() || req_json["password"].isNull()) {
        route_util::send_json_response(rsp, 400, 2, "缺少用户名或密码");
        return;
    }
    std::string username = req_json["username"].asString();
    std::string password = req_json["password"].asString();

    // 3. 基本校验
    if (username.empty() || password.empty()) {
        route_util::send_json_response(rsp, 400, 2, "用户名或密码不能为空");
        return;
    }
    if (password.length() < 6) {
        route_util::send_json_response(rsp, 400, 2, "密码长度至少6位");
        return;
    }

    // 4. 生成 user_id 和密码哈希
    std::string user_id = route_util::generate_uuid();
    std::string hashed_pwd = route_util::sha256(password);

    // 5. 准备用户数据
    Json::Value user;
    user["user_id"] = user_id;
    user["username"] = username;
    user["password"] = hashed_pwd;
    user["avatar"] = "";  // 默认空，后续可上传

    // 6. 尝试插入
    if (g_user_db->signup(user)) {
        // 注册成功
        Json::Value data;
        data["user_id"] = user_id;
        data["username"] = username;
        route_util::send_json_response(rsp, 201, 0, "注册成功", data);
    } else {
        // 插入失败，可能是用户名已存在
        route_util::send_json_response(rsp, 200, 1, "用户名已存在");
    }

}



//登录
void handle_login(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 解析请求体 JSON
    Json::Value req_json;
    Json::Reader reader;
    if (!reader.parse(req._body, req_json)) {
        route_util::send_json_response(rsp, 400, 2, "JSON 格式错误");
        return;
    }

    // 2. 验证必要字段是否存在
    if (!req_json.isMember("username") || !req_json.isMember("password")) {
        route_util::send_json_response(rsp, 400, 2, "缺少用户名或密码");
        return;
    }

    // 3. 提取并校验非空
    std::string username = req_json["username"].asString();
    std::string password = req_json["password"].asString();
    if (username.empty() || password.empty()) {
        route_util::send_json_response(rsp, 400, 2, "用户名或密码不能为空");
        return;
    }

    // 4. 准备用户对象用于数据库查询
    Json::Value user;
    user["username"] = username;
    user["password"] = route_util::sha256(password);   // 使用与注册相同的哈希

    // 5. 调用数据库验证（内部会填充 user_id 和 role）
    if (g_user_db->login(user)) {
        // 登录成功，从查询结果中获取 user_id 和 role
        std::string user_id = user["user_id"].asString();
        std::string role = user.get("role", "user").asString();
        std::string avatar = user.get("avatar", "").asString(); // 新增：获取头像字段

        std::string token = route_util::generate_token(user_id);
        Json::Value data;
        data["token"] = token;
        data["user_id"] = user_id;
        data["username"] = username;
        data["role"] = role;
        data["avatar"] = avatar; // 新增：返回头像 URL
        DBG_LOG("Login success: user_id=%s, avatar=%s", user_id.c_str(), avatar.c_str());
        route_util::send_json_response(rsp, 200, 0, "登录成功", data);
    } else {
        // 登录失败（用户名或密码错误）
        route_util::send_json_response(rsp, 200, 1, "用户名或密码错误");
    }
}




//供客户端获取当前登录用户的任务列表，并支持分页浏览
void handle_get_tasks(const HttpRequest &req, HttpResponse *rsp) {
    DBG_LOG("进入分页展示接口");
    
    //1.先验证token
    // 从 Authorization 头提取 token
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty()) return;  // 认证失败，响应已发送
 
    //2.------验证通过，返回任务状态-------

    // 获取分页参数
    int page = 1, limit = 10;                                               //默认采用一页10个数据  TODO
    auto it_page = req._params.find("page");
    auto it_limit = req._params.find("limit");
    if (it_page != req._params.end()) page = std::stoi(it_page->second);
    if (it_limit != req._params.end()) limit = std::stoi(it_limit->second);

    // 查询任务列表
    Json::Value result;
    if (g_task_db->get_user_tasks(user_id, page, limit, result)) {
        route_util::send_json_response(rsp, 200, 0, "success", result);
    } else {
        route_util::send_json_response(rsp, 500, 1, "查询失败");
    }
    DBG_LOG("完成分页展示接口响应");

}










//检查任务状态，轮询功能
void handle_get_task_status(const HttpRequest &req, HttpResponse *rsp)
{   
    DBG_LOG("进入轮询接口, task_id=%s", req.GetParam("task_id").c_str());
     
    // 1. 认证
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty())
        return;

    // 2. 获取查询参数 task_id
    if (!req.hasParam("task_id"))
    {
        route_util::send_json_response(rsp, 400, 2, "缺少 task_id");
        return;
    }
    std::string task_id = req.GetParam("task_id");
    if (task_id.empty())
    {
        route_util::send_json_response(rsp, 400, 2, "task_id 不能为空");
        return;
    }

    // 3. 从数据库查询任务详情
    Json::Value task;
    if (!g_task_db->get_task_by_id(task_id, task))
    {
        route_util::send_json_response(rsp, 404, 5, "任务不存在");
        return;
    }

    // 4. 权限校验：仅允许任务所有者查看
    if (task["user_id"].asString() != user_id)
    {
        route_util::send_json_response(rsp, 403, 4, "无权访问");
        return;
    }

    // 5. 构造响应数据
    Json::Value data;
    data["status"] = task["status"];
    data["error_msg"] = task["error_msg"];

    // 处理音频 URL：从 "../outputs/xxx.wav" 提取文件名，返回 "/outputs/xxx.wav"
    std::string wav_path = task["wav_path"].asString();
    if (!wav_path.empty()) {
        // 统一使用正斜杠处理
        size_t pos = wav_path.rfind('/');
        if (pos == std::string::npos) {
            pos = wav_path.rfind('\\');  // 兼容 Windows 反斜杠
        }
        std::string filename = (pos != std::string::npos) ? wav_path.substr(pos + 1) : wav_path;
        data["audio_url"] = "/outputs/" + filename;
    } else {
        data["audio_url"] = Json::nullValue;
    }

    route_util::send_json_response(rsp, 200, 0, "success", data);
}


// 用户上传头像
// POST /api/user/avatar
void handle_upload_avatar(const HttpRequest &req, HttpResponse *rsp) {
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty()) return;

    HttpRequest &mutable_req = const_cast<HttpRequest&>(req);
    if (!mutable_req.parseMultipart()) {
        route_util::send_json_response(rsp, 400, 2, "请求格式错误");
        return;
    }

    const auto& files = req.getMultipartFiles();
    auto it = files.find("avatar");
    if (it == files.end() || it->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少头像文件");
        return;
    }
    const auto& file = it->second;

    // 严格校验文件名
    if (file.filename.empty() || file.filename.find('.') == std::string::npos) {
        route_util::send_json_response(rsp, 400, 2, "无效的文件名");
        return;
    }

    std::string ext = route_util::get_file_extension(file.filename);
    if (ext != "jpg" && ext != "jpeg" && ext != "png" && ext != "gif") {
        route_util::send_json_response(rsp, 400, 2, "仅支持 jpg/png/gif");
        return;
    }

    // 头像存储目录（相对于 build/ 的路径，指向项目根目录下的 avatars/）
    const std::string AVATAR_DIR = "../avatars/";

    // 删除旧头像
    std::string old_url = g_user_db->get_avatar_path(user_id);
    if (!old_url.empty() && old_url.find("/avatars/") != std::string::npos) {
        size_t pos = old_url.rfind('/');
        if (pos != std::string::npos) {
            std::string old_filename = old_url.substr(pos + 1);
            std::string old_path = AVATAR_DIR + old_filename;
            remove(old_path.c_str());   // 删除旧文件，忽略失败
        }
    }

    // 生成新文件名：user_id_随机6位数.ext
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    std::string random_suffix = std::to_string(dis(gen));  // 加上分号
    std::string filename = user_id + "_" + random_suffix + "." + ext;
    std::string filepath = AVATAR_DIR + filename;
    if (!route_util::save_file(file.content, filepath)) {
        route_util::send_json_response(rsp, 500, 6, "文件保存失败");
        return;
    }

    // 更新数据库（存储相对 URL）
    std::string avatar_url = "/avatars/" + filename;
    if (!g_user_db->update_avatar(user_id, avatar_url)) {
        remove(filepath.c_str());
        route_util::send_json_response(rsp, 500, 6, "数据库更新失败");
        return;
    }

    Json::Value data;
    data["avatar_url"] = avatar_url;
    route_util::send_json_response(rsp, 200, 0, "上传成功", data);
}





//用户修改密码
// 用户修改密码
void handle_change_password(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 认证
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty()) return;

    // 2. 解析 JSON 请求体
    Json::Value req_json;
    Json::Reader reader;
    if (!reader.parse(req._body, req_json)) {
        route_util::send_json_response(rsp, 400, 2, "JSON 格式错误");
        return;
    }

    // 3. 提取字段
    if (!req_json.isMember("old_password") || !req_json.isMember("new_password")) {
        route_util::send_json_response(rsp, 400, 2, "缺少必要参数");
        return;
    }
    std::string old_pwd = req_json["old_password"].asString();
    std::string new_pwd = req_json["new_password"].asString();

    // 4. 新密码长度校验
    if (new_pwd.length() < 6) {
        route_util::send_json_response(rsp, 400, 2, "密码长度至少6位");
        return;
    }

    // 5. 验证旧密码
    if (!g_user_db->verify_password(user_id, old_pwd)) {
        route_util::send_json_response(rsp, 200, 1, "旧密码错误");
        return;
    }

    // 6. 更新密码
    if (!g_user_db->update_password(user_id, new_pwd)) {
        route_util::send_json_response(rsp, 500, 6, "更新失败");
        return;
    }

    // 7. 成功响应
    route_util::send_json_response(rsp, 200, 0, "修改成功");
}






//------------super admin-----------------

// GET /admin/users?page=1&limit=20&keyword=xxx
void handle_admin_get_users(const HttpRequest &req, HttpResponse *rsp) {
    if (!route_util::require_super_admin(req, rsp)) return;

    int page = 1, limit = 20;
    std::string keyword;
    auto it_page = req._params.find("page");
    if (it_page != req._params.end()) page = std::stoi(it_page->second);
    auto it_limit = req._params.find("limit");
    if (it_limit != req._params.end()) limit = std::stoi(it_limit->second);
    auto it_kw = req._params.find("keyword");
    if (it_kw != req._params.end()) keyword = it_kw->second;

    Json::Value result;
    if (g_user_db->get_all_users(page, limit, keyword, result)) {
        route_util::send_json_response(rsp, 200, 0, "success", result);
    } else {
        route_util::send_json_response(rsp, 500, 6, "查询失败");
    }
}

// DELETE /admin/users?user_id=xxx
void handle_admin_delete_user(const HttpRequest &req, HttpResponse *rsp) {
    std::string operator_id = route_util::authenticate(req, rsp);
    if (operator_id.empty()) return;
    if (!g_user_db->is_super_admin(operator_id)) {
        route_util::send_json_response(rsp, 403, 4, "需要超级管理员权限");
        return;
    }

    if (!req.hasParam("user_id")) {
        route_util::send_json_response(rsp, 400, 2, "缺少 user_id");
        return;
    }
    std::string target_id = req.GetParam("user_id");

    // 级联删除任务
    if (!g_task_db->delete_tasks_by_user(target_id)) {
        route_util::send_json_response(rsp, 500, 6, "删除用户任务失败");
        return;
    }

    if (g_user_db->delete_user_admin(target_id, operator_id)) {
        route_util::send_json_response(rsp, 200, 0, "删除成功");
    } else {
        route_util::send_json_response(rsp, 500, 6, "删除失败");
    }
}


//管理员获取用户数据
// GET /admin/stats
void handle_admin_stats(const HttpRequest &req, HttpResponse *rsp) {
    // 鉴权：仅超级管理员可访问
    if (!route_util::require_super_admin(req, rsp)) return;

    Json::Value stats;
    
    // 1. 总用户数
    stats["total_users"] = g_user_db->count_all_users();
    
    // 2. 总任务数
    stats["total_tasks"] = g_task_db->count_all_tasks();
    
    // 3. 今日新增任务数
    stats["today_tasks"] = g_task_db->count_today_tasks();
    
    // 4. 各状态任务数量（用于饼图）
    Json::Value status_counts;
    if (g_task_db->get_status_counts(status_counts)) {
        stats["status_counts"] = status_counts;
    } else {
        stats["status_counts"] = Json::objectValue;
    }

    route_util::send_json_response(rsp, 200, 0, "success", stats);
}

//管理员获取任务数据
// GET /admin/tasks/stats
void handle_admin_task_stats(const HttpRequest &req, HttpResponse *rsp) {
    if (!route_util::require_super_admin(req, rsp)) return;

    int page = 1, limit = 20;
    std::string keyword;
    auto it_page = req._params.find("page");
    if (it_page != req._params.end()) page = std::stoi(it_page->second);
    auto it_limit = req._params.find("limit");
    if (it_limit != req._params.end()) limit = std::stoi(it_limit->second);
    auto it_kw = req._params.find("keyword");
    if (it_kw != req._params.end()) keyword = it_kw->second;

    Json::Value result;
    if (g_task_db->get_user_task_stats(page, limit, keyword, result)) {
        route_util::send_json_response(rsp, 200, 0, "success", result);
    } else {
        route_util::send_json_response(rsp, 500, 6, "查询失败");
    }
}


// 管理员操作某个用户的任务
// GET /admin/users/tasks?user_id=xxx&page=1&limit=10
void handle_admin_user_tasks(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 超级管理员权限校验
    if (!route_util::require_super_admin(req, rsp)) return;

    // 2. 获取参数
    if (!req.hasParam("user_id")) {
        route_util::send_json_response(rsp, 400, 2, "缺少 user_id");
        return;
    }
    std::string target_user_id = req.GetParam("user_id");

    int page = 1, limit = 10;
    auto it_page = req._params.find("page");
    if (it_page != req._params.end()) page = std::stoi(it_page->second);
    auto it_limit = req._params.find("limit");
    if (it_limit != req._params.end()) limit = std::stoi(it_limit->second);

    // 3. 调用现有方法（不校验登录用户，因为上面已确保是超管）
    Json::Value result;
    if (g_task_db->get_user_tasks(target_user_id, page, limit, result)) {
        route_util::send_json_response(rsp, 200, 0, "success", result);
    } else {
        route_util::send_json_response(rsp, 500, 6, "查询失败");
    }
}





//管理员删除某个用户的某个任务
// DELETE /admin/task?task_id=xxx
void handle_admin_delete_single_task(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 超级管理员权限校验
    if (!route_util::require_super_admin(req, rsp)) return;

    // 2. 获取 task_id
    if (!req.hasParam("task_id")) {
        route_util::send_json_response(rsp, 400, 2, "缺少 task_id");
        return;
    }
    std::string task_id = req.GetParam("task_id");

    // 3. 执行删除
    if (g_task_db->delete_single_task_admin(task_id)) {
        route_util::send_json_response(rsp, 200, 0, "删除成功");
    } else {
        route_util::send_json_response(rsp, 500, 6, "删除失败，任务可能不存在");
    }
}



//重设密码
// POST /admin/user/set-password
void handle_admin_set_password(const HttpRequest &req, HttpResponse *rsp) {
    // 1. 超级管理员权限校验
    if (!route_util::require_super_admin(req, rsp)) return;

    // 2. 解析 JSON 请求体
    Json::Value req_json;
    Json::Reader reader;
    if (!reader.parse(req._body, req_json)) {
        route_util::send_json_response(rsp, 400, 2, "JSON 格式错误");
        return;
    }

    if (!req_json.isMember("user_id") || !req_json.isMember("new_password")) {
        route_util::send_json_response(rsp, 400, 2, "缺少必要参数");
        return;
    }
    std::string target_user_id = req_json["user_id"].asString();
    std::string new_password = req_json["new_password"].asString();


    // 3. 执行设置
    if (g_user_db->set_user_password_admin(target_user_id, new_password)) {
        route_util::send_json_response(rsp, 200, 0, "密码设置成功");
    } else {
        route_util::send_json_response(rsp, 500, 6, "设置失败（密码长度至少6位）");
    }
}










//----------提交任务接口--------------



// 公共任务提交逻辑：仅执行数据库插入和 Redis 推送
static bool submit_task_to_db_and_queue(const std::string &task_id,
                                        const std::string &user_id,
                                        const std::string &midi_path,
                                        const std::string &ref_midi_path,
                                        const std::string &ref_audio_path,
                                        std::string &error_msg) {
    // 1. 写入 MySQL 任务记录
    Json::Value task_data;
    task_data["task_id"] = task_id;
    task_data["user_id"] = user_id;
    task_data["midi_path"] = midi_path;
    // 修正：使用 Json::Value 包装 null 值
    task_data["ref_midi_path"] = ref_midi_path.empty() ? Json::Value(Json::nullValue) : Json::Value(ref_midi_path);
    task_data["ref_audio_path"] = ref_audio_path.empty() ? Json::Value(Json::nullValue) : Json::Value(ref_audio_path);
    task_data["status"] = "pending";
    task_data["audio_url"] = Json::Value(Json::nullValue);
    task_data["error_msg"] = Json::Value(Json::nullValue);

    if (!g_task_db->insert(task_data)) {
        error_msg = "任务写入数据库失败";
        return false;
    }

    // 2. 推入 Redis 队列
    if (g_redis_ctx) {
        if (!redis_util::lpush(g_redis_ctx, "music:task:queue", task_id)) {
            DBG_LOG("Redis 推送失败，任务 %s 将依赖补偿机制", task_id.c_str());
        }
    } else {
        DBG_LOG("Redis 连接不可用，任务 %s 未能入队", task_id.c_str());
    }
    return true;
}
/**
 * 处理任务提交请求
 * 路由: POST /api/tasks
 * 功能: 接收 MIDI 文件及可选参考文件，创建异步生成任务，返回 task_id
//  */
// void handle_submit_task(const HttpRequest &req,  HttpResponse *rsp) {
//     DBG_LOG("进入任务提交接口, task_id=%s", req.GetParam("task_id").c_str());

//     // ==================== 1. 用户认证 ====================
//     // 从 Authorization 头中提取并验证 Token，获取 user_id
//     std::string user_id = route_util::authenticate(req, rsp);
//     if (user_id.empty()) {
//         // 认证失败，authenticate 已内部发送 401 响应
//         return;
//     }

//     // ==================== 2. 解析 multipart 表单数据 ====================
//     // 由于 parseMultipart 会修改内部成员，需要 const_cast
//     HttpRequest &mutable_req = const_cast<HttpRequest&>(req);
//     if (!mutable_req.parseMultipart()) {
//         // 解析失败（格式错误或非 multipart 请求）
//         route_util::send_json_response(rsp, 400, 2, "请求格式错误，应为 multipart/form-data");
//         return;
//     }

//     const auto& files = req.getMultipartFiles();

//     // ==================== 3. 校验目标midi文件 ====================
//     // 检查 "midi" 字段是否存在且非空
//     auto it_midi = files.find("midi");
//     if (it_midi == files.end() || it_midi->second.content.empty()) {
//         route_util::send_json_response(rsp, 400, 2, "缺少MIDI文件");
//         return;
//     }
//     const auto& midi_file = it_midi->second;

//     // 验证 MIDI 文件扩展名（仅允许 .mid / .midi）
//     std::string midi_ext = route_util::get_file_extension(midi_file.filename);
//     if (midi_ext != "mid" && midi_ext != "midi") {
//         route_util::send_json_response(rsp, 400, 2, "MIDI 文件格式不支持，仅支持 .mid / .midi");
//         return;
//     }

//     // ==================== 4. 校验参考音色midi+参考音色wav文件 ====================
//     // 参考 MIDI
//     const MultipartFile* ref_midi_file = nullptr;
//     auto it_ref_midi = files.find("ref_midi");
//     if (it_ref_midi != files.end() && !it_ref_midi->second.content.empty()) {
//         ref_midi_file = &it_ref_midi->second;
//         std::string ext = route_util::get_file_extension(ref_midi_file->filename);
//         if (ext != "mid" && ext != "midi") {
//             route_util::send_json_response(rsp, 400, 2, "参考 MIDI 文件格式不支持，仅支持 .mid / .midi");
//             return;
//         }
//     }

//     // 参考音频
//     const MultipartFile* ref_audio_file = nullptr;
//     auto it_ref_audio = files.find("ref_audio");
//     if (it_ref_audio != files.end() && !it_ref_audio->second.content.empty()) {
//         ref_audio_file = &it_ref_audio->second;
//         std::string ext = route_util::get_file_extension(ref_audio_file->filename);
//         if (ext != "wav") {
//             route_util::send_json_response(rsp, 400, 2, "参考音频格式不支持，仅支持 .wav ");
//             return;
//         }
//     }

//     // ==================== 5. 生成任务 ID 并创建存储目录 ====================
//     std::string task_id = route_util::generate_task_id();  // 毫秒时间戳 + 6 位随机数
//     const std::string UPLOAD_BASE = "../uploads/";
//     std::string task_dir = UPLOAD_BASE + task_id;

//     //./uploads/task_id/
//     if (mkdir(task_dir.c_str(), 0755) != 0 && errno != EEXIST){
//         DBG_LOG("创建任务目录失败: %s (errno=%d)", task_dir.c_str(), errno);
//         route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
//         return;
//     }

//     // ==================== 6. 保存文件到磁盘 ====================
//     // 目标 MIDI（统一命名为 input.mid，方便 Python 推理服务定位）
//     //./uploads/task_id/input.mid  例如"./uploads/1700123456789_123456/input.mid"
//     std::string midi_path = task_dir + "/input.mid";
//     if (!route_util::save_file(midi_file.content, midi_path)) {
//         DBG_LOG("保存 MIDI 文件失败: %s", midi_path.c_str());
//         route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
//         return;
//     }

//     // 参考 MIDI
//     //./uploads/task_id/ref.mid
//     std::string ref_midi_path;
//     if (ref_midi_file) {
//         ref_midi_path = task_dir + "/ref.mid";
//         if (!route_util::save_file(ref_midi_file->content, ref_midi_path)) {
//             DBG_LOG("保存参考 MIDI 失败: %s", ref_midi_path.c_str());
//             route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
//             return;
//         }
//     }

//     // 参考音频
//     //./uploads/task_id/ref.wav
//     std::string ref_audio_path;
//     if (ref_audio_file) {
//         std::string ext = route_util::get_file_extension(ref_audio_file->filename);
//         ref_audio_path = task_dir + "/ref.wav" ;
//         if (!route_util::save_file(ref_audio_file->content, ref_audio_path)) {
//             DBG_LOG("保存参考音频失败: %s", ref_audio_path.c_str());
//             route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
//             return;
//         }
//     }
//     // ==================== 7. 写入 MySQL 任务记录 ====================
//     // 插入新任务（task 中需包含 task_id, user_id, midi_path, ref_midi_path, ref_audio_path）
//     Json::Value task_data;
//     task_data["task_id"] = task_id;
//     task_data["user_id"] = user_id;
//     task_data["midi_path"] = midi_path;
//     task_data["ref_midi_path"] =  ref_midi_path;
//     task_data["ref_audio_path"] = ref_audio_path;

//     if (!g_task_db->insert(task_data)){
//         DBG_LOG("任务写入数据库失败: task_id=%s", task_id.c_str());
//         route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
//         return;
//     }


//     // ==================== 8. 推入 Redis 任务队列 ====================
//     if (g_redis_ctx){
//         if (!redis_util::lpush(g_redis_ctx, "music:task:queue", task_id)){
//             DBG_LOG("Redis 推送失败，任务 %s 将依赖补偿机制", task_id.c_str());
//         }
//     }
//     else
//     {
//         DBG_LOG("Redis 连接不可用，任务 %s 未能入队", task_id.c_str());
//     }

//     // ==================== 9. 返回成功响应 ====================
//     Json::Value resp_data;
//     resp_data["task_id"] = task_id;
//     route_util::send_json_response(rsp, 202, 0, "任务已提交", resp_data);
// }


void handle_submit_task(const HttpRequest &req, HttpResponse *rsp) {
    DBG_LOG("进入任务提交接口");

    // 1. 认证
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty()) return;

    // 2. 解析 multipart
    HttpRequest &mutable_req = const_cast<HttpRequest&>(req);
    if (!mutable_req.parseMultipart()) {
        route_util::send_json_response(rsp, 400, 2, "请求格式错误");
        return;
    }
    const auto& files = req.getMultipartFiles();

    // 3. 校验目标 MIDI 文件
    auto it_midi = files.find("midi");
    if (it_midi == files.end() || it_midi->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少MIDI文件");
        return;
    }
    const auto& midi_file = it_midi->second;
    std::string midi_ext = route_util::get_file_extension(midi_file.filename);
    if (midi_ext != "mid" && midi_ext != "midi") {
        route_util::send_json_response(rsp, 400, 2, "MIDI 文件格式不支持，仅支持 .mid / .midi");
        return;
    }

    // 4. 校验参考 MIDI
    const MultipartFile* ref_midi_file = nullptr;
    auto it_ref_midi = files.find("ref_midi");
    if (it_ref_midi != files.end() && !it_ref_midi->second.content.empty()) {
        ref_midi_file = &it_ref_midi->second;
        std::string ext = route_util::get_file_extension(ref_midi_file->filename);
        if (ext != "mid" && ext != "midi") {
            route_util::send_json_response(rsp, 400, 2, "参考 MIDI 文件格式不支持，仅支持 .mid / .midi");
            return;
        }
    }

    // 5. 校验参考音频
    auto it_audio = files.find("ref_audio");
    if (it_audio == files.end() || it_audio->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少参考音频");
        return;
    }
    const auto& audio_file = it_audio->second;
    std::string audio_ext = route_util::get_file_extension(audio_file.filename);
    if (audio_ext != "wav") {
        route_util::send_json_response(rsp, 400, 2, "参考音频格式不支持，仅支持 .wav");
        return;
    }

    // 6. 生成任务 ID 并创建目录
    std::string task_id = route_util::generate_task_id();
    std::string task_dir = "../uploads/" + task_id;
    if (mkdir(task_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        DBG_LOG("创建任务目录失败: %s", task_dir.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 7. 保存文件
    // 目标 MIDI -> input.mid
    std::string midi_path = task_dir + "/input.mid";
    if (!route_util::save_file(midi_file.content, midi_path)) {
        DBG_LOG("保存 MIDI 文件失败: %s", midi_path.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 参考 MIDI-> ref.mid
    std::string ref_midi_path;
    if (ref_midi_file) {
        ref_midi_path = task_dir + "/ref.mid";
        if (!route_util::save_file(ref_midi_file->content, ref_midi_path)) {
            DBG_LOG("保存参考 MIDI 失败: %s", ref_midi_path.c_str());
            route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
            return;
        }
    }

    // 参考音频 -> ref.wav
    std::string ref_audio_path = task_dir + "/ref.wav";
    if (!route_util::save_file(audio_file.content, ref_audio_path)) {
        DBG_LOG("保存参考音频失败: %s", ref_audio_path.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 8. 调用公共函数完成数据库和 Redis
    std::string error_msg;
    if (!submit_task_to_db_and_queue(task_id, user_id, midi_path, ref_midi_path, ref_audio_path, error_msg)) {
        route_util::send_json_response(rsp, 500, 6, error_msg);
        return;
    }

    // 9. 返回成功
    Json::Value resp_data;
    resp_data["task_id"] = task_id;
    route_util::send_json_response(rsp, 202, 0, "任务已提交", resp_data);
}


// 调用外部 Python 脚本将图片转换为 MIDI
static bool convert_image_to_midi(const std::string &image_path,
                                  const std::string &output_midi_path,
                                  std::string &error_msg) {
    std::string cmd = "python3 /home/wboss/Fcw_muduo/scripts/convert_image.py \"" 
                    + image_path + "\" \"" + output_midi_path + "\"";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        error_msg = "无法执行转换脚本";
        return false;
    }
    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    int ret = pclose(pipe);
    if (ret != 0) {
        error_msg = output.empty() ? "转换脚本执行失败" : output;
        return false;
    }
    return true;
}





// 图片转 MIDI 接口
void handle_convert_images(const HttpRequest &req, HttpResponse *rsp) {
    DBG_LOG("进入图片转MIDI接口(异步)");
    // 1. 认证
    std::string user_id = route_util::authenticate(req, rsp);
    if (user_id.empty()) return;

    // 2. 解析 multipart
    HttpRequest &mutable_req = const_cast<HttpRequest&>(req);
    if (!mutable_req.parseMultipart()) {
        route_util::send_json_response(rsp, 400, 2, "请求格式错误");
        return;
    }
    const auto& files = req.getMultipartFiles();

    // 3. 检查目标图片
    auto it_target = files.find("target_image");
    if (it_target == files.end() || it_target->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少目标图片");
        return;
    }
    const auto& target_file = it_target->second;
    std::string target_ext = route_util::get_file_extension(target_file.filename);
    if (target_ext != "png" && target_ext != "jpg" && target_ext != "jpeg") {
        route_util::send_json_response(rsp, 400, 2, "目标图片格式不支持，仅支持 png/jpg/jpeg");
        return;
    }

    // 4. 检查参考图片
    auto it_ref = files.find("ref_image");
    if (it_ref == files.end() || it_ref->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少参考图片");
        return;
    }
    const auto& ref_file = it_ref->second;
    std::string ref_ext = route_util::get_file_extension(ref_file.filename);
    if (ref_ext != "png" && ref_ext != "jpg" && ref_ext != "jpeg") {
        route_util::send_json_response(rsp, 400, 2, "参考图片格式不支持，仅支持 png/jpg/jpeg");
        return;
    }

    // 5. 检查参考音频
    auto it_audio = files.find("ref_audio");
    if (it_audio == files.end() || it_audio->second.content.empty()) {
        route_util::send_json_response(rsp, 400, 2, "缺少参考音频");
        return;
    }
    const auto& audio_file = it_audio->second;
    std::string audio_ext = route_util::get_file_extension(audio_file.filename);
    if (audio_ext != "wav") {
        route_util::send_json_response(rsp, 400, 2, "参考音频格式不支持，仅支持 .wav");
        return;
    }

    // 6. 生成任务 ID 并创建目录
    std::string task_id = route_util::generate_task_id();
    std::string task_dir = "../uploads/" + task_id;
    if (mkdir(task_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        DBG_LOG("创建任务目录失败: %s", task_dir.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 7. 保存文件到任务目录
    std::string target_path = task_dir + "/target." + target_ext;
    if (!route_util::save_file(target_file.content, target_path)) {
        DBG_LOG("保存目标图片失败: %s", target_path.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    std::string ref_path = task_dir + "/ref." + ref_ext;
    if (!route_util::save_file(ref_file.content, ref_path)) {
        DBG_LOG("保存参考图片失败: %s", ref_path.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    std::string audio_path = task_dir + "/ref.wav";
    if (!route_util::save_file(audio_file.content, audio_path)) {
        DBG_LOG("保存参考音频失败: %s", audio_path.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 8.写入 MySQL（状态设为 pending_convert，MIDI 路径暂留空）
    Json::Value task_data;
    task_data["task_id"] = task_id;
    task_data["user_id"] = user_id;
    task_data["midi_path"] = Json::Value(Json::nullValue);
    task_data["ref_midi_path"] = Json::Value(Json::nullValue);
    task_data["ref_audio_path"] = audio_path;
    task_data["status"] = "pending";                        
    task_data["audio_url"] = Json::Value(Json::nullValue);
    task_data["error_msg"] = Json::Value(Json::nullValue);

    if (!g_task_db->insert(task_data)) {
        DBG_LOG("任务写入数据库失败: task_id=%s", task_id.c_str());
        route_util::send_json_response(rsp, 500, 6, "服务器内部错误");
        return;
    }

    // 9. 推入 Redis 转换队列（而非推理队列）
    if (g_redis_ctx) {
        if (!redis_util::lpush(g_redis_ctx, "music:convert:queue", task_id)) {
            DBG_LOG("Redis 推送失败，任务 %s 将依赖补偿机制", task_id.c_str());
        }
    } else {
        DBG_LOG("Redis 连接不可用，任务 %s 未能入队", task_id.c_str());
    }

    // 10. 返回成功
    Json::Value resp_data;
    resp_data["task_id"] = task_id;
    route_util::send_json_response(rsp, 202, 0, "任务已提交", resp_data);
    DBG_LOG("图片转MIDI任务已入队, task_id=%s", task_id.c_str());
}