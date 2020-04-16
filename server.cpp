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
uint8_t send_pack [MAX_PACKET_SIZE];

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
  //struct sockaddr_in from;
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
  std::ofstream writef(fullfile.c_str() + 1, std::ios::binary);

  while (1){
    startlisten:
    memset(buffer, '\0', sizeof(buffer));
    n = recvfrom(sock,buffer,524,0,(struct sockaddr *)&server,&fromlen);
    if (n < 0){
      perror("RECVFROM...");
    }

    
    Packet p = Packet(buffer, n);
    memcpy(args.payload, p.payload, p.payload_size());
    writef.write((char*)p.payload, p.payload_size());
    writef.close();
    //i will just stop here with the first packet.
    

    //once it receives the packet we have to verify first


    //if it's good
      //we edit the packet and send back the header
      //goto beginning of the loop
    std::cout << "received p.seq " << p.seq_num << std::endl;
    std::cout << "received p.ack " << p.ack_num << std::endl;
    std::cout << "received p.conn_id " << p.conn_id << std::endl;
    std::cout << "received p.flags " << p.flags  << std::endl;
   


    
    
    //ACK
    //when we receive 2, we need to establish the connection and send back.
    if (p.flags == 2){//then it's trying to ack, just send data
      //std::cout << "FLAG 010"  << std::endl;
      //packetargs args is what we will be sending back
      //p.ack has 4322
      //p.seq has 12346

      //server needs to have
      //seq = 4322
      //ack = 12347
      args.seq_num = p.ack_num;
      args.ack_num = p.seq_num + 1;
      args.conn_id = 1;
      args.flags = 6;
      p.seq_num = args.seq_num;
      p.ack_num = args.ack_num;
      p.conn_id = args.conn_id;
      p.flags = args.flags;

      p = Packet (args);//pack it up then send it
      p.to_uint32_string(send_pack); //so now send_pack is what we're sending
     



      if (sendto(sock, send_pack, 524, 0, (struct sockaddr *) &(server), length) < 0){ //SENDING IT HERE
          perror("sending payload packet to server");
      }
      //std::cout << "SENT BACK WITH FLAG 6 TO START"  << std::endl;
      goto startlisten;
    }//end of 2, 010

    else if (p.flags == 4){//
      std::cout << "FLAG 100"  << std::endl;
      std::cout << "IT SHOULD WRITE IF YOU SEE THIS"  << std::endl;
      writef.write((char*)p.payload, sizeof(p.payload));
    }

    else if (p.flags == 1){//then it's done sending
      std::cout << "FLAG 100" << std::endl;
      //then we know the position of the first null character 
      //then we write a special amount to the file then

    }

  }//end of while 1
}