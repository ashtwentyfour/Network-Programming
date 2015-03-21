#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>


#define BUFF_SIZE 1000


typedef std::string str;


/* function which sends requests and headers */
int send_all(int fd, const void *data, size_t size, int flags) {
  ssize_t ret, sent = 0;
  uint8_t *bytes = (uint8_t *)data;

  while (size > 0) {
    ret = send(fd, bytes, size, flags);

    if (ret < 0) {
      return ret;
    } else {
      size -= ret;
      bytes += ret;
      sent += ret;
    }
  }

  return sent;
}


/* function which spilts the input arguement into
   url, path and port number */
bool parse_url(str& message, str& url,str& path, str& port, 
               str arg, str user_agent) {
  
  if(arg.size() <= 7) { // input must atleast have 'http://'
    return false;
  }
  
  // get the url and port (if any)
  int index = 7;
  while(arg[index] != '/') {
    if(arg[index] == ':') {
      index++;
      while(arg[index] != '/') {
        port = port + arg[index];
        index++;
      }
      break;
    }
    url = url + arg[index];
    index++;
  }
  
  // get the required path 
  while(index != arg.size()) {
    path = path + arg[index];
    index++;
  }

  if(path == "") path = path + '/';
    
  // final message to be sent
  message = "GET " + path + " HTTP/1.1" +
             "\r\n" + "HOST: " + url + "\r\n"
    + user_agent + "\r\n\r\n";
  
  return true;
}
    
   

int main(int argc, char *argv[]) {
  
  if(argc != 2) {
    std::cerr<<"Incorrect number of input arguements"<<std::endl;
    exit(0);
  }
 
  // strings for url, path, port and message 
  str url(""), path(""), port(""), message(""), arg;
  str user_agent = "User-Agent: Ashwin";
  arg = str(argv[1]);

  bool parsed = false;
  // parse the input arguement
  parsed = parse_url(message , url , path , port , arg, user_agent);
  
  if(!parsed) {
    std::cerr<<"Error parsing input arguement"<<std::endl;
    exit(0);
  }

  
  int sockfd, ret, success;
  struct addrinfo ai_hints;
  struct addrinfo *ai_results, *j;

  char recvbuf[BUFF_SIZE];
  
  // create an IPv6 TCP socket 
  sockfd = socket(AF_INET6, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return 0;
  }

  // find TCP IPv6 addresses from command line arg 
  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET6;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG; 
  
  // use default port 80 if required
  if(port.size() == 0) port = "80"; 
 
  ret = getaddrinfo(url.c_str(), port.c_str(), &ai_hints, &ai_results);

  if (ret != 0) {
    fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
    return 0;
  }

  // try to connect to one of the addresses
  success = 0;
  for (j = ai_results; j != NULL; j = j->ai_next) {
    ret = connect(sockfd, j->ai_addr, j->ai_addrlen);
    if (ret == 0) {
      success = 1;
      break;
    }
  }

  freeaddrinfo(ai_results);

  if (success == 0) {
    perror("connect");
    return 0;
  }

  // send message after connecting 
  ret = send_all(sockfd, message.c_str(), message.size(), 0);
  if (ret < 0) {
    perror("send_all");
    return 0;
  }

  // receive the header information  
  ret = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
   
  fwrite(recvbuf , ret , 1  , stderr);
  
  // obtain the content length from the header
  char* pch = strstr(recvbuf , "Content-Length: ");
  if(pch == NULL) {
    std::cerr<<"Missing content header"<<std::endl;
    exit(0);
  }   
     
  
   // create a buffer of with size = content length 
   int index = 16;
   str content_length("");
   while(*(pch + index) != '\r') {
     content_length = content_length + *(pch + index);
     index++;
   }
 
  
   long int x = atoi(content_length.c_str());
   char content[x+1];
   
   // receive content and display
   while(ret > 0) {  
     ret = recv(sockfd, content, sizeof(content), 0);
     fwrite(content, ret, 1, stdout);
   }
   

   std::cout<<std::endl;  

   // close socket
   close(sockfd);
   return 0; 

}
