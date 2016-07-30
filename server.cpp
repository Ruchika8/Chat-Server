#include<iostream>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<unistd.h>
#include<sys/select.h>
#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include <errno.h>
#include<fstream>
#define BUFFER 1024
using namespace std;

const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;
const char* const DELIMITER = " ";

struct activeUsers{                            
             int fd;
             char user[MAX_CHARS_PER_LINE];
                  }au[100];

fd_set readset,tempset;
int activeusers_num=0;    //to keep track of active users
char rbuff[BUFFER];

struct userinfo{
         int fd;
         char cmd[BUFFER];
               }user;
        
int validity(char *cmd){
   
   if(strcmp(cmd,"\n")){
   if(!strncmp(cmd,"kick",4)||!strncmp(cmd,"ban",3)||!strncmp(cmd,"unban",5)||!strncmp(cmd,"broadcast",9)||!strncmp(cmd,"send",4)){
     char *token=strtok(cmd," "),*arg1,*arg2;
     arg1=strtok(NULL," \n");
     arg2=strtok(NULL,"\n");
    

     if(!strcmp(token,"broadcast")){
          if((arg1!=NULL) && (arg2== NULL))
		return 1;	
	}
     
    
     if(!strcmp(token,"send")){
         if((arg1!=NULL) && (arg2!= NULL))
		return 1;	
      }

      if((arg1!=NULL))//for the rest
      return 1; 
    }

     char *token=strtok(cmd,"\n");

     if(!strcmp(token,"logout") || !strcmp(token,"lobbystatus"))
            return 1;
   }
     return 0;   
    
}


int parsing(char * uname, char * pwd,int r_flag){
         ifstream fin;
         fin.open("login_pass.txt");
         char buf[MAX_CHARS_PER_LINE];
         char* token[MAX_TOKENS_PER_LINE]; 
         int n;
        while (!fin.eof())
        {
       	
        bzero(buf,MAX_CHARS_PER_LINE);
        fin.getline(buf, MAX_CHARS_PER_LINE);
        n = 0; 
    	
        token[0] = strtok(buf, DELIMITER); 
    
        if (token[0]) 
   	 {
          for (n = 1; n < MAX_TOKENS_PER_LINE; n++)
         {
          token[n] = strtok(NULL, DELIMITER); 
          if (!token[n]) break;        //returns null
          }	 
             
         if(r_flag==0){
 	 if(!strcasecmp(token[0],uname)){
           if((!strcmp(token[1],pwd)))
            {
               
               if(!strcmp(token[2],"1")){
                fin.close();
                return 2;
                }
            fin.close();
            return 1;
            }
          }
       	}//login
        if(r_flag==1){
          if(strcasecmp(token[0],uname)==0){
           	fin.close();
        return 1;
          }
       
         }//register
  
     }
   }//while
    fin.close();
    return 0;	
  } 

void serverlogs(int fd,char*username, const char* command)
{
   fstream f;
   f.open("Server_logs.txt",fstream::app | fstream::out);
   
  
   char i_a[100]; 
   char wbuff[BUFFER];
   bzero(wbuff,BUFFER);
   sprintf(i_a, "%d",fd);
   
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

  strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
  string str(buffer);
   
   strcat(wbuff,i_a);
   strcat(wbuff,DELIMITER);
   strcat(wbuff,username);
   strcat(wbuff,DELIMITER);
   strcat(wbuff,buffer);
   strcat(wbuff,DELIMITER);
   strcat(wbuff,command);
  
   f << wbuff<<endl;

  f.close();
}

void registerClient(userinfo *u){
   int fd=u->fd;
   char *token=strtok(u->cmd,DELIMITER),*uname,*pwd;
   uname=strtok(NULL,DELIMITER);
   pwd=strtok(NULL,"\n");
   bzero(rbuff,BUFFER);
 
   std::fstream f;
   f.open("login_pass.txt",fstream::app | fstream::in |   fstream::out);
   if(!f)
    {
      f.open("login_pass.txt",fstream::app | fstream::trunc | fstream::out);    
    
    }
  
   int flag=parsing(uname,pwd,1);
   
   if(flag==0){//MATCHING NOT FOUND

     strcat(rbuff,uname);
     strcat(rbuff,DELIMITER);
     strcat(rbuff,pwd);
     strcat(rbuff,DELIMITER);
     strcat(rbuff,"0\n"); 
     f<<rbuff;

     bzero(rbuff,BUFFER);
     strcpy(rbuff,"You are registered Successfully. Welcome!\n"); 
      }
    else
     strcpy(rbuff,"Choose different username.\n");
     
     f.close();
     send(fd,rbuff,strlen(rbuff),0);
     serverlogs(fd,uname,"register");
}

