/*

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <deque>
#include <memory>
#include <map>

//this is from geektogeek
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 


using std::cerr;
using std::cout;
using std::deque;
using std::endl;
using std::fstream;
using std::ios;
using std::map;
using std::ofstream;
using std::shared_ptr;



void sig_quit_handler(int s)
{
    exit(0);
}

void sig_term_handler(int s)
{
    exit(0);
}


int main()
{
  signal(SIGQUIT, sig_quit_handler);
  signal(SIGTERM, sig_term_handler);


  std::cerr << "server is not implemented yet" << std::endl;

}




#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
  


         
int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    run(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

    return 0;
}


*/


//YOUTUBE METHOD
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include "packet.h"
#include <iostream>
#include <sys/stat.h> //for mkdir
#include <fstream> //handles reading writing files


void error (char *msg){
  perror(msg);
  exit(0);
}

void sig_quit_handler(int s)
{
    exit(0);
}

void sig_term_handler(int s)
{
    exit(0);
}

bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str() + 1, &buffer) == 0);
}

/*
//checks if the file exists
bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str() + 1, &buffer) == 0);
}
*/


//MAIN
int main (int argc, char *argv[]){ //argv[1] is port num argv[2] is directory to save files

  signal(SIGQUIT, sig_quit_handler);
  signal(SIGTERM, sig_term_handler);


  char buffer[512];
  std::string dirname = argv[2]; //takes in the /save
  



  
  //CONNECTION HANDLE PART
  int port = atoi(argv[1]);
  int sock, length, n;
  socklen_t fromlen;
  struct sockaddr_in server;
  struct sockaddr_in from;
  //checks if there is enough arguments when typing in the command
  if (argc < 2){
    fprintf(stderr, "ERROR, NOT ENOUGH ARGUMENT. IT SHOULD BE:\n");
    fprintf(stderr, "./server [port number] [directory]\n");
    exit(0);
  }
  //checks for valid port number
  if (port < 1024 || port > 65535){
    fprintf(stderr, "INVALID PORT NUMBER\n");
    exit(1);
  }
  sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0){
    perror("OPENING SOCKET...");
  }
  length = sizeof(server);
  bzero(&server, length); //clearing the server
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=htons(port);
  if (bind(sock,(struct sockaddr *)&server, length) < 0){
    perror("BINDING...");
  }
  fromlen = sizeof(struct sockaddr_in);


/*
  //MAKE A DIRECTORY TO SAVE THE FILE
  const int dir_err = mkdir(argv[2], 0777);
    if (-1 == dir_err)
    {
        printf("Error creating directory!n");
    }
*/


  //makes a directory name save, not /save
  mkdir(dirname.c_str() + 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   //THE #.FILE
  int i = 1;
  std::string name = std::to_string(i); // 'name' is the number
  //starts with 1.file
  std::string halffile = name + ".file";
  //    fullfile =          /save   /       1     .file
  std::string fullfile = dirname + "/" + name + ".file"; //     /save/1.file
  //checks if the file exists and name the next #.file name
  while (1){
    if (!fileExists(fullfile)){
      break;
    }
    else if (fileExists(fullfile)){//IF IT EXISTS ALREADY, INCREMENT THE COUNT AND RECHECK
        i++;
        name = std::to_string(i);
        fullfile = dirname + "/" + name + ".file";
    }
  }
  i++;
  //create the #.file
  std::cerr << "before we create the text file!!!";
  std::ofstream writef(fullfile.c_str() + 1, std::ios::binary);

  while (1){
    memset(buffer, '\0', sizeof(buffer));
    n = recvfrom(sock,buffer,512,0,(struct sockaddr *)&from,&fromlen);
    if (n < 0){
      perror("RECVFROM...");
    }
    std::cerr << buffer;

    //checks if buff has null
    bool contains = false;
    int count = 0;
    for (int i = 0; i < 512; i++){
      if (buffer[i] == '\0'){
        count = i;  //counts has the index where the null starts
        contains = true;
        break;
      }
    }

      //if it contains null, then we write special amount instead
    if (contains){
      char wbuff[count];
      int i = 0;
      for (i = 0; i < count; i++){
        wbuff[i] = buffer[i];
      }
      std::cout << "RIGHT BEFORE IT USES WRITEF";
      writef.write(wbuff, sizeof(wbuff)); //this actually writes it out to the file
      memset(buffer, '\0', sizeof(buffer)); //clears it after
      std::cout << "File received! \n";
      //goto startlisten;
      writef.close();
      break;
    }

    //if it doesnt contain null, we know that we're not end of the file, so we
    //write the full buffer into the file
    else if (!contains){
      writef.write(buffer, sizeof(buffer));
    }


  }

/*
  //RECEIVE MESSAGE
  while (1){
    n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);
    if (n < 0){
      perror("RECVFROM...");
    }
    write(1, "Received a datagram: ",21); //21 is the length
    write(1,buf,n);
    n = sendto(sock,"got your message\n", 17, 0, (struct sockaddr *)&from,fromlen);
    if (n < 0){
      perror("sendto");
    }
  }
*/  



}

