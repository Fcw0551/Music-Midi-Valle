#include "Fcw_Http.hpp"
    HttpRequest::HttpRequest() 
    : _version("HTTP/1.1") {}
    void HttpRequest::reSet(){
        _method.clear();
        _path.clear();
        _version = "HTTP/1.1";
        _body.clear();
        std::smatch match;
        _matches.swap(match);//交换清空
        _headers.clear();
        _params.clear();
    }
    // 插⼊头部字段
    void HttpRequest::setHeader(const std::string &key, const std::string &val){
        _headers.insert(std::make_pair(key, val));
    }
    // 判断是否存在指定头部字段
    bool HttpRequest::hasHeader(const std::string &key) const{
        auto it = _headers.find(key);
        if (it == _headers.end()){
            return false;
        }
        return true;
    }
    // 获取指定头部字段的值
    std::string HttpRequest::getHeader(const std::string &key) const{
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    // 插⼊查询字符串 
    void HttpRequest::setParam(const std::string &key, const std::string &val) {
        _params.insert(std::make_pair(key, val));
    }
    // 判断是否有某个指定的查询字符串
    bool HttpRequest::hasParam(const std::string &key) const{
        auto it = _params.find(key);
        if (it == _params.end()){
            return false;
        }
        return true;
    }
    // 获取指定的查询字符串
    std::string HttpRequest::GetParam(const std::string &key) const{
        auto it = _params.find(key);
        if (it == _params.end()){
            return "";
        }
        return it->second;
    }
    // 获取正⽂⻓度
    size_t HttpRequest::contentLength() const{
        // Content-Length: 1234\r\n
        bool ret = hasHeader("Content-Length");
        if (ret == false){
            return 0;
        }
        std::string clen = getHeader("Content-Length");
        return std::stol(clen);
}

    // 判断是否是短链接
    bool HttpRequest::close() const{
        // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是⻓连接
        if (hasHeader("Connection") == true && getHeader("Connection") =="keep-alive"){
            return false;
        }
        return true;
    }










// http的响应
    HttpResponse::HttpResponse() 
    : _redirect_flag(false)
    , _statu(200) 
    {}
    HttpResponse::HttpResponse(int statu) 
    : _redirect_flag(false)
    , _statu(statu) {}

    //清空防止干扰
    void HttpResponse::reSet(){
        _statu = 200;//默认返回200
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }
    // 插⼊头部字段
    void HttpResponse::setHeader(const std::string &key, const std::string &val){
        _headers.insert(std::make_pair(key, val));
    }
    // 判断是否存在指定头部字段
    bool HttpResponse::hasHeader(const std::string &key){
        auto it = _headers.find(key);
        if (it == _headers.end()){
            return false;
        }
        return true;
    }
    // 获取指定头部字段的值
    std::string HttpResponse::getHeader(const std::string &key){
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }

    // 设置响应的正⽂body部分
    void HttpResponse::setContent(const std::string &body, const std::string &type){
        _body = body;
        setHeader("Content-Type", type);
    }

    // 设置重定向
    void HttpResponse::setRedirect(const std::string &url, int statu ){
        _statu = statu;
        _redirect_flag = true;
        _redirect_url = url;
    } 
    // 判断是否是短链接
    bool HttpResponse::close(){
        // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是⻓连接
        if (hasHeader("Connection") == true && getHeader("Connection") =="keep-alive"){
            return false;
        }
        return true;
    }













    
    //解析请求行
    bool HttpContext::parseHttpLine(const std::string &line){
        
        std::smatch matches;//正则分离，匹配结果放入matches中
        std::regex re("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);//icase忽略大小写匹配
        bool ret = std::regex_match(line, matches, re);
        if (ret == false) {
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 400; // BAD REQUEST
            return false;
        }
        //0 : GET /Fcw_muduo/login?user=xiaoming&pass=123123 HTTP/1.1
        //1 : GET
        //2 : /Fcw_muduo/login
        //3 : user=xiaoming&pass=123123
        //4 : HTTP/1.1
        
        //请求⽅法的获取，并且统一转换为大写
        _request._method = matches[1];
        std::transform(_request._method.begin(), _request._method.end(), _request._method.begin(), ::toupper);
        
        //资源路径的获取，需要进⾏URL解码操作，但是不需要+转空格
        _request._path = Util::urlDecode(matches[2], false);
        
        //协议版本的获取
        _request._version = matches[4];

        //查询字符串的获取与处理
        std::vector<std::string> query_string_arry;
        std::string query_string = matches[3];
        //查询字符串的格式 key=val&key=val....., 先以 & 符号进⾏分割，得到各个字串
        Util::split(query_string, "&", &query_string_arry);
        //针对各个字串，以 = 符号进⾏分割，得到key 和val， 得到之后也需要进⾏URL解码
        for (auto &str : query_string_arry) {
            size_t pos = str.find("=");
            if (pos == std::string::npos){
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 400; // BAD REQUEST
                return false;
            }
            std::string key = Util::urlDecode(str.substr(0, pos), true);
            std::string val = Util::urlDecode(str.substr(pos + 1), true);
            _request.setParam(key, val);
        }
            return true;
    }

    //接收http的请求行
    bool HttpContext::recvHttpLine(Buffer *buf)
    {
        //判断状态
        if (_recv_statu != RECV_HTTP_LINE)
            return false;

        // 1. 获取⼀⾏数据，带有末尾的换⾏
        std::string line = buf->getLineAndPop();
        // 2. 需要考虑的⼀些要素：缓冲区中的数据不⾜⼀⾏， 获取的⼀⾏数据超⼤
        if (line.size() == 0){
            // 缓冲区中的数据不⾜⼀⾏，则需要判断缓冲区的可读数据⻓度，如果很⻓了都不⾜⼀⾏，这是有问题的 
            if (buf->readSize() > MAX_LINE){
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 414; // URI TOO LONG
                return false;
            }
            // 缓冲区中数据不⾜⼀⾏，但是也不多，就等等新数据的到来
            //注意不要把状态改变
            return true;
        }
        if (line.size() > MAX_LINE){
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 414; // URI TOO LONG
            return false;
        }
        bool ret = parseHttpLine(line);
        if (ret == false){
            return false;
        }
        
        // ⾸⾏处理完毕，进⼊头部获取阶段
        _recv_statu = RECV_HTTP_HEAD;
        return true;
    }
    
    //接收http的head
    bool HttpContext::recvHttpHead(Buffer *buf){
        if (_recv_statu != RECV_HTTP_HEAD)
            return false;

        // ⼀⾏⼀⾏取出数据，直到遇到空⾏为⽌， 头部的格式 key: val\r\nkey:val\r\n....
        while (1){
            std::string line = buf->getLineAndPop();
            // 2. 需要考虑的⼀些要素：缓冲区中的数据不⾜⼀⾏， 获取的⼀⾏数据超⼤
            if (line.size() == 0){
                // 缓冲区中的数据不⾜⼀⾏，则需要判断缓冲区的可读数据⻓度，如果很⻓了都不⾜⼀⾏，这是有问题的
                if (buf->readSize() > MAX_LINE){
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 414; // URI TOO LONG
                    return false;
                }
                // 缓冲区中数据不⾜⼀⾏，但是也不多，就等等新数据的到来
                //注意不要把状态改变
                return true;
            }
            if (line.size() > MAX_LINE){
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 414; // URI TOO LONG
                return false;
            }
            if (line == "\n" || line == "\r\n"){
                //如果某一行为空行，则说明头部处理完毕
                break;
            }
            bool ret = parseHttpHead(line);
            if (ret == false)
            {
                return false;
            }
        }
        // 头部处理完毕，进⼊正⽂获取阶段
        _recv_statu = RECV_HTTP_BODY;
        return true;
    }
    bool HttpContext::parseHttpHead(std::string &line){
        // key: val\r\n
        if (line.back() == '\n')
            line.pop_back(); // 末尾是换⾏则去掉换⾏字符
        if (line.back() == '\r')
            line.pop_back(); // 末尾是回⻋则去掉回⻋字符
        size_t pos = line.find(": ");//注意有空格
        if (pos == std::string::npos){
            _recv_statu = RECV_HTTP_ERROR;
            _resp_statu = 400; 
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 2);
        _request.setHeader(key, val);
        return true;
    }
    bool HttpContext::recvHttpBody(Buffer *buf){
        if (_recv_statu != RECV_HTTP_BODY)
            return false;
        // 1. 获取正⽂⻓度
        size_t content_length = _request.contentLength();
        if (content_length == 0){
            // 没有正⽂，则请求接收解析完毕
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }

        // 2. 当前已经接收了多少正⽂,其实就是往 _request._body 中放了多少数据了
        size_t real_len = content_length - _request._body.size(); // 实际还需要接收的正⽂⻓度
        // 3. 接收正⽂放到body中，但是也要考虑当前缓冲区中的数据，是否是全部的正⽂
        // 3.1 缓冲区中数据，包含了当前请求的所有正⽂，则取出所需的数据
        if (buf->readSize() >= real_len){
            _request._body.append(buf->readPos(), real_len);
            buf->moveReadOffset(real_len);
            _recv_statu = RECV_HTTP_OVER;
            return true;
        }
        // 3.2 缓冲区中数据，⽆法满⾜当前正⽂的需要，数据不⾜，取出数据，然后等待新数据到来
        _request._body.append(buf->readPos(), buf->readSize());
        buf->moveReadOffset(buf->readSize());
        //状态不能变，还需要接收数据
        return true;
    }
    
    HttpContext::HttpContext() 
    : _resp_statu(200)
    , _recv_statu(RECV_HTTP_LINE) {}
    void HttpContext::reSet(){
        _resp_statu = 200;
        _recv_statu = RECV_HTTP_LINE;
        _request.reSet();
    }
    int HttpContext::respStatu() { return _resp_statu; }
    HttpRecvStatu HttpContext::recvStatu() { return _recv_statu; }
    HttpRequest &HttpContext::request() { return _request; }
    
    // 接收并解析HTTP请求
    void HttpContext::recvHttpRequest(Buffer *buf){
        DBG_LOG("解析请求中。。。");
        // 不同的状态，做不同的事情，但是这⾥不要break， 因为处理完请求⾏后，应该⽴即处理头部，⽽不是退出等新数据
        //一开始是请求行
        //利用switch的贯穿，即使你跳出，也会继续执行下面的
        //但是由于一开始有状态判断，所以即使执行，但是状态不满足也不行
        switch (_recv_statu){
            case RECV_HTTP_LINE:
                recvHttpLine(buf);
            case RECV_HTTP_HEAD:
                recvHttpHead(buf);
            case RECV_HTTP_BODY:
                recvHttpBody(buf);
        }
        return;
    }