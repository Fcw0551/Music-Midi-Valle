#pragma once 
#include <iostream>
#include <string>
#include <unordered_map>
#include <regex>
#include "Fcw_Util.hpp"
#include<algorithm>
#include"Fcw_Buffer.hpp"


// 单个上传文件的信息
struct MultipartFile {
    std::string filename;       // 原始文件名
    std::string content;        // 文件二进制内容
    std::string content_type;   // MIME 类型
};

//解析http的请求
class HttpRequest
{
public:
    std::string _method;                                   // 请求⽅法
    std::string _path;                                     // 资源路径
    std::string _version;                                  // 协议版本
    std::string _body;                                     // 请求正⽂
    std::smatch _matches;                                  // 资源路径的正则表达式提取数据
    std::unordered_map<std::string, std::string> _headers; // 头部字段
    std::unordered_map<std::string, std::string> _params;  // 查询参数
    
    std::unordered_map<std::string, MultipartFile> _multipart_files;//存储解析后的上传文件（字段名 → 文件信息）比如<ref_midi 文件结构体>
public:
    HttpRequest();

    //清空
    void reSet();
    // 插⼊头部字段
    void setHeader(const std::string &key, const std::string &val);
    // 判断是否存在指定头部字段
    bool hasHeader(const std::string &key) const;
    // 获取指定头部字段的值
    std::string getHeader(const std::string &key) const;
    // 插⼊查询字符串 
    void setParam(const std::string &key, const std::string &val) ;
    // 判断是否有某个指定的查询字符串
    bool hasParam(const std::string &key) const;
    // 获取指定的查询字符串
    std::string GetParam(const std::string &key) const;
    // 获取正⽂⻓度
    size_t contentLength() const;
    // 判断是否是短链接
    bool close() const;
    
    //项目需要上传文件，解析 multipart/form-data 请求体
    bool parseMultipart();

    // 获取解析后的文件映射
    const std::unordered_map<std::string, MultipartFile> &getMultipartFiles() const{
        return _multipart_files;
    }

    // 判断是否已解析过
    bool isMultipart() const{
        return !_multipart_files.empty();
    }
};




// http的响应
class HttpResponse
{
public:
    int _statu;                                             //状态码
    bool _redirect_flag;                                    //重定向标志
    std::string _redirect_url;                              //重定向的url 
    std::string _body;                                      //响应的正⽂
    std::unordered_map<std::string, std::string> _headers;  // 头部字段

public:
    HttpResponse();
    HttpResponse(int statu);
    // 设置状态码
    void setStatus(int statu);
    //设置长连接
    void setKeepAliveByRequest(const HttpRequest &req);
    //清空防止干扰
    void reSet();
    // 插⼊头部字段
    void setHeader(const std::string &key, const std::string &val);
    // 判断是否存在指定头部字段
    bool hasHeader(const std::string &key);
    // 获取指定头部字段的值
    std::string getHeader(const std::string &key);

    // 设置响应的正⽂body部分
    void setContent(const std::string &body, const std::string &type ="text/html");

    // 设置重定向
    void setRedirect(const std::string &url, int statu = 302);
    // 判断是否是短链接
    bool close();
};







//解析http请求
typedef enum{   
    //严格执行状态转换
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatu;
#define MAX_LINE 8192
class HttpContext
{
private:
    int _resp_statu;            // 响应状态码
    HttpRecvStatu _recv_statu;  // 当前接收及解析的阶段状态
    HttpRequest _request;       // 已经解析得到的请求信息
private:

    
    //解析请求行
    bool parseHttpLine(const std::string &line);

    //接收http的请求行
    bool recvHttpLine(Buffer *buf);

    bool recvHttpBody(Buffer *buf);
    //接收http的head
    bool recvHttpHead(Buffer *buf);
    bool parseHttpHead(std::string &line);

public:
    HttpContext();
    void reSet();
    int respStatu();
    HttpRecvStatu recvStatu();

    HttpRequest &request();
    
    // 接收并解析HTTP请求
    void recvHttpRequest(Buffer *buf);
};