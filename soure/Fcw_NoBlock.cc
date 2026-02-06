#include "Fcw_NoBlock.hpp"
// 设置非阻塞
void setNoBlock(size_t fd){
    int flag=fcntl(fd,F_GETFL);//先查询
    if(flag==-1){
        ERR_LOG("fcntl F_GETFL error");
        return ;
    }
    //设置成非阻塞
    if(fcntl(fd,F_SETFL,flag|O_NONBLOCK)==-1){
        ERR_LOG("fcntl F_SETFL error");
    }
}