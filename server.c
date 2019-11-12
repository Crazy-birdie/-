#include "net.h"


/*信息传递结构体*/
typedef struct {
	int type;
	char name[N];
	char data[256];
}MSG;


void do_client(int acceptfd, sqlite3 *db);
void do_register(int acceptfd, MSG *msg, sqlite3 *db);
void do_login(int acceptfd, MSG *msg, sqlite3 *db);
int do_query(int acceptfd, MSG *msg, sqlite3 *db);
int do_history(int acceptfd, MSG *msg, sqlite3 *db);
void do_root(int acceptfd, MSG *msg, sqlite3 *db);
int history_callback(void *arg, int f_num, char **f_value, char **f_name);
int get_date(char *date);
int do_serchword(int acceptfd, MSG *msg, char *word);
int do_query_uer_all(int acceptfd, MSG *msg, sqlite3 *db);
int uer_callbak(void *arg, int f_num, char **f_value, char **f_name);


int main(int argc, const char *argv[])
{
	sqlite3 *db;
	int sockfd, acceptfd;
	struct sockaddr_in serveraddr;
	pid_t pid;

	if(argc != 3){
		printf("User:%s 127.0.0.1 5005 \n", argv[0]);
		return -1;
	}
	//打开数据库
	if(sqlite3_open(FILENAME, &db) != SQLITE_OK){
		printf("%s\n", sqlite3_errmsg(db));
		exit(1);
	}else{
		printf("打开数据库成功\n");
	}
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}
/*填充sockaddr_in 结构体*/
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	if(bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("bind");
		exit(1);
	}
	if(listen(sockfd, BACKLOG) < 0){
		perror("listen");
		exit(1);
	}
	//处理僵尸进程
	signal(SIGCHLD, SIG_IGN);

	while(1){
		//阻塞等待接入的客户端，不关心客户端的ip和端口号
		if((acceptfd = accept(sockfd, NULL, NULL)) < 0){
			perror("accept");
			exit(1);
		}
		//创建父子进程处理接入客户端
		if((pid = fork()) < 0){
			perror("fork");
			exit(1);
		}
		if(pid == 0){//子进程
			//处理连接的客户端数据
			close(sockfd);
			do_client(acceptfd, db);
		}
		else{//父进程

			close(acceptfd);
		}
	
	}

	return 0;
}

void do_client(int acceptfd, sqlite3 *db)
{
	MSG msg;
	
	while(recv(acceptfd, &msg, sizeof(msg), 0) > 0){
		printf("type:%d\n", msg.type);
		switch(msg.type){
			case R: //注册
				do_register(acceptfd, &msg, db);
				break;
			case L:  //登录
				do_login(acceptfd, &msg, db);
				break;
			case Q:  //查单词
				do_query(acceptfd, &msg, db);
				break;
			case H:  //查历史记录
				do_history(acceptfd, &msg, db);
				break;
			case A:  //管理员（查看所有用户记录）
				do_query_uer_all(acceptfd, &msg, db);
				break;
			default:
				printf("数据无效\n");
		}
	}
	printf("客户端数据处理完毕\n");
	close(acceptfd);
	exit(0);

	return;
}

void do_register(int acceptfd, MSG *msg, sqlite3 *db){
	char *errmsg;
	char sql[128];

	sprintf(sql,"insert into uer values('%s', '%s');", msg->name, msg->data);
	printf("%s\n", sql);

	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
		printf("%s\n", errmsg);
		strcpy(msg->data, "注册失败");
	}
	else{
		strcpy(msg->data, "注册成功");
	}
	if(send(acceptfd, msg, sizeof(MSG), 0) < 0){
		perror("send");
		exit(1);
	}
	return;
}

void do_login(int acceptfd, MSG *msg, sqlite3 *db){
	char *errmsg;
	char sql[128];
	char **resultp;
	int nrow;
	int ncolumnl;

	sprintf(sql, "select *from uer where name = '%s' and password = '%s';", msg->name, msg->data);
	if(sqlite3_get_table(db, sql, &resultp, &nrow, &ncolumnl, &errmsg) != SQLITE_OK){
		printf("%s\n", errmsg);
		exit(1);
	}else{
		printf("查询语句执行成功！\n");
	}
	if(nrow == 1){
		printf("用户登录成功！\n");
		strcpy(msg->data, "OK");
		if(send(acceptfd, msg, sizeof(MSG), 0) < 0){
			perror("send");
			exit(1);
		}
	}
	if(nrow == 0){
		printf("用户登录失败！\n");
		strcpy(msg->data, "登录失败！");
		if(send(acceptfd, msg, sizeof(MSG), 0) < 0){
			perror("send");
			exit(1);
		}
	}

	return;
}

