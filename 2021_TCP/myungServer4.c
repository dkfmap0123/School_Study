#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
void *do_chat(void *); //채팅 메세지를 보내는 함수
int pushClient(int, char *, char *); //새로운 클라이언트가 접속했을 때 클라이언트 정보 추가
int popClient(int); //클라이언트가 종료했을 때 클라이언트 정보 삭제


pthread_t thread;
pthread_mutex_t mutex;
#define MAX_CLIENT 10
#define CHATDATA 1024
#define INVALID_SOCK -1
#define PORT 9000

char    escape[ ] = "exit";
char    greeting[ ] = "채팅방에 입장하신걸 환영합니다.\n";
char    CODE200[ ] = "채팅방이 꽉찼습니다.\n";
int c_index = 1;
char username[CHATDATA];
int count=0; //닉네임을 받아올때 사용
char cut_name[CHATDATA];
char cut_talk[CHATDATA];
char client_talk[CHATDATA];
char room_select[CHATDATA];



struct room{
	int room_c;
	char room_list[CHATDATA];
};
struct room l_room[MAX_CLIENT];




struct c_user_code{
	int list_c;
	char user_name[CHATDATA];
	char user_room[CHATDATA]; //클라이언트가 선택한 채팅방 이름
};

struct c_user_code user_code[MAX_CLIENT];

int main(int argc, char *argv[ ])
{
    int c_socket, s_socket;
    struct sockaddr_in s_addr, c_addr;
    int    len;
    int    i, j, n;
    int    res;
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Can not create mutex\n");
        return -1;
    }
    s_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    if(bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1) {
        printf("Can not Bind\n");
        return -1;
    }
    if(listen(s_socket, MAX_CLIENT) == -1) {
        printf("listen Fail\n");
        return -1;
    }
    for(i = 0; i < MAX_CLIENT; i++)
        user_code[i].list_c = INVALID_SOCK;
    while(1) {
        len = sizeof(c_addr);
        c_socket = accept(s_socket, (struct sockaddr *) &c_addr, &len);
		memset(username,0,sizeof(username));
		read(c_socket,username,sizeof(username)); //클라이언트에서 오는 닉네임을 받는다.
		printf("Welcome [%s] \n",username);
		
		memset(room_select,0,sizeof(room_select));
		read(c_socket,room_select,sizeof(room_select)); //클라이언트가 선택한 채팅방을 받는다.
		printf("[%s]님이 [%s]방에 입장하셨습니다.\n",username, room_select);
		
       res = pushClient(c_socket, username, room_select);
		
		//클라이언트의 접속을 기다리고 접속시에 pushClient에서 배열에 클라이언트 번호 추가, 클라이언트가 꽉차있다면 -1, 아니면 해당 인덱스 번호가 리턴댐, 닉네임도 함께 전송
        if(res < 0) { //MAX_CLIENT만큼 이미 클라이언트가 접속해 있다면,
            write(c_socket, CODE200, strlen(CODE200));
            close(c_socket);
        } else {
            write(c_socket, greeting, strlen(greeting)); //c_socket에 환영인사를 보낸다.
           		 //채팅관련 쓰레드 생성 - 서버
			pthread_create(&thread,NULL,do_chat,(void *)&c_socket);
        }
    }
}
void *do_chat(void *arg)
{
    int c_socket = *((int *)arg);
    char chatData[CHATDATA];
	char copychatData[CHATDATA];
	char copyroom_select[CHATDATA];
    int i,j ,n;
	char *x_room = NULL;
	char *room = NULL;
	char *ptr = NULL;
	char *cutname = NULL;
	char *talk = NULL;

    while(1) {
        memset(chatData, 0, sizeof(chatData));
        if((n = read(c_socket, chatData, sizeof(chatData))) > 0) {
				
				strcpy(copychatData, chatData);
				x_room = strtok(chatData, " ");
				x_room = strtok(NULL, "\0");
				strcpy(chatData, x_room);
				

				room = strtok(copychatData, " "); // 대화방
				strcpy(copyroom_select, room);

				ptr = strtok(NULL," "); //ptr = 대화 닉네임
				strcpy(username,ptr); //username 변수에 자신의 이름저장 - 귓속말 하고나서 username 고정방지

				ptr = strtok(NULL, " "); // "/w"
				if(strncasecmp(ptr,"/w",2)==0) //귓속말 이라면
				{
					cutname=strtok(NULL," "); // cutname = 상대
					strcpy(cut_name,cutname);
					talk=strtok(NULL,"\0"); // talk = 말
					strcpy(cut_talk,talk);
					for(i=0;i<MAX_CLIENT;i++)
					{
						if(strncmp(user_code[i].user_name, cut_name, strlen(cut_name))==0) //찾는 유저가 있는지 검색
						{

							if(strncmp(user_code[i].user_room, copyroom_select, strlen(copyroom_select))==0) // 그 유저가 같은방에 있는지 검색
							{

								sprintf(client_talk, "[/W : (%s -> [%s])] : %s \n", username, cut_name, cut_talk);
								write(user_code[i].list_c, client_talk, strlen(client_talk));
								break;
							}
							else //같은방에 없으면
								write(c_socket, "채팅방에 유저가 없습니다", 50);		
						}
					}
				} //귓속말이면 입력된 닉네임(cut_name)과 구조체에 저장된 닉네임을 비교해 같으면 유저에게 내용전송

				else
				{
					for(i=0; i<MAX_CLIENT; i++)
					{
						if(user_code[i].list_c != c_socket)
						{

							if( strncasecmp(user_code[i].user_room, copyroom_select, strlen(copyroom_select))==0)
									write(user_code[i].list_c, chatData, strlen(chatData));


						}
					}

				} //자신을 제외한 다른 클라이언트에게 메세지 전송



            if(strstr(chatData, escape) != NULL) {
                popClient(c_socket);
                break;
			}
            }
        }
    }

int pushClient(int c_socket, char *name, char *room) {
    //list_c 배열에 접속한 클라이언트를 넣는다.
	//같은 인덱스로 c_user_name 배열에 닉네임 저장
	pthread_mutex_lock(&mutex);
	user_code[c_index].list_c=c_socket;
	strcpy(user_code[c_index].user_name, name);
	strcpy(user_code[c_index].user_room, room);
	c_index++;
	pthread_mutex_unlock(&mutex);
    	//클라이언트가 풀접속이면 -1, 아니면 해당 인덱스 리턴
	if(c_index>10)
		return -1;
	else
		return c_index;
}

int popClient(int c_socket)
{
	int i;
    close(c_socket);
 
	for(i=0; i<MAX_CLIENT; i++)
	{
		if(user_code[i].list_c == c_socket)
		{
			user_code[i].list_c=INVALID_SOCK;
			memset(user_code[i].user_name, '\0', sizeof(user_code[i].user_name));
			
		}
	}
	
}





