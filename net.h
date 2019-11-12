#ifndef __MAKEU_NET_H__
#define __MAKEU_NET_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>
#include <time.h>

#define SERV_PORT 5005
#define SERV_IP_ADDR "127.0.0.1"
#define BACKLOG 5
#define QUIT_STR "quit"

#define N 32
#define A 0    //管理员,可以查看所有用户记录
#define R 1    //用户注册 user - register
#define L 2    //用户登录 user - login
#define Q 3    //用户查询单词 user - query
#define H 4    //用户查询历史记录 user - history
#define FILENAME "my.db"

#endif
