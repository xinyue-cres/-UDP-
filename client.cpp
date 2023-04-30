#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fstream>
#include <assert.h>
using namespace std;
 
//消息类型
enum type_t
{
    Regist,    //注册
    Login,     //登录
    Query,     //查询
    Chat,      //聊天
    Quit,      //退出
};
 
//定义描述消息结构体
typedef struct msg_t
{
    int type;       //消息类型:注册  登录  查询  聊天  退出
    char name[32];  //姓名
    char text[128]; //消息正文
} MSG_t;
 
 
int main(int argc, char const *argv[])
{
    //参数数量错误
    if (argc != 3) {
		printf("Usage:%s server_ip server_port\n", argv[0]);
		return -1;
	}

    //初始化socket
    int sockfd;
    socklen_t  addrlen;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        return -1;
    }
 
    //填充结构体
    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    addrlen = sizeof(struct sockaddr_in);
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_port = htons(atoi(argv[2]));  //端口地址
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);  //ip地址
    
    //UDP客户端不用bind地址，可以直接发送自己的登陆消息
    MSG_t msg;
    printf("what's your next choose?\n");
    printf("input 1 to Regist\ninput 2 to Login\ninput 3 to Quit\n");
    int input = 0;
    scanf("%d", &input);
    switch (input)
    {
        case 1:
            msg.type = Regist;
            printf("please input regist name>>");
            scanf("%s", &msg.name);
            strcpy(msg.text, "Regist"); 
            break;
        case 2:
            msg.type = Login;
            printf("please input login name>>");
            scanf("%s", &msg.name);
            strcpy(msg.text, "Login"); 
            break;
        case 3:
            exit(0);
            break;
        default:
            printf("wrong input!\n");
            return 0;
    }
    if (msg.name[strlen(msg.name) - 1] == '\n')
        msg.name[strlen(msg.name) - 1] = '\0';

    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr,addrlen); 
   
    // bool flag1 = false;
    // ofstream outfile;
    // outfile.open("flag.txt");
    // assert(outfile.is_open());
    // outfile << "0";
    // outfile.close();

    //创建父子进程：父进程收消息 子进程发送消息
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork error");
        return -1;
    }
    else if (pid == 0)
    {
        // fstream infile; 
        // infile.open("flag.txt".data());   //将文件流对象与文件连接起来 
        // assert(infile.is_open());   //若失败,则输出错误消息,并终止程序运行 
        // string buf, flag="";
        // getline(infile,buf);
        // flag = flag + buf;
        // infile.close();
        
        while (1) //发
        {
            fgets(msg.text, sizeof(msg.text), stdin);
            if (msg.text[strlen(msg.text) - 1] == '\n')
                msg.text[strlen(msg.text) - 1] = '\0';
            
            if (strncmp(msg.text, "Regist", 6) == 0 || strncmp(msg.text, "regist", 6) == 0)//判断发送的消息是否为“regist”注册消息
            {
                msg.type = Regist;
                printf("please input regist name>>");
                scanf("%s", &msg.name);
                strcpy(msg.text, "Regist"); 
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr,addrlen);
                continue;
            }
            else if (strncmp(msg.text, "Login", 5) == 0 || strncmp(msg.text, "login", 5) == 0)//判断发送的消息是否为“login”登录消息
            {
                msg.type = Login;
                printf("please input login name>>");
                scanf("%s", &msg.name);
                strcpy(msg.text, "Login"); 
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr,addrlen);
                continue;
            }
            else if (strncmp(msg.text, "Query", 5) == 0 || strncmp(msg.text, "query", 5) == 0)//判断发送的消息是否为“query”查询消息
            {
                msg.type = Query;
                strcpy(msg.text, "Query"); 
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr,addrlen);
                continue;
            }
            else if (strncmp(msg.text, "Quit", 4) == 0 || strncmp(msg.text, "quit", 4) == 0)//判断发送的消息是否为“quit”退出消息
            {
                msg.type = Quit;
                strcpy(msg.text, "Quit"); 
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen);
                //杀死子进程
                kill(getppid(), SIGKILL);
                //退出循环，子进程结束
                break;
            }
            // else if (strncmp(flag, "1", 1) == 0 || flag1)
            else
            {
                msg.type = Chat;//消息类型是chat
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen);
            }
                
            
        }
    }
    else
    {
        while (1) //收
        {
            if (recvfrom(sockfd, &msg, sizeof(msg), 0,(struct sockaddr *)(&serveraddr), &addrlen) < 0)
            {
                perror("recvfrom error");
                return -1;
            }

            // if (strncmp(msg.text, "login successfully!", 19) == 0 || strncmp(msg.text, "regist successfully!", 20) == 0)
            // {
            //     // printf("got the msg");
            //     // ofstream outfile1;
            //     // outfile1.open("flag1.txt");
            //     // assert(outfile.is_open());
            //     // outfile1 << "1";
            //     // outfile1.close();
            //     // flag1 = true;
            // }

            if (msg.text != "" && msg.text != NULL && msg.text != "\n")
            {
                // printf("%s\n", msg.name);
                // fprintf(stdout, "%s\n", msg.text);
                fprintf(stdout, "%s : %s\n", msg.name, msg.text);
            }
                
        }
    }
    close(sockfd);
    return 0;
}