void broadcast(char * msg){ 
  
  for(int i=0;i<activeusers_num;i++)
   {
      send(au[i].fd,msg,strlen(msg),0);  
   }
   
}
void lobbystatus(int fd){

   char uname[100];
   strcpy(uname,"Server");
  
   if(activeusers_num){
   bzero(rbuff,BUFFER);
   strcat(rbuff,"\nLobbyStatus:- ");
   for(int i=0;i<activeusers_num;i++)
    {
     if(au[i].fd==fd)
       {
        strcpy(uname,au[i].user);
         serverlogs(fd,uname,"lobbystatus");
        }
    strcat(rbuff,au[i].user);
    strcat(rbuff,DELIMITER);
    }
    strcat(rbuff,"\n");
    if(!fd){
      cerr<<rbuff;
       serverlogs(fd,uname,"lobbystatus");  
     }
     
    if(fd==-1){
      broadcast(rbuff);
    return;
    }
    send(fd,rbuff,strlen(rbuff),0);
    return;
   }
   cerr<<"\nNo one is online.\n";
   serverlogs(fd,uname,"lobbystatus");
}

void logout(userinfo *u)
{
   int fd=u->fd,pos;
   char user[MAX_CHARS_PER_LINE];
   bzero(user,MAX_CHARS_PER_LINE);
   bzero(rbuff,BUFFER);
   for(int i=0;i<activeusers_num;i++)
   {
   if(au[i].fd==fd){
   strcpy(user,au[i].user);//for broadcast
   pos=i;
   break;
   }
   }//for

   for(int j=pos;j<activeusers_num-pos;j++)
    {
         au[j].fd=au[j+1].fd;
         strcpy(au[j].user,au[j+1].user);
        }
     activeusers_num--;
     strcat(rbuff,user);
     strcat(rbuff," has left the chat.\n");
     if(activeusers_num){
       broadcast(rbuff);
       lobbystatus(-1);
     }

 serverlogs(fd,au[pos].user,"logout");
     
       
}
void kick(char *u,char *msg)
{
   int pos=-1;
   bzero(rbuff,BUFFER);
   
   for(int i=0;i<activeusers_num;i++)
   {
   if(!strcasecmp(au[i].user,u)){
      pos=i;
      break;      
   }
   }//for
   if(pos==-1)
      { 
         cerr<<"User not online\n";
         return;
       }
   close(au[pos].fd);
   FD_CLR(au[pos].fd, &readset);
   for(int j=pos;j<activeusers_num-pos;j++)
{
         au[j].fd=au[j+1].fd;
         strcpy(au[j].user,au[j+1].user);
        }
     activeusers_num--;
 
   
     strcat(rbuff,u); 
     strcat(rbuff," has been kicked out ");
 
     if(msg!=NULL){
       strcat(rbuff,"for "); 
       strcat(rbuff,msg);
       }
     strcat(rbuff,"\n");
     if(activeusers_num){
        broadcast(rbuff);
        lobbystatus(-1);
      }
    serverlogs(0,u,"kick");

}
void ban(char* u, char* msg){
    for(int i=0;i<activeusers_num;i++)
     {
       if(!strcasecmp(au[i].user,u))
        kick(u,msg);
      }
    //int count=0;
    char *uname;
    fstream fin;
    fin.open("login_pass.txt");
    if(!fin){
     return;
    }
    int count=0;
        while (!fin.eof())
        {
       	
    bzero(rbuff,MAX_CHARS_PER_LINE);
    fin.getline(rbuff, MAX_CHARS_PER_LINE);
        uname= strtok(rbuff, DELIMITER);
        count+=fin.gcount();  
       
        if(uname){
          if(!strcmp(uname,u)){
         fin.seekp (count-2); 
         fin<<'1';
        } 
        }
       
    } //while 

  strcat(rbuff,u); 
     strcat(rbuff," has been banned ");
 
     if(msg!=NULL){
       strcat(rbuff,"for "); 
       strcat(rbuff,msg);
       }
     strcat(rbuff,"\n");

     if(activeusers_num){
        broadcast(rbuff);
        lobbystatus(-1);
      }
   
   serverlogs(0,u,"ban");
 }
