#include "../include/Fcw_Util.hpp"
#include "../include/Fcw_Http.hpp"
#include <openssl/hmac.h>
#include "../include/User_table.hpp"        
#include "../include/Tasks_table.hpp"      
extern user_table* g_user_db;
extern task_table* g_task_db;



std::unordered_map<int, std::string> _statu_msg  = {
    {100, "Continue"},
    {101, "Switching Protocol"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choice"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};
std::unordered_map<std::string, std::string> _mime_msg = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}};



    // 字符串分割函数,将src字符串按照sep字符进⾏分割，得到的各个字串放到arry中，最终返回字串的数量
    size_t Util::split(const std::string &src, const std::string &sep,std::vector<std::string> *arry){
        size_t offset = 0;
        // 有10个字符，offset是查找的起始位置，范围应该是0~9，offset==10就代表已经越界了
        while (offset < src.size())
        {
            size_t pos = src.find(sep, offset); // 在src字符串偏移量offset处，开始向后查找sep字符 / 字串，返回查找到的位置 
            if(pos==std::string::npos){
                //说明没有分隔符,把剩余的字串加入到数组中
                if (pos == src.size())
                    break;
                arry->push_back(src.substr(offset));
                return arry->size();
            }
            if(pos==offset){
                //说明刚好开始切割的位置有sep分隔符
                //,,b,,c,,d
                offset=offset+sep.size();
                continue;//继续分隔
            }
            //剩下的就是pos和offest的位置不一样了
            //abc,,abc
            arry->push_back(src.substr(offset,pos-offset));
            offset=pos+sep.size();//更新offset位置
        }
        return arry->size();
    }
    // 读取⽂件的所有内容，将读取的内容放到⼀个Buffer中
    bool Util::readFile(const std::string &filename, std::string *buf){
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false){
            printf("OPEN %s FILE FAILED!!", filename.c_str());
            return false;
        }
        size_t fsize = 0;
        ifs.seekg(0, ifs.end); // 跳转读写位置到末尾
        fsize = ifs.tellg();   // 获取当前读写位置相对于起始位置的偏移量，从末尾偏移刚好就是⽂件⼤⼩
        ifs.seekg(0, ifs.beg); // 跳转到起始位置
        buf->resize(fsize);    // 开辟⽂件⼤⼩的空间
        ifs.read(&(*buf)[0], fsize);
        if (ifs.good() == false)
        {
            ERR_LOG("READ %s FILE FAILED!!", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }
    // 向⽂件写⼊数据
    bool Util::writeFile(const std::string &filename, const std::string&buf){
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        if (ofs.is_open() == false)
        {
            ERR_LOG("OPEN %s FILE FAILED!!", filename.c_str());
            return false;
        }
        ofs.write(buf.c_str(), buf.size());
        if (ofs.good() == false)
        {
            ERR_LOG("WRITE %s FILE FAILED!", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产⽣歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀% C++ -> C%2B%2B
    // 不编码的特殊字符： RFC3986⽂档规定 . - _ ~ 字⺟，数字属于绝对不编码字符
    // RFC3986⽂档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
    std::string Util::urlEncode(const std::string url, bool convert_space_to_plus){
        std::string res;
        for (auto &c : url)
        {
            if (c == '.' || c == '-' || c == '_' || c == '~' ||
                isalnum(c))
            {
                res += c;
                continue;
            }
            if (c == ' ' && convert_space_to_plus == true)
            {
                res += '+';
                continue;
            }
            // 剩下的字符都是需要编码成为 %HH 格式
            char tmp[4] = {0};
            // snprintf 与 printf⽐较类似，都是格式化字符串，只不过⼀个是打印，⼀个是放到⼀块空间中
            snprintf(tmp, 4, "%%%02X", c);
            res += tmp;
        }
        return res;
    }
    char Util::HEXTOI(char c){
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'z')
        {
            return c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'Z')
        {
            return c - 'A' + 10;
        }
        return -1;
    }
    std::string Util::urlDecode(const std::string url, bool convert_plus_to_space){
        // 遇到了%，则将紧随其后的2个字符，转换为数字，第⼀个数字左移4位，然后加上第⼆个数字 +->2b % 2b->2 << 4 + 11 
        std::string res;
        for (int i = 0; i < url.size(); i++){
            if (url[i] == '+' && convert_plus_to_space == true){
                res += ' ';
                continue;
            }
            if (url[i] == '%' && (i + 2) < url.size()){
                char v1 = HEXTOI(url[i + 1]);
                char v2 = HEXTOI(url[i + 2]);
                char v = v1 * 16 + v2;
                res += v;
                i += 2;
                continue;
            }
            res += url[i];
        }
        return res;
    }
    // 响应状态码的描述信息获取
    std::string Util::statuDesc(int statu)
    {

        auto it = _statu_msg.find(statu);
        if (it != _statu_msg.end())
        {
            return it->second;
        }
        return "Unknow";
    }


    // 根据⽂件后缀名获取⽂件mime，设置文件类型
    std::string Util::extMime(const std::string &filename){

        // a.b.txt 先获取⽂件扩展名
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream";
        }
        // 根据扩展名，获取mime
        std::string ext = filename.substr(pos);
        auto it = _mime_msg.find(ext);
        if (it == _mime_msg.end())
        {
            return "application/octet-stream";
        }
        std::string mime=it->second;

        // 步骤3：统一处理：文本类 MIME 追加 ; charset=utf-8（核心新增逻辑）
        bool is_text_type =
            (mime.find("text/") == 0) ||
            (mime == "application/json") ||
            (mime == "application/javascript") ||
            (mime == "application/xml");

        if (is_text_type && mime.find("charset=") == std::string::npos){
            mime += "; charset=utf-8";
        }

        // 步骤4：返回处理后的 MIME
        return mime;
    }
    // 判断⼀个⽂件是否是⼀个⽬录
    bool Util::isDirectory(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }
    // 判断⼀个⽂件是否是⼀个普通⽂件
    bool Util::isRegular(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISREG(st.st_mode);
    }
    // http请求的资源路径有效性判断
    //  /index.html --- 前边的/叫做相对根⽬录 映射的是某个服务器上的⼦⽬录
    //  想表达的意思就是，客⼾端只能请求相对根⽬录中的资源，其他地⽅的资源都不予理会
    //  /../login, 这个路径中的..会让路径的查找跑到相对根⽬录之外，这是不合理的，不安全的
    bool Util::validPath(const std::string &path)
    {
        // 思想：按照/进⾏路径分割，根据有多少⼦⽬录，计算⽬录深度，有多少层，深度不能⼩于0
        std::vector<std::string>subdir;
        split(path, "/", &subdir);
        int level = 0;
        for (auto &dir : subdir)
        {
            if (dir == "..")
            {
                level--; // 任意⼀层⾛出相对根⽬录，就认为有问题
                if (level < 0)
                    return false;
                continue;
            }
            level++;
        }
        return true;
    }











//-----------------------------mysql_util------------------------
MYSQL* mysql_util::create(const std::string &host, const std::string &user_name,
                          const std::string &password, const std::string &db_name,
                          uint16_t port) {
    MYSQL *mysql = mysql_init(nullptr);
    if (!mysql) {
        DBG_LOG("mysql_init failed");
        return nullptr;
    }
    if (!mysql_real_connect(mysql, host.c_str(), user_name.c_str(), password.c_str(),
                            db_name.c_str(), port, nullptr, 0)) {
        DBG_LOG("mysql_real_connect failed: %s", mysql_error(mysql));
        mysql_close(mysql);
        return nullptr;
    }
    // 设置字符集为 utf8mb4，支持中文
    mysql_set_character_set(mysql, "utf8mb4");
    return mysql;
}

void mysql_util::destroy(MYSQL *mysql) {
    if (mysql) {
        mysql_close(mysql);
    }
}

bool mysql_util::exec(MYSQL *mysql, const char *sql) {
    if (!mysql) return false;
    if (mysql_real_query(mysql, sql, strlen(sql)) != 0) {
        DBG_LOG("MySQL exec error: %s", mysql_error(mysql));
        return false;
    }
    return true;
}





//----------------redis_util---------------
redisContext* redis_util::create(const std::string &host, int port, const std::string &password) {
    redisContext *ctx = redisConnect(host.c_str(), port);
    if (ctx == nullptr || ctx->err) {
        if (ctx) {
            DBG_LOG("Redis create error: %s", ctx->errstr);
        } else {
            DBG_LOG("Redis create error: can't allocate redis context");
        }
        return nullptr;
    }
    
    if (!password.empty()) {
        redisReply *reply = (redisReply*)redisCommand(ctx, "AUTH %s", password.c_str());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            DBG_LOG("Redis AUTH error: %s", reply ? reply->str : "unknown");
            if (reply) freeReplyObject(reply);
            redisFree(ctx);
            return nullptr;
        }
        freeReplyObject(reply);
    }
    
    return ctx;
}

void redis_util::destroy(redisContext *ctx) {
    if (ctx) {
        redisFree(ctx);
    }
}

bool redis_util::lpush(redisContext *ctx, const std::string &key, const std::string &value) {
    if (!ctx) return false;
    
    redisReply *reply = (redisReply*)redisCommand(ctx, "LPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        DBG_LOG("Redis LPUSH error: %s", reply ? reply->str : "unknown");
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}
//调试用
std::string redis_util::brpop(redisContext *ctx, const std::string &key, int timeout_sec) {
    if (!ctx) return "";
    
    redisReply *reply = (redisReply*)redisCommand(ctx, "BRPOP %s %d", key.c_str(), timeout_sec);
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string value;
    if (reply->elements >= 2 && reply->element[1]->type == REDIS_REPLY_STRING) {
        value = reply->element[1]->str;
    }
    freeReplyObject(reply);
    return value;
}





//----------------------route_util--------------------
// 生成 UUID
std::string route_util::generate_uuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[37];
    uuid_unparse(uuid, str);
    return std::string(str);
}
// 密码哈希,使用 SHA-256
std::string route_util::sha256(const std::string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    // 1. 初始化上下文
    SHA256_Init(&sha256);
    // 2. 输入数据
    SHA256_Update(&sha256, str.c_str(), str.size());
    // 3. 计算最终哈希值
    SHA256_Final(hash, &sha256);
    // 4. 将二进制哈希值转换成十六进制字符串
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
void route_util::send_json_response(HttpResponse *rsp, int http_status, int biz_code,
                        const std::string &msg, const Json::Value &data ) {
    rsp->setStatus(http_status);   // 设置 HTTP 状态码
    //设置body
    Json::Value res;
    res["code"] = biz_code;
    res["message"] = msg;
    if (!data.isNull()) {
        res["data"] = data;
    }
    Json::FastWriter writer;
    std::string body = writer.write(res);
    rsp->setContent(body, "application/json");
}

std::string route_util::generate_token(const std::string &user_id, int expire_seconds) {
    // 1. 获取当前时间戳（秒）
    time_t now = time(nullptr);
    // 2. 计算过期时间戳 = 当前时间 + 有效期（秒）
    time_t expire = now + expire_seconds;

    // 3. 构造需要签名的数据：user_id 和 expire 用冒号分隔
    std::string data = user_id + ":" + std::to_string(expire);

    // 4. HMAC 签名密钥（实际生产中应使用复杂的随机字符串，并从配置文件读取） TODO
    const char* secret = "7xRqL3pK9mN2vE5wY8uB1aC4dF6gH0jZ2aXcVbNmLkQwErTyUiOpAsDfGhJk";

    // 5. 准备存放签名结果的缓冲区
    unsigned char digest[EVP_MAX_MD_SIZE]; // 足够大的数组
    unsigned int len = 0;                  // 实际签名长度

    // 6. 初始化 HMAC 上下文（HMAC 是一种基于哈希函数的消息认证码，使用一个密钥（secret）对数据进行签名）
    HMAC_CTX *ctx = HMAC_CTX_new();
    // 设置 HMAC 算法、密钥
    HMAC_Init_ex(ctx, secret, strlen(secret), EVP_sha256(), nullptr);
    // 输入要签名的数据
    HMAC_Update(ctx, (unsigned char *)data.c_str(), data.size());
    // 计算最终签名，结果存入 digest，长度存入 len
    HMAC_Final(ctx, digest, &len);
    // 释放 HMAC 上下文
    HMAC_CTX_free(ctx);

    // 7. 将二进制签名转换为十六进制字符串（便于传输和存储）
    std::stringstream ss;
    for (unsigned int i = 0; i < len; ++i)
    {
        // std::hex 输出十六进制，std::setw(2) 宽度2，std::setfill('0') 不足补0
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    std::string signature = ss.str();

    // 8. 组合最终 token：数据部分 + 冒号 + 签名
    return data + ":" + signature;
}




std::string route_util::verify_token_and_get_user_id(const std::string& token) {
    // 1. 复用 Util::split 分割 token
    std::vector<std::string> parts;
    Util::split(token, ":", &parts);
    if (parts.size() != 3) {
        return "";  // 格式错误
    }

    std::string user_id = parts[0];
    std::string expire_str = parts[1];
    std::string received_signature = parts[2];

    // 2. 检查过期时间
    time_t expire_time;
    try {
        expire_time = std::stoll(expire_str);
    } catch (...) {
        return "";  // 过期时间不是合法数字
    }

    time_t now = time(nullptr);
    if (now > expire_time) {
        return "";  // Token 已过期
    }

    // 3. 重新计算签名（与 generate_token 相同的密钥和算法）
    const char* secret = "7xRqL3pK9mN2vE5wY8uB1aC4dF6gH0jZ2aXcVbNmLkQwErTyUiOpAsDfGhJk";
    std::string data = user_id + ":" + expire_str;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret, strlen(secret), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char *)data.c_str(), data.size());
    HMAC_Final(ctx, digest, &len);
    HMAC_CTX_free(ctx);

    // 4. 将二进制签名转换为十六进制字符串（便于传输和存储）
    std::stringstream ss;
    for (unsigned int i = 0; i < len; ++i)
    {
        // std::hex 输出十六进制，std::setw(2) 宽度2，std::setfill('0') 不足补0
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    std::string expected_signature = ss.str();

    // 5. 比对签名
    if (expected_signature != received_signature) {
        return "";
    }

    return user_id;
}

//验证token,返回user_id
std::string route_util::authenticate(const HttpRequest &req, HttpResponse *rsp) {
    std::string auth = req.getHeader("Authorization");
    if (auth.empty() || auth.find("Bearer ") != 0){
        send_json_response(rsp, 401, 3, "未认证");
        return "";
    }
    std::string token = auth.substr(7);
    std::string user_id = verify_token_and_get_user_id(token);
    //DBG_LOG("Token验证: token=%.20s..., user_id=%s", token.c_str(), user_id.c_str());
    if (user_id.empty()){
        send_json_response(rsp, 401, 3, "无效 token");
        return "";
    }
    // 新增：检查用户是否还存在
    if (!g_user_db->user_exists(user_id)){
        send_json_response(rsp, 401, 7, "账号已注销"); // 使用新的业务码 7
        return "";
    }
    return user_id;
}

// 生成唯一任务ID：毫秒时间戳 + 6位随机数
std::string route_util::generate_task_id() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(ms) + "_" + std::to_string(dis(gen));
}

// 获取小写文件扩展名（不含点）
std::string route_util::get_file_extension(const std::string& filename) {
    size_t pos = filename.rfind('.');
    if (pos == std::string::npos) return "";
    std::string ext = filename.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}


// 保存二进制内容到文件
bool route_util::save_file(const std::string& content, const std::string& path) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) return false;
    ofs.write(content.data(), content.size());
    return ofs.good();
}





bool route_util::require_super_admin(const HttpRequest &req, HttpResponse *rsp) {
    std::string user_id = authenticate(req, rsp);
    if (user_id.empty()) return false;
    if (!g_user_db->is_super_admin(user_id)) {
        send_json_response(rsp, 403, 4, "需要超级管理员权限");
        return false;
    }
    return true;
}