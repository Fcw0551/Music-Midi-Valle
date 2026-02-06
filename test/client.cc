#include "../soure/Fcw_Socket.hpp"
#include "../soure/Fcw_SignalHandler.hpp"
#include "../soure/Fcw_Util.hpp"
#include <cassert>

//测试刷新策略
// int main()
// {
//     Socket cli_sock;
//     cli_sock.createClient(8080, "127.0.0.1");
//     std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
//     while (1)
//     {
//         assert(cli_sock.send(req.c_str(), req.size()) != -1);
//         char buf[1024] = {0};
//         assert(cli_sock.recv(buf, 1023));
//         DBG_LOG("[%s]", buf);
//         sleep(3);
//     }
//     cli_sock.close();
//     return 0;
// }

//测试发送数据不对
// int main()
// {
//     Socket cli_sock;
//     cli_sock.createClient(8080, "127.0.0.1");
//     std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 100\r\n\r\nFcw_muduo";
//     while (1){
//         std::string body="Fcw_muduo";
//         std::cout<<"body长度为"<<body.size()<<std::endl;
//         std::cout<<"准备发送字节数"<<req.size()<<std::endl;
//         assert(cli_sock.send(req.c_str(), req.size()) != -1);
//         assert(cli_sock.send(req.c_str(), req.size()) != -1);
//         assert(cli_sock.send(req.c_str(), req.size()) != -1);
//         char buf[1024] = {0};
//         assert(cli_sock.recv(buf, 1023));
//         DBG_LOG("[%s]", buf);
//         sleep(3);
//     }
//     cli_sock.close();
//     return 0;
// }



//测试超时是否会导致其他连接受关联
// int main()
// {
//     signal(SIGCHLD, SIG_IGN);
//     for (int i = 0; i < 5; i++)
//     {
//         pid_t pid = fork();
//         if (pid < 0)
//         {
//             DBG_LOG("FORK ERROR");
//             return -1;
//         }
//         else if (pid == 0)
//         {
//             Socket cli_sock;
//             cli_sock.createClient(8080, "127.0.0.1");
//             std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
//             while (1)
//             {
//                 assert(cli_sock.send(req.c_str(), req.size()) != -1);
//                 char buf[1024] = {0};
//                 assert(cli_sock.recv(buf, 1023));
//                 DBG_LOG("[%s]", buf);
//             }
//             cli_sock.close();
//             exit(0);
//         }
//     }
//     while (1)
//         sleep(1);

//     return 0;
// }



//多请求处理
/*⼀次性给服务器发送多条数据，然后查看服务器的处理结果*/
/*每⼀条请求都应该得到正常处理*/
// int main()
// {
//     Socket cli_sock;
//     cli_sock.createClient(8080, "127.0.0.1");
//     std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
//     req += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
//     req += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
//         while (1){
//         assert(cli_sock.send(req.c_str(), req.size()) != -1);
//         char buf[1024] = {0};
//         assert(cli_sock.recv(buf, 1023));
//         DBG_LOG("[%s]", buf);
//         sleep(3);
//     }
//     cli_sock.close();
//     return 0;
// }



/*⼤⽂件传输测试，给服务器上传⼀个⼤⽂件，服务器将⽂件保存下来，观察处理结果*/
/*
 上传的⽂件，和服务器保存的⽂件⼀致
*/
int main()
{
    Socket cli_sock;
    cli_sock.createClient(8080, "127.0.0.1");
    std::string req = "PUT /1234.txt HTTP/1.1\r\nConnection: keep-alive\r\n";
    std::string body;
    Util::readFile("../build/wwwroot/hello.txt", &body);
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    assert(cli_sock.send(req.c_str(), req.size()) != -1);
    assert(cli_sock.send(body.c_str(), body.size()) != -1);
    char buf[1024] = {0};
    assert(cli_sock.recv(buf, 1023));
    DBG_LOG("[%s]", buf);
    sleep(3);
    cli_sock.close();
    return 0;
}
