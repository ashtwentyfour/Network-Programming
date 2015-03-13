#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <cstring>

typedef std::string str;

#define buff_size 64

int main(int argc , char* argv[]) {
       
  int sockfd, ret;
  struct addrinfo ai_hints;     // structs to store address info
  struct addrinfo *ai_results;
  char node[80], service[80];   // char arrays to store node & service info of the sender 
  char buf[buff_size];       // to store packet message
  ssize_t size;  
  struct sockaddr_in6 src;
  socklen_t srclen;
    
  sockfd = socket(AF_INET6, SOCK_DGRAM, 0);  // creating IPv6 UDP socket

  if(sockfd == -1) {
    perror("socket");
    return 0;
  }
 
  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET6;
  ai_hints.ai_socktype = SOCK_DGRAM;
  ai_hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
  
  ret = getaddrinfo(argv[1], "12345", &ai_hints, &ai_results);  // obtain the address information of the rendezvous server

  if (ret != 0) {
    fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
    return 1;
  }
  
  
  str peer_address = str(argv[2]);
  str registration = "REGISTER " + peer_address;
  
  ret = sendto(sockfd, registration.c_str(), registration.size(), 0, ai_results->ai_addr, ai_results->ai_addrlen);  // send initial message to register with thw rendezvous server

  if (ret == -1) {
    perror("sendto");
    return 0;
  }
 

  str contact_address = str(argv[3]);
  str contact = "GET_ADDR " + contact_address;
  
  ret = sendto(sockfd, contact.c_str(), contact.size(), 0, ai_results->ai_addr, ai_results->ai_addrlen);  // message to get the ID of the peer to contact
  
  srclen = sizeof(src);  
  size = recvfrom(sockfd, buf, buff_size, 0, (struct sockaddr*)&src, &srclen);  // receiving a reply with the address details
  
  if(size == -1) {
    perror("recvfrom");
    return 0;
  }
  
  
  if((buf[strlen(buf)] == '\0' && strcmp(buf , "NOT FOUND\0") != 0) || (buf[strlen(buf)] != '\0' && strcmp(buf , "NOT FOUND") != 0)) {  // if the address was available 
    
    str message = str(argv[4]);
    str addr = "", port ="";      // obtaining IP address, port number
    int i = 0;

    while(buf[i] != ' ') {       
      addr = addr + buf[i];
      i++;
    }
   
    i++;
    while(buf[i] != '\0') {
      port = port + buf[i];
      i++;
    }

    
    ret = getaddrinfo(addr.c_str(), port.c_str(), &ai_hints, &ai_results);   // getting the address info for the peer to contact

    if (ret != 0) {
      fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
      return 0;
    }
        
    ret = sendto(sockfd, message.c_str(), message.size(), 0, ai_results -> ai_addr, ai_results -> ai_addrlen);   // sending the message from the command line
    
    if (ret == -1) {
      perror("sendto");
      return 0;
    }
    
  }

  
    
  for(;;) {	    // going into an infinite loop to receive packets
    srclen = sizeof(src);
    char buf_2[buff_size];
    size = recvfrom(sockfd , buf_2 , buff_size , 0 , (struct sockaddr*)&src, &srclen);
      
    if(size == -1) {
      perror("recvfrom");
      return 0;
   }
      
   
    ret = getnameinfo((struct sockaddr*)&src , sizeof(src), node, sizeof(node), service, sizeof(service), 0 | NI_NUMERICHOST); // host and service details of the sender
   
    if (ret != 0) {
       fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(ret));
   } 

    else {
      std::cout<<"received from "<<node<<" ("<<service<<"): "<<buf_2<<std::endl;   // print details of the sender
    }
  
  }
  
  return 0;
}
