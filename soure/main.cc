
#include "Fcw_HttpServer.hpp"
#include "Fcw_Log.hpp"
#define WWWROOT "./wwwroot/"
std::string RequestStr(const HttpRequest &req)
{
    std::stringstream ss;
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &it : req._params)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers)
    {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}
void Hello(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->setContent(RequestStr(req), "text/plain");
}
void Login(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->setContent(RequestStr(req), "text/plain");
}
void PutFile(const HttpRequest &req, HttpResponse *rsp)
{
    std::string pathname = WWWROOT + req._path;
    Util::writeFile(pathname, req._body);
}
void DelFile(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->setContent(RequestStr(req), "text/plain");
}


//测试功能
int main()
{
    HttpServer server(8080);
    server.setThreadCount(1);
    server.setBaseDir(WWWROOT); // 设置静态资源根⽬录，告诉服务器有静态资源请求到来，需要到哪⾥去找资源⽂件
    server.Get("/hello", Hello);
    server.Post("/login", Login);
    server.Put("/1234.txt", PutFile);
    server.Delete("/1234.txt", DelFile);
    server.listen();
    return 0;
}