void unban(char* u, char* msg){
   
    char *uname;
    fstream fin;
    fin.open("login_pass.txt");
     int count=0;
     if(!fin){
     return;
    }
      
        while (!fin.eof())
        {
       	
    bzero(rbuff,MAX_CHARS_PER_LINE);
    fin.getline(rbuff, MAX_CHARS_PER_LINE);
        uname= strtok(rbuff, DELIMITER);
        count+=fin.gcount();  
       
        if(uname){
          if(!strcmp(uname,u)){
         fin.seekp (count-2); 
         fin<<'0';
        }
        }
       
    } //while
    
     bzero(rbuff,BUFFER);
    
     strcat(rbuff,u); 
     strcat(rbuff," has been unbanned ");
 
     if(msg!=NULL){
       strcat(rbuff,"for "); 
       strcat(rbuff,msg);
       }
     strcat(rbuff,"\n");
     if(activeusers_num){
        broadcast(rbuff);
      }
  
  serverlogs(0,u,"unban");
  
 }

void send(userinfo * u)
{
   bzero(rbuff,BUFFER);
   int pos=-1,fd;
   char *token=strtok(u->cmd,DELIMITER),*d_uname,*message;
   d_uname=strtok(NULL,DELIMITER);
   message=strtok(NULL,"\n");
   
   char user[MAX_CHARS_PER_LINE];
   char s_uname[]="Server ";
   bzero(user,MAX_CHARS_PER_LINE);
   for(int i=0;i<activeusers_num;i++)
   {
   if(au[i].fd==u->fd){
    strcpy(s_uname,au[i].user); 
    } //source
  
   if(!strcasecmp(au[i].user,d_uname)){
   pos=i;
   }//destination
   }//for
    
     serverlogs(u->fd,s_uname,"send");
    if(pos==-1){
    strcpy(rbuff,"User not available.\n");
    cerr<<rbuff;
    if(u->fd)
    send(u->fd,rbuff,strlen(rbuff),0);
    return;
   }
  
   if(!strcasecmp(s_uname,d_uname)){
    strcpy(rbuff,"You cannot message yourself.\n");
    send(u->fd,rbuff,strlen(rbuff),0);
    return;
   }
  
   strcat(rbuff,s_uname);
   strcat(rbuff,":-  ");
   strcat(rbuff,message);
    strcat(rbuff,"\n");

   fd=au[pos].fd;
   send(fd,rbuff,strlen(rbuff),0);
}


void loginClient(userinfo *u){
  bzero(rbuff,BUFFER);
  int flag=0,repeat=0;
   int fd=u->fd;
  
   char *token=strtok(u->cmd," "),*uname,*pwd;
   uname=strtok(NULL," ");
   pwd=strtok(NULL,"\n");   
   serverlogs(fd,uname,"login");   
    fstream f;
    f.open("login_pass.txt", fstream::in);
    if(!f)
   {
    strcpy(rbuff,"You need to register first!\n");  
    send(fd,rbuff,strlen(rbuff),0); 
    f.close();
    return;
   }
  
   flag=parsing(uname,pwd,0);
   
    if(flag==0){//MATCHING NOT FOUND  
       strcpy(rbuff,"\nInvalid Username or password.\n");
       send(fd,rbuff,strlen(rbuff),0); 
    }
    else if(flag==2){
       strcpy(rbuff,"\nYou are banned!!\n");
       send(fd,rbuff,strlen(rbuff),0); 
    }
    else{
          for(int i=0;i<activeusers_num;i++){
            if(!strcasecmp(au[i].user,uname))
                repeat=1;
          }
          if(!repeat){
          au[activeusers_num].fd=fd;
          strcpy(au[activeusers_num++].user,uname);
          strcat(rbuff,"Welcome ");
          strcat(rbuff,uname);
          strcat(rbuff,"!\n");
          send(fd,rbuff,strlen(rbuff),0); 
          lobbystatus(-1);
          return; 
          } 
  
    strcpy(rbuff,"You are already logged in!\n"); 
    send(fd,rbuff,strlen(rbuff),0);              
        
}
      
}
 
