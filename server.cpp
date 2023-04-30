#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include "assert.h"
#include "vector"

#define SERVER_PORT 3000
using namespace std;

//在线用户存储容器
vector <string> clientsOnline;

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
 
//链表的节点结构体
typedef struct node_t
{
    struct sockaddr_in addr; //数据域
    struct node_t *next;     //指针域
    // char*  name;
} link_t;
 
link_t *createLink(void);
void client_regist(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg);
void client_login(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg);
void client_query(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg);
void client_chat(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg);
void client_quit(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg);
 
int main()
{
    //初始化socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        return -1;
    }
 
    //填充服务器的ip和port
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);
    inet_aton("127.0.0.1",&serveraddr.sin_addr);
    //绑定socket
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    
    MSG_t msg;
    link_t *p = createLink();//创建一个空的有头单向链表
    link_t *q = p;
    //创建子进程，子进程接收客户端的信息并处理，父进程转发消息
    for(int i = 0; i < 10; i++)
    {
        pid_t pid[10];
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("fork error");
            return -1;
        }
        else if (pid[i] == 0)
        {
            
            while (1) //收到客户端的请求，处理请求
            {
                p = q;
                if (recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&clientaddr, &addrlen) < 0)
                {
                    perror("recvfrom error.");
                    return -1;
                }
                
                switch (msg.type)
                {
                    case Regist: //注册
                        client_regist(sockfd, p, clientaddr, msg);
                        break;    
                    case Login: //登录
                        client_login(sockfd, p, clientaddr, msg);
                        break;
                    case Query: //查询在线用户
                        client_query(sockfd, p, clientaddr, msg);
                        break;
                    case Chat: //聊天
                        client_chat(sockfd, p, clientaddr, msg);
                        break;
                    case Quit: //退出
                        client_quit(sockfd, p, clientaddr, msg);
                        break;
                }
            }
        }
        else
        {
            while (1) //服务器发通知
            {
                p = q;
                msg.type = Chat;
                //给结构体中的数组成员变量赋值，一般使用strcpy进行赋值
                strcpy(msg.name, "server");
                //获取终端输入
                fgets(msg.text, sizeof(msg.text), stdin);
                //解决发送信息时，会将换行符发送过去的问题
                if (msg.text[strlen(msg.text) - 1] == '\n')
                    msg.text[strlen(msg.text) - 1] = '\0';
                //将信息发送给同一局域网的其他客户端
                while(p->next != NULL) 
                {
                    p = p-> next;
                    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(p->addr), sizeof(clientaddr));
                    printf("sended\n");
                }
            }
        }
    }
    //程序结束时，关闭套接字描述符
    close(sockfd);
    return 0;
}
 
//链表函数 -- 创建一个空的有头单向链表
link_t *createLink()
{
    link_t *p = (link_t *)malloc(sizeof(link_t));
    if (p == NULL)
    {
        perror("malloc head node error.");
        return NULL;
    }
    p->next = NULL;
    return p;
}

//已注册用户文件写函数
void clientWrite(string file, string temp)
{
    ofstream outfile; 

    outfile.open(file.data(), ios_base::app);   //将文件流对象与文件连接起来 
    assert(outfile.is_open());   //若失败,则输出错误消息,并终止程序运行 

    outfile<<temp<<endl;
    outfile.close();             //关闭文件流
}

//已注册用户文件读函数
bool clientRead(string file, string temp)
{
    ifstream infile; 
    
    infile.open(file.data());   //将文件流对象与文件连接起来 
    assert(infile.is_open());   //若失败,则输出错误消息,并终止程序运行 

    string buf, s="";
    while(getline(infile,buf))
        s = s + buf;
    if (s.find(temp) == string::npos)
        return false;
    else 
        return true;
    infile.close();             //关闭文件流 
}