int do_serchword(int acceptfd, MSG *msg, char *word){
	FILE * fp;
	int len = 0;
	char temp[512] = {};//存放解释
	int result;
	char *p;

	//打开文件，读取文件，进行对比
	if((fp = fopen("dict.txt", "r")) == NULL){
		perror("fopen");
		strcpy(msg->data, "字典文件打开失败");
		send(acceptfd, msg, sizeof(MSG), 0);
		return 0;
	}

	//打印出客户端要查询的单词
	len = strlen(word);
	printf("%s, len = %d\n", word, len);

	//读文件，来查询单词
	while(fgets(temp, 512, fp) != NULL){
		result = strncmp(temp, word, len);

		if(result < 0){//跳出比较下一个单词
			continue;
		}
		if(result > 0 || ((result == 0) && (temp[len] != ' '))){
			break;
		}

		//找到了查询的单词
		p = temp + len;
	
		while(*p == ' '){
			p++;
		}

		//找到注释，跳过所有空格
		strcpy(msg->data, p);
		printf("单词释义：%s\n", msg->data);

		fclose(fp);
		return 1;
	}
	fclose(fp);
	return 0;
}

int get_date(char *date){
	time_t t;
	struct tm *tp;

	time(&t);

	//进行时间格式转换
	tp = localtime(&t);

	sprintf(date,"%d-%d-%d  %d:%d:%d", tp->tm_year+1900, tp->tm_mon+1, 
			tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	printf("查询时间：%s\n", date);
	return 0;
}

int do_query(int acceptfd, MSG *msg, sqlite3 *db){
	char *errmsg;
	int found;
	char sql[128];
	char word[64];//存放要查找的单词
	char date[128];//存放查询时间

	//拿出要查询的单词
	strcpy(word, msg->data);

	//查找单词
	found = do_serchword(acceptfd, msg, word);
	printf("查询单词完毕！\n");

	if(found == 1){//找到单词，将名字，时间，单词插入历史记录表，将解释发给客户端
		
		get_date(date);//获取系统时间

		sprintf(sql,"insert into history values('%s','%s','%s');", msg->name, date, word);
		printf("%s\n", sql);

		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
			printf("%s\n", errmsg);
			return -1;
		}
		else{
			printf("历史查找已记录！\n");
		}
	}

	if(found == 0){//没找到
		strcpy(msg->data, "查找单词失败");
		printf("单词：%s 没找到\n", word);
	}

	//将查询结果发给客户端
	send(acceptfd, msg, sizeof(MSG), 0);
	printf("查询结果发送成功\n");

	return 0;
}

//回调函数，得到查询结果，并将历史记录发送给客户端
int history_callback(void *arg, int f_num, char **f_value, char **f_name){
	//history, name, time, word
	int acceptfd;
	MSG msg;

	acceptfd = *((int *)arg);//强制装换传递的参数为int型
	
	//将history表中的查询时间和单词拼接给msg->data
	sprintf(msg.data,"%s %s", f_value[1], f_value[2]);
	send(acceptfd, &msg, sizeof(MSG), 0);

	return 0;
}

int do_history(int acceptfd, MSG *msg, sqlite3 *db){
	char *errmsg;
	char sql[128] = {};

	sprintf(sql, "select *from history where name = '%s';", msg->name);
	printf("%s\n", sql);

	//查询数据库
	if(sqlite3_exec(db, sql, history_callback, (void*)&acceptfd, &errmsg) != SQLITE_OK){
		printf("%s\n", errmsg);
	}
	else{
		printf("历史记录查询完毕！\n");
	}
	//历史记录查询完毕后，将结束信息发给客户端
	msg->data[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);

	return 0;
}

//将uer表中的所有记录发给客户端
int uer_callbak(void *arg, int f_num, char **f_value, char **f_name){
	//uer, name, password
	int acceptfd;
	MSG msg;

	acceptfd = (*(int *)arg);

		sprintf(msg.data,"%s %s\n", f_value[0], f_value[1]);
		
		printf("%s\n", msg.data);
		send(acceptfd, &msg, sizeof(MSG), 0);
	return 0;
}
int do_query_uer_all(int acceptfd, MSG *msg, sqlite3 *db){
	char *errmsg;
	char sql[128] = {};

	sprintf(sql,"select *from uer;");
	printf("%s\n", sql);

	if(sqlite3_exec(db, sql, uer_callbak, (void *)&acceptfd, &errmsg) !=SQLITE_OK){
		printf("%s\n", errmsg);
	}
	else{
		printf("所有用户记录查询成功\n");
	}
	//查询完毕后，将结束信息发给客户端
	msg->data[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);

	return 0;
}

