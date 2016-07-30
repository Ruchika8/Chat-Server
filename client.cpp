#include<iostream>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<unistd.h>
#include<sys/select.h>
#include<stdlib.h>
#include<stdio.h>
#include<strings.h>
#define BUFFER 1024
using namespace std;

int session_control=0;

int validity(char *cmd)
{
  if(strcmp(cmd,"\n")){ 
  if(!strncmp(cmd,"regi",4)||!strncmp(cmd,"logi",4)||!strncmp(cmd,"send",4))
   {
     char *token=strtok(cmd," "),*arg1,*arg2;
     arg1=strtok(NULL," ");
     arg2=strtok(NULL,"\n");
  
    	if(!strcmp(token,"register"))
	{
        	if((arg1!=NULL) && (arg2!= NULL))
        	return 1;	
       	}

      	if((!strcmp(token,"send"))&& session_control==1)
	{
          	if((arg1!=NULL) && (arg2!= NULL))
           	return 1;	
       	}
        
       	if((!strcmp(token,"login"))&& session_control==0)
	{
          	if((arg1!=NULL) && (arg2!= NULL))
           	return 1;	
       	}
  
    }

   char *token=strtok(cmd,"\n");
    if((!strcmp(token,"logout") || !strcmp(token,"lobbystatus")) && session_control==1) 
    return 1;
   }    
    return 0;   
}

void beginClient(int myfd){
  fd_set readset;

  int result,count,num;
  char rbuff[BUFFER],wbuff[BUFFER],checkInput[BUFFER];
  bzero(rbuff,BUFFER);
  bzero(wbuff,BUFFER);	
 	
  while(1){
   FD_ZERO(&readset);
   FD_SET(myfd,&readset);
   FD_SET(STDIN_FILENO,&readset);
   
   result=select(myfd+1,&readset,NULL,NULL,NULL);  //multiplexing on standard input and client fd   
   

   if(result>0){
      if(FD_ISSET(STDIN_FILENO,&readset)){        //standard input check 
         num=read(0,wbuff,BUFFER);
         strcpy(checkInput,wbuff);
         if(validity(checkInput)){                //checking validity of command
             send(myfd,wbuff,num,0);              //sending command to connected socket
          }
          else
            cerr<<"Invalid command. Please enter again.\n\n";
            bzero(wbuff,BUFFER);  
            bzero(checkInput,BUFFER);   
          }

       if(FD_ISSET(myfd,&readset)){                //socket check
         count=recv(myfd,rbuff,BUFFER,0);          //reading from connected socket
         if(strncmp(rbuff,"Welcome",7)==0)            
           session_control=1;
         cerr<<rbuff<<endl;
        
         if(count==0){
         close(myfd);
         exit(0);	
        }	
      bzero(rbuff,BUFFER); 
    }
  }//result>0

   if(result<0)
   perror("error:");

}//WHILE
return;
}

int main() {

struct sockaddr_un servaddr;
int myfd=socket(AF_LOCAL,SOCK_STREAM,0);
bzero(&servaddr, sizeof(servaddr));
servaddr.sun_family=AF_LOCAL;
strcpy(servaddr.sun_path,"mysocket");

int conn_status=connect(myfd,(sockaddr*) &servaddr, sizeof(servaddr));
if(conn_status==-1){
cerr<<"\nCannot connect.";
return -1;
}

cout<<"\n\t\t\tWelcome To ChatOn   \n";
cout<<"_________________________     MENU  _____________________________\n\n";
cout<<"\t\t\t>>register<<\n";
cout<<"\t\t\t>>login<<\n";
cout<<"\t\t\t>>send<<\n";
cout<<"\t\t\t>>lobbystatus<<\n";
cout<<"\t\t\t>>logout<<\n\n";
cout<<"Enter the Command you want to perform :\n";

fcntl(myfd, 0, O_NONBLOCK); //non blocking sockets

beginClient(myfd);
close(myfd);	
}

