#include "Fcw_Util.hpp"
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