int main() {
        struct sockaddr_un servaddr;
        int maxfd,listenfd,result,connfd,j;
        bzero(user.cmd,BUFFER);
        socklen_t addrlen;
        char check[BUFFER];
        strcpy(check,"Server"); 

        fstream f;
        f.open("Server_logs.txt",fstream::app | fstream::trunc |fstream::out); 
         serverlogs(0,check,"Active");
        bzero(check,BUFFER);
        if((listenfd=socket(AF_LOCAL,SOCK_STREAM,0))< 0){
		perror("socket failed");
        	exit(1);	
	}
	fcntl(listenfd, 0, O_NONBLOCK);      //To make socket Non Blocking
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family=AF_LOCAL;
	strcpy(servaddr.sun_path,"mysocket");
	
	unlink("mysocket");
	
        if (bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr))<0) 
        {
        perror("bind failed");
        exit(EXIT_FAILURE);
        }
	
        listen(listenfd,100);
   	puts("Waiting for connections ...");
	FD_ZERO(&readset);
        FD_SET(STDIN_FILENO,&readset);
	FD_SET(listenfd, &readset);
	maxfd = listenfd;
	while(1){
	FD_ZERO(&tempset);
	memcpy(&tempset, &readset, sizeof(readset));   
        result = select(maxfd + 1, &tempset, NULL, NULL, NULL);
 	
        if (result < 0 && errno != EINTR) {
     	printf("Error in select(): %s\n", strerror(errno));
   	}
        else if (result > 0) {
                if (FD_ISSET(listenfd, &tempset)) {                          
                    addrlen = sizeof(servaddr);
                    connfd = accept(listenfd, (sockaddr *)&servaddr, &addrlen);
                    if (connfd < 0) {
                         printf("Error in accept(): %s\n", strerror(errno));
                    }
                    else {
                        FD_SET(connfd, &readset);
                        maxfd = (maxfd < connfd)?connfd:maxfd;
                 }
               continue;        
              }//listenfd check

        for (j=0; j<maxfd+1; j++) {                       //we are checking for each socket except listenfd       
            if (FD_ISSET(j, &tempset) && j!=listenfd) {   
               do {
                  result = read(j, user.cmd, BUFFER);
                  
                  }while (result == -1 && errno == EINTR);

               if (result > 0) {

               strcpy(check,user.cmd);
               if(!j && !validity(check)){             
                  cerr<<"Invalid Command.Please Enter again.\n\n";
                  continue;
              }
              bzero(check,BUFFER);
              if(strncmp(user.cmd,"regi",4)==0){
                user.fd=j;
                registerClient(&user);
      		bzero(user.cmd,BUFFER);	
               }
            
              if(strncmp(user.cmd,"logi",4)==0){
              user.fd=j;
              loginClient(&user);
              bzero(user.cmd,BUFFER);
              }
              
              if(strncmp(user.cmd,"lobby",5)==0){
              lobbystatus(j);
              bzero(user.cmd,BUFFER);
              }
            
              if(strncmp(user.cmd,"logout",6)==0){
                 if(j){                       //for client
                   user.fd=j;
                   logout(&user);
                   bzero(user.cmd,BUFFER); 
                   close(j);
                   FD_CLR(j, &readset);
                 }
                 else                         //for server
                  {
                   close(listenfd);
                   return 0;
                  }
 	      }
            
              if(strncmp(user.cmd,"send",4)==0){
                 user.fd=j;
                 send(&user);
                 bzero(user.cmd,BUFFER); 
                }
 	   
             if(strncmp(user.cmd,"broadcast",8)==0){
                char *token=strtok(user.cmd,DELIMITER),*msg;
                msg=strtok(NULL,"\n");
                broadcast(msg);
                bzero(user.cmd,BUFFER); 
               }
            
            if(strncmp(user.cmd,"kick",4)==0){   
               int flag=0; 
               char *token=strtok(user.cmd,DELIMITER),*username,*p,*msg;
               username=strtok(NULL," \n");   //message is optional
               msg= strtok(NULL,"\n");
               kick(username,msg);         //msg!Null checking in kick
               bzero(user.cmd,BUFFER); 
                
 	   }
        
           if(strncmp(user.cmd,"ban",3)==0){
                char *token=strtok(user.cmd,DELIMITER),*username,*msg;
                username=strtok(NULL," \n");   
                msg= strtok(NULL,"\n");
                ban(username,msg);
                bzero(user.cmd,BUFFER); 
                
 	   }
            
           if(strncmp(user.cmd,"unban",5)==0){
                char *token=strtok(user.cmd,DELIMITER),*username,*msg;
                username=strtok(NULL," \n");   
                msg= strtok(NULL,"\n");
                unban(username,msg);
                bzero(user.cmd,BUFFER); 
                
 	   }
 
     }//if result(read) >0
          
     else if (result == 0){
            close(j);
            FD_CLR(j, &readset);
            }
     else 
        cout<<strerror(errno);
      
    }      // end if (FD_ISSET(j, &tempset))
  }      // end for 
 }//result(select)>0
}//While
}