//注册函数 -- 将注册客户端的名称保存到文件中
void client_regist(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg)
{   
    string name = msg.name;
    string text = msg.text;
    
    if (!clientRead("clients.txt", name))//clients文件中不存在则写入文件
    {
        clientWrite("clients.txt", name);

        strcpy(msg.name, "server");
        sprintf(msg.text, "regist successfully!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
    }
    else//clients文件中存在则报错
    {
        strcpy(msg.name, "server");
        sprintf(msg.text, "you have registed already!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
        return;
    }
    cout << name << " " << text << endl;
    printf("port: %d\n",clientaddr.sin_port);
    clientsOnline.push_back(name);

    //告诉其他用户注册的新用户是谁
    string name1 = name + " Regist";
    strcpy(msg.text, name1.c_str()); //服务端发送新登陆的用户名
    strcpy(msg.name, "server"); //发送消息的人是服务端
    //循环发送给之前已经登陆的用户
    while (p->next != NULL)
    {
        p = p->next;
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(p->addr), sizeof(clientaddr));
    }
    //上面的代码运行结束后，此时链表指针已经指向了最后一个用户
    //创建一个新节点保存新连接的客户端地址 ，连接到链表结尾
    link_t *pnew = (link_t *)malloc(sizeof(link_t));
    if (pnew == NULL)
    {
        perror("malloc new node error");
    }
    //初始化
    pnew->addr = clientaddr;
    pnew->next = NULL;
    //链接  p是最后一个节点的地址
    p->next = pnew;
}
 
//登录函数 -- 将客户端的clientaddr保存到链表中，循环链表告诉其他用户谁退出了，客户端名称保存到在线容器中
void client_login(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg)
{
    string name = msg.name;
    string text = msg.text;
    
    if (!clientRead("clients.txt", name))//clients文件中不存在则返回需要注册
    {
        strcpy(msg.name,"server");
        strcpy(msg.text, "you should regist first!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
        return;
    }
    else//clients文件中存在则返回登录成功
    {   
        strcpy(msg.name,"server");
        strcpy(msg.text, "login successfully!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
    }
    cout << name << " " << text << endl;
    printf("port: %d\n",clientaddr.sin_port);
    clientsOnline.push_back(name);
    
    //告诉其他用户登录的新用户是谁
    string name1 = name + " Login";
    strcpy(msg.text, name1.c_str()); //服务端发送新登陆的用户名
    strcpy(msg.name, "server"); //发送消息的人是服务端
    //循环发送给之前已经登陆的用户
    while (p->next != NULL)
    {
        p = p->next;
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(p->addr), sizeof(clientaddr));
    }
    //上面的代码运行结束后，此时链表指针已经指向了最后一个用户
    //创建一个新节点保存新连接的客户端地址 ，连接到链表结尾
    link_t *pnew = (link_t *)malloc(sizeof(link_t));
    if (pnew == NULL)
    {
        perror("malloc new node error");
    }
    //初始化
    pnew->addr = clientaddr;
    pnew->next = NULL;
    //链接  p是最后一个节点的地址
    p->next = pnew;
}

//查询函数 -- 将客户端容器内存储的在线用户发送给请求者
void client_query(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg)
{
    string name = msg.name;
    string text = msg.text;
   

    if (!clientRead("clients.txt", name))//client文件中不存在则返回需要注册
    {
        strcpy(msg.name, "server");
        sprintf(msg.text, "you should regist first!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
        return;
    }
     cout << name << " " << text << endl;

    strcpy(msg.name,"server"); //发送消息的人是服务端
    for (int i = 0; i < clientsOnline.size(); i++)
        {
            strcpy(msg.text, clientsOnline[i].c_str());
            sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
        } 
}
 
//聊天信息发送函数 -- 将消息转发给所有的用户，除去发送消息的自己
void client_chat(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg)
{
    string name = msg.name;
    string text = msg.text;

    if(text != "" && text != "\0" && text != "Regist" && text != "Login" && text != "Query")
    { 
        if (!clientRead("clients.txt", name))//client文件中不存在则返回需要注册
        {
            strcpy(msg.name,"server");
            sprintf(msg.text, "you should regist first!");
            sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));//cichu hui fachu buzhidao weishenme
            return;
        }

        //从链表头开始遍历
        while (p->next != NULL)
        {
            p = p->next;
            //memcmp函数，比较内存区域a和b的前n个字节
            //参数--区域a,区域b,比较字节数n
            //返回值--a<b返回负数，a=b返回0，a<b返回正数
            if(memcmp(&(p->addr), &clientaddr, sizeof(clientaddr)) != 0)
            {
                //只要判断出用户地址和发送消息的用户地址不同，就将消息发送给该用户
                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(p->addr), sizeof(clientaddr));
            }
        }
        //在服务器中打印发送的消息
        cout << name << " said " << text << endl;
    }
}
 
//退出函数 -- 将客户端的clientaddr从链表中删除，循环链表告诉其他用户谁退出了
void client_quit(int sockfd, link_t *p, struct sockaddr_in clientaddr, MSG_t msg)
{
    string name = msg.name;
    
    if (!clientRead("clients.txt", name))//client文件中不存在则返回需要注册
    {
        strcpy(msg.name, "server");
        sprintf(msg.text, "you should regist first!");
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(clientaddr), sizeof(clientaddr));
        return;
    }
    cout << name << " " << msg.text << endl;
    
    link_t *pdel = NULL;
    
    //删除在线容器中用户的名称
    vector<string>::iterator it = clientsOnline.begin();
    while (it != clientsOnline.end())
    {
        if (*it == name)
        {
            it = clientsOnline.erase(it);
        }
        else
            it++;
    }

    string name1 = name + " Quit";
    strcpy(msg.text, name1.c_str());
    strcpy(msg.name,"server"); //发送消息的人是服务端
    //从头开始遍历查找要删除的节点
    while (p->next != NULL)
    {
        //如果循环到的地址是要删除的用户，则删除
        if (memcmp(&(p->next->addr), &clientaddr, sizeof(clientaddr)) == 0)
        {
            //删除指定用户
            pdel = p->next;
            p->next = pdel->next;
            free(pdel);
            pdel = NULL;
        }
        else
        {
            p = p->next;
            //如果不是要删除的用户，则向其发送指定用户要删除的消息
            sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&(p->addr), sizeof(clientaddr));
        }
    }
}