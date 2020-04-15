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
void sig_quit_handler(int s){
    exit(0);
}
void sig_term_handler(int s){
    exit(0);
}
bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str() + 1, &buffer) == 0);
}
//create the packet from the buffer
//set the sequence and acknowledgment number based on the number of bytes
Packet::Packet(const uint8_t buffer[MAX_PACKET_SIZE], int packet_size) : size_(packet_size) {
	if (size_ < HEADER_SIZE) valid_ = false;
	else {
		payload_size_ = size_ - HEADER_SIZE;

		memcpy(&seq_num, buffer, 4);
		memcpy(&ack_num, buffer+4, 4);
		seq_num = ntohl(seq_num);
		ack_num = ntohl(ack_num);

		uint32_t id_flags;
		memcpy(&id_flags, buffer+8, 4);
		id_flags = ntohl(id_flags);
		conn_id = id_flags >> 16;
		flags = id_flags;

		memcpy(&payload, buffer+12, payload_size_);
	}
}
//Parse the packet
Packet::Packet(const PacketArgs& args) : size_(args.size) {
	if (size_ < HEADER_SIZE) valid_ = false;
	else {
		payload_size_ = size_ - HEADER_SIZE;

		seq_num = args.seq_num;
		ack_num = args.ack_num;
		conn_id = args.conn_id;
		flags = args.flags;
		memcpy(&payload, &args.payload, payload_size_);
	}
}

void Packet::to_uint32_string(uint8_t (&buf)[MAX_PACKET_SIZE]) {
	uint32_t val = htonl(seq_num);
	// std::cout << seq_num << " " << val << "\n";
	memcpy(buf, &val, 4);

	val = htonl(ack_num);
	memcpy(buf+4, &val, 4);

	val = htonl(((uint32_t)conn_id << 16) + flags);
	memcpy(buf+8, &val, 4);

	memcpy(buf+12, payload, payload_size_);
}




PacketArgs args;

//MAIN
int main (int argc, char *argv[]){ //argv[1] is port num argv[2] is directory to save files

  signal(SIGQUIT, sig_quit_handler);
  signal(SIGTERM, sig_term_handler);


  //char buffer[524];
  uint8_t buffer[524];
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
  //END OF CONNECTION HANDLE


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
    startlisten:
    memset(buffer, '\0', sizeof(buffer));
    n = recvfrom(sock,buffer,524,0,(struct sockaddr *)&from,&fromlen);
    if (n < 0){
      perror("RECVFROM...");
    }
    std::cerr << "im printing out buffer right after we get it"  << std::endl;
    std::cerr << buffer  << std::endl;

    Packet p = Packet(buffer, n);

    //p.payload has the content within the package we are looking for!!!!!!!!!!!!!!!!!!!!!
    std::cerr << p.payload  << std::endl;





    //once it receives the packet we have to verify first


    //if it's good
      //we edit the packet and send back the header
      //goto beginning of the loop



    //if it's bad, assume the first packet client send is always good.
      //i send back what ack and syc im expecting



    //checks if buff has null
    bool contains = false;
    int count = 0;
    for (int i = 0; i < 512; i++){
      if (p.payload[i] == '\0'){
        count = i;  //counts has the index where the null starts
        contains = true;
        break;
      }
    }

      //if it contains null, then we write special amount instead
    if (contains){
      writef.write((char*)p.payload, count);
/*
      uint8_t wbuff[count];
      std::cerr << count << "THIS IS COUNT" << std::endl;
      int i = 0;
      for (i = 0; i < count; i++){
        wbuff[i] = p.payload[i];
      }
      std::cerr << "RIGHT BEFORE IT USES WRITEF IN CONTAINS SECTION" << std::endl;
      writef.write((char*)p.payload, sizeof(p.payload)); //this actually writes it out to the file, originally its (char*)wbuff, sizeof(wbuff)
      memset(p.payload, '\0', sizeof(p.payload)); //clears it after
      std::cout << "File received! \n";
      goto startlisten;
      
      writef.close();
      */
      writef.close();
      break;
    }
    else if (!contains){
      
      writef.write((char*)p.payload, sizeof(p.payload));
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

