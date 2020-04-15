#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include "packet.h"
#include <iostream>
#include <fstream>

struct Connection {

	FILE* fd;
	uint16_t id = 1;
	uint32_t client_seq_num = 12345;
	uint32_t server_seq_num;
	uint32_t cwnd;
	uint32_t ssthresh;
}c;

PacketArgs args;

void error (char *msg);
void sig_quit_handler(int s);
void sig_term_handler(int s);



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




//MAIN
int main (int argc, char *argv[]){

  signal(SIGQUIT, sig_quit_handler);
  signal(SIGTERM, sig_term_handler);

  int port = atoi(argv[2]);
  std::string file_name = argv[3]; //file_name is the file we are sending over

  int sock, n;
  //int length;
  socklen_t length;


  struct sockaddr_in server;
  struct sockaddr_in from;
  struct hostent *hp;
  //char buffer[512];
  uint8_t buffer[524];

  if (argc < 3){
    fprintf(stderr, "ERROR, NOT ENOUGH ARGUMENT. IT SHOULD BE:\n");
    fprintf(stderr, "./client [IP address] [port number] [file name]\n");
    exit(1);
  }

  //checks for valid port number
  if (port < 1024 || port > 65535){
    fprintf(stderr, "INVALID PORT NUMBER\n");
    exit(1);
  }

  sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0){
    perror("SOCKET");
  }

  server.sin_family=AF_INET;
  hp = gethostbyname(argv[1]);

  if (hp == 0){
    perror("UNKNOWN HOST");
  }

  bcopy((char *)hp->h_addr, (char *)&server.sin_addr,hp->h_length);
  server.sin_port=htons(atoi(argv[2]));
  length = sizeof(struct sockaddr_in);



  //WE READ THE FILE, PUT IN THE THE PACKET, SEND 
  //the 512th byte in the buffer is set to be \0, only read 511
  std::ifstream input(file_name, std::ios::binary);
  FILE * file;
  file = fopen(file_name.c_str(), "r");


  if (!input.is_open())
    {
        std::cerr << "ERROR: cannot open the file" << std::endl;
        exit(EXIT_FAILURE);
    }


  


  //initializing args
  args.seq_num = c.client_seq_num;
  args.ack_num = 0;
  args.flags = 2;

  //THIS PART ACTUALLY MANAGES ACK, SYNC, AND SENDS THE MESSAGE, ALSO WAITS FOR ACK  
  while (1){
    startsending:
    uint8_t read_buff [MAX_PAYLOAD_SIZE];
    uint8_t send_pack [MAX_PACKET_SIZE];
    //std::cerr << "before reading the file" << std::endl;

    int read_bytes = fread(read_buff, sizeof(char), MAX_PAYLOAD_SIZE, file);//it was c.fd for the last parameter
    std::cerr << read_buff << std::endl;
    std::cerr << "HOW MANY BYTES I READ IN FROM THE FILE: " << read_bytes << std::endl;


    if (read_bytes < 0) {
      std::cerr << "no bytes were read from the file, so it is now done" << std::endl;
			//report_error("reading payload file", true, 1);
      break;
			}
		if (read_bytes == 0) {
      std::cerr << "no bytes were read from the file, so it is now done2" << std::endl;
			break; // No payload to send
		}

  

    //if this is our first time sending, we will send without the payload
    Packet p = Packet(args);
    //first time sending
    if (args.flags == 2){//starting with the flag 2, we will start connecting
      args.size = 12 + read_bytes;
      p = Packet(args);
      p.to_uint32_string(send_pack); //so now send_pack is what we're sending
      if (sendto(sock, send_pack, p.size(), 0, (struct sockaddr *) &(server), length) < 0){ //SENDING IT HERE
        perror("sending payload packet to server");
      }
    }

    std::cerr << "BEFORE IT RECEIVES"  << std::endl;
    memset(buffer, '\0', sizeof(buffer));
    //now we have to wait to receive the handshake confirmation
    while (1){
      n = recvfrom(sock,buffer,524,0,(struct sockaddr *) &from, &length);
      std::cerr << "AFTER IT RECEIVES"  << std::endl;
      break;
    }
    n = recvfrom(sock,buffer,524,0,(struct sockaddr *) &from, &length);
    std::cerr << "IT RECEIVED"  << std::endl;
    if (n < 0){
      perror("RECVFROM...");
    }
    p = Packet(buffer, n);



          //////////////////////////////
    if (p.flags == 6 || p.flags == 4){  //just established the connection
    //now we read the file and send and such
      std::cerr << "FLAG 4 or 6"  << std::endl;

      args.seq_num = p.ack_num;
      args.ack_num = p.seq_num + 1;
      args.flags = 4;
      memcpy(&args.payload, buffer, read_bytes);
      args.size = 12 + read_bytes;
      p = Packet(args);
      p.to_uint32_string(send_pack); //so now send_pack is what we're sending
      if (sendto(sock, send_pack, p.size(), 0, (struct sockaddr *) &(server), length) < 0){ //SENDING IT HERE
        perror("sending payload packet to server");
      }
      args.seq_num = p.ack_num;
      args.ack_num = p.seq_num + 1;
      args.flags = 4;
      input.close();
      goto startsending;
    }//end of flag 4 / 6

      else if (p.flags == 1){  //finishing up
        std::cerr << "FLAG 001"  << std::endl;
        break;
      }

    



    if (read_bytes > 0){//if valid
      
      memcpy(&args.payload, read_buff, read_bytes);
      args.size = 12 + read_bytes;
      Packet p = Packet(args);


      //SENDING
      p.to_uint32_string(send_pack); //so now send_pack is what we're sending
      std::cerr << send_pack << std::endl;
      if (sendto(sock, send_pack, p.size(), 0, (struct sockaddr *) &(server), length) < 0){ //SENDING IT HERE
        perror("sending payload packet to server");
      }

      
      //now we have to wait to receive it again
      n = recvfrom(sock,buffer,524,0,(struct sockaddr *) &from, &length);
      if (n < 0){
      perror("RECVFROM...");
      }
      p = Packet(buffer, n);

      }



    }//end of while
  }//end of main




  /*
  while (true){
    memset(buffer, '\0', sizeof(buffer));
    int bytes_send = input.read(buffer, sizeof(buffer)).gcount(); //buffer contains the contents within the txt file
    if (bytes_send == 0){
      break;
    }

    
    std::cerr << buffer;  //so buffer has the content in text file

    //currently the content is in buffer
    //needs to put the buffer in a packet with a header
    //than we can send it
    //PacketArgs package(buffer, (int)sizeof(buffer));


    //sends the first 512
    n = sendto(sock, buffer,strlen(buffer),0,(struct sockaddr *) &server, length);
    if (n < 0){
     perror("SENDTO...");
    }

    //receves what the server sends back
    n = recvfrom(sock,buffer,512,0,(struct sockaddr *) &from, &length);
    
    if (n < 0){
     perror("RECVFROM...");
    }
  }
  */
  //nput.close();

  
  //SENDING MESSAGE 
  /*
  while (1){
    printf("PLEASE ENTER THE MESSAGE: ");

    bzero(buffer, 512);
    fgets(buffer, 512, stdin);
    n = sendto(sock,buffer,strlen(buffer),0,(struct sockaddr *) &server,length);
    
    if (n < 0){
        perror("SENDTO...");
      }
    n = recvfrom(sock,buffer,256,0,(struct sockaddr *) &from, &length);
    if (n < 0){
        perror("RECVFROM...");
      }

    write(1, "Got an ack: ", 12);
    write(1, buffer, n);
  }
  */
  

 //end of main



//METHODS
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


