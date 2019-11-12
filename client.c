/**1、在搭建的框架中实现并发，
 * 2、同时支持管理员（用户名：root,密码：1）和普通用户
 * 3、管理员可以查看所有普通用户的查询记录，普通用户功能不变 
 * 提示：可以在通信的结构体中添加flag标志位判断是root用户还是普通用户
 */

#include "net.h"

#define N 32
#define R 1  //用户注册 user - register
#define L 2  //用户登录 user - login
#define Q 3  //用户查询单词 user - query
#define H 4  //用户查询历史记录 user - history

/*定义通信双方的信息结构体*/
typedef struct{
	int type;  //标志位判断
	char name[N]; //用户名
	char data[256]; //传递的数据
}MSG;

int do_register(int sockfd, MSG *msg){
	msg->type = R;

	printf("请输入姓名：");
	scanf("%s", msg->name);
	getchar();

	printf("请输入密码：");
	scanf("%s", msg->data);
	getchar();

	if(send(sockfd, msg, sizeof(MSG), 0) < 0){
		perror("send");
		return 0;
	}
	if(recv(sockfd, msg, sizeof(MSG), 0) <0){
		perror("recv");
		return 0;;
	}
	//返回“注册成功”或“注册失败”
	printf("%s\n", msg->data);

	return 1;
}

int do_login(int sockfd, MSG *msg){
	msg->type = L;
	
	printf("请输入姓名：");
	scanf("%s", msg->name);
	getchar();

	printf("请输入密码：");
	scanf("%s", msg->data);
	getchar();

	if(send(sockfd, msg, sizeof(MSG), 0) < 0){
		perror("send");
		return 0;
	}
	if(recv(sockfd, msg, sizeof(MSG), 0) <0){
		perror("recv");
		return 0;
	}

	if(strncmp(msg->data, "OK", 3) == 0){
		printf("登录成功\n");
		return 1;
	}
	else{
		printf("%s\n", msg->data);
	}
	return 0;
}

int do_query(int sockfd, MSG *msg){
	msg->type = Q;
	
	while(1){
		printf("请输入单词：");
		scanf("%s", msg->data);
		getchar();

		if(strncmp(msg->data, "#", 1) == 0){//退出单词查询
			break;
		}

		//将单词发送给服务器
		if(send(sockfd, msg, sizeof(MSG), 0) < 0){
			perror("send");
			return 0;
		}
		//接收单词的查询结果
		if(recv(sockfd, msg, sizeof(MSG), 0) <0){
			perror("recv");
			return 0;
		}
		printf("释义：%s\n", msg->data);
	}

	return 1;
}

int do_history(int sockfd, MSG *msg){
	msg->type = H;

	if(send(sockfd, msg, sizeof(MSG), 0) < 0){
		perror("send");
		return 0;
	}

	while(1){
		if(recv(sockfd, msg, sizeof(MSG), 0) < 0){
			perror("recv");
			exit(0);
		}
		//表示数据发送完毕，退出接收
		if(msg->data[0] == '\0'){
			break;
		}
		printf("%s\n", msg->data); 
	}
	return 0;
}

//查询所有用户信息
int do_query_uer_all(int sockfd, MSG *msg){
	msg->type = A;

	send(sockfd, msg, sizeof(MSG), 0);

	while(1){
	//接收所有用户信息
		recv(sockfd, msg, sizeof(MSG), 0);

		//数据接收完毕，退出接收
		if(msg->data[0] == '\0'){
			break;
		}
	
		printf("%s\n", msg->data);
	}
	return 0;
}
int do_root_login(int sockfd, MSG *msg){
	msg->type = L;
	
	printf("请输入姓名：");
	scanf("%s", msg->name);
	getchar();

	printf("请输入密码：");
	scanf("%s", msg->data);
	getchar();

	if(send(sockfd, msg, sizeof(MSG), 0) < 0){
		perror("send");
		return 0;
	}
	if(recv(sockfd, msg, sizeof(MSG), 0) <0){
		perror("recv");
		return 0;
	}

	if(strncmp(msg->data, "OK", 3) == 0){
		printf("登录成功\n");
		return 1;
	}
	else{
		printf("%s\n", msg->data);
	}
	return 0;
}


// ./server 192.168.0.200 5005
int main(int argc, const char *argv[])
{
	int sockfd;
	struct sockaddr_in serveraddr;
	MSG msg;

	if(argc != 3){
		printf("Usage: %s serverip port.\n", argv[0]);
		return -1;
	}

	/*1创建套接字*/
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <0){
		perror("socket");
		exit(1);
	}
/*2.绑定*/
	/*2.1填充struct sockaddr_in结构体*/
	bzero(&serveraddr, sizeof(serveraddr));//清零
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));//网络字节序的端口号
	//优化1:让服务器能绑定在任意的IP上
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);//点分类型IP地址转换为32位网络字节序IP地址

/*客户端调取connect函数连接服务器*/
	if(connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("connect");
		exit(1);
	}
	printf("连接服务器成功!\n");

/*一级菜单*/
	int n;
	while(1){
		printf("*******************************************\n");
		printf("*  1.register  2.login  3.quit   0.root   *\n");
		printf("*******************************************\n");

		printf("请选择：");
		scanf("%d", &n);
		getchar();

		switch(n){
			case 0:
				if(do_root_login(sockfd, &msg) == 1){
					goto root;
				}
				break;
			case 1:
				do_register(sockfd, &msg);
				break;
			case 2:
				if(do_login(sockfd, &msg) == 1){
					goto next;
				}
				break;
			case 3:
				close(sockfd);
				exit(0);
				break;
			default:
				printf("输入错误\n");
				break;
		}
	}

/*二级菜单，表示登录成功*/
next:
	while(1){
		printf("********************************************\n");
		printf("*  1.query_word  2.history_record  3.quit  *\n");
		printf("********************************************\n");
		printf("请选择：");
		scanf("%d", &n);
		getchar();

		switch(n){
		case 1:
			do_query(sockfd, &msg);
			break;
		case 2:
			do_history(sockfd, &msg);
			break;
		case 3:
			close(sockfd);
			exit(0);
			break;
		default:
			printf("输入错误\n");
		}
	}
//管理员选项
root:
	while(1){
		printf("********************************************\n");
		printf("*  1.all_name  2.quit  *\n");
		printf("********************************************\n");
		printf("请选择：");
		scanf("%d", &n);
		getchar();

		switch(n){
		case 1:
			do_query_uer_all(sockfd, &msg);//查看uer表
			break;
		case 2:
			close(sockfd);
			exit(0);
			break;
		default:
			printf("输入错误\n");
		}
	}

	return 0;
}
