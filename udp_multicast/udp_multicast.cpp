#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <pthread.h>
#include <map>
#include <utility>


#define BUFF_SIZE 64    // buffer size

// typedefs
typedef std::string STR;
typedef std::map<std::string , std::pair<std::string , std::string> > ADDRESS_TABLE;


// struct of arguements for the announcement thread
struct thread1_arg {  
  int sockfd_;
  STR username_;
  struct sockaddr_in dest_;
};


// struct with arguements for the receive thread
struct thread2_arg {
  int sockfd_;
  socklen_t srclen_;
  ssize_t size_;
  struct sockaddr_in src_;
};


ADDRESS_TABLE address_table; // address table


/* thread for periodic announcements */
void* announce_thread(void* ptr) {
  int ret;
  thread1_arg* ptr2 = (thread1_arg*)ptr;
  STR announcement = "ANNOUNCE: " + ptr2->username_;

 while(true) {

  // make announcement
  ret = sendto(ptr2->sockfd_, announcement.c_str(), announcement.size(), 
  0, (struct sockaddr*)&(ptr2->dest_), sizeof(ptr2->dest_));
  
  if (ret == -1) {
    perror("sendto");
  }
  
  sleep(60); // thread sleeps for a minute
  
 }
 
  pthread_exit(NULL);

}


/* thread for receiving messages */
void* receive_thread(void* ptr) {
  char buf[BUFF_SIZE], host[80], svc[80];
  thread2_arg* ptr2 = (thread2_arg*)ptr;
  char* pos;
  int ret;
  STR buf_str;
 while(true) {
  memset(buf , 0 , sizeof(buf));
  ptr2->srclen_ = sizeof(ptr2->src_);

  // receive message
  ptr2->size_ = recvfrom(ptr2->sockfd_, buf, BUFF_SIZE, 
  0, (struct sockaddr *)&(ptr2->src_), &(ptr2->srclen_));
  
  if (ptr2->size_ == -1) {
    free(ptr);
    perror("recvfrom");
  }

  // get sender info
  ret = getnameinfo((struct sockaddr *)&(ptr2->src_), sizeof(ptr2->src_), 
  host, sizeof(host), svc, sizeof(svc), NI_NUMERICHOST);
    
  if (ret != 0) {
    fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(ret));
  } 

  // if the message is an announcement
  pos = strstr(buf, "ANNOUNCE");
  if(pos != NULL) {
    buf_str = STR(buf);
    address_table[buf_str.substr(10, buf_str.size() - 10)] = 
    std::make_pair(STR(host) , STR(svc));
  }
  
  // otherwise print out the message
  else {
    std::cout<<buf<<std::endl;
    fflush(stdout);
  }

 }  
  
  pthread_exit(NULL);

}


int main(int argc , char* argv[]) {
  if(argc != 2) {
    std::cerr<<"Incorrect number of input arguements"<<std::endl;
    exit(0);
  }

  STR username = STR(argv[1]);
  
  const char* multi_addr = "239.255.24.25";   
  int sockfd, ret, ret_reuse;
  struct sockaddr_in dest, bindaddr, src, msg_dest;
  socklen_t srclen = 0;
  ssize_t size = 0;
  struct ip_mreq mreq;
  
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_port = htons(23456);
  bindaddr.sin_addr.s_addr = INADDR_ANY;

  dest.sin_family = AF_INET;
  dest.sin_port = htons(23456);
  if (inet_aton(multi_addr, &dest.sin_addr) == 0) {
    fprintf(stderr, "invalid address!\n");
    return 0;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return 0;
  }
  
  // SO_REUSESOCKET for testing
  int optval = 1;
  ret_reuse = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
  
  if (bind(sockfd, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) != 0) {
    perror("bind");
    return 0;
  }
 
  
  if (inet_pton(AF_INET, multi_addr, &mreq.imr_multiaddr) != 1) {
    fprintf(stderr, "inet_pton failed\n");
    return 0;
  }
  
  
  ret = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
   
  
  if (ret != 0 || ret_reuse != 0) {
  perror("setsockopt");
  return 0;
  }
       
  
  pthread_t thread1, thread2;  // creating 2 pthreads  
  int ret_1, ret_2;

  /* declaring their respective struct arguements */

  struct thread1_arg *args_1;
  args_1 = (struct thread1_arg *) malloc(sizeof(struct thread1_arg));
  args_1->sockfd_ = sockfd;
  args_1->username_ = username;
  args_1->dest_ = dest;


  struct thread2_arg *args_2;
  args_2 = (struct thread2_arg *) malloc(sizeof(struct thread2_arg));
  args_2->sockfd_ = sockfd;
  args_2->src_ = src;
  args_2->srclen_ = srclen;
  args_2->size_ = size;
  
  // creating the 2 pthreads   
  ret_1 = pthread_create(&thread1, NULL, &announce_thread, (void*) args_1);
  ret_2 = pthread_create(&thread2, NULL, &receive_thread, (void*) args_2); 
  sleep(1);
  

  STR message, sub_usrname, rest_of_msg;
  std::size_t pos;

  /* starting the infinite while loop for
   message input */

  while(true) {
    std::getline(std::cin , message);
    if(message[0] == '/') {  // if the message is private
      pos = message.find(' ');
      sub_usrname = message.substr(1,pos-1); // get the message 
      // check whether the username has been recorded
      if(address_table.find(sub_usrname) != address_table.end()) {
	rest_of_msg = message.substr(pos+1, message.size() - pos - 1);
	rest_of_msg = "from " + username + " to " + sub_usrname + ": " + rest_of_msg;
        
        msg_dest.sin_family = AF_INET;
	msg_dest.sin_port = htons(23456);
            
	if (inet_aton((address_table[sub_usrname].first).c_str(), &msg_dest.sin_addr) == 0) {
	  fprintf(stderr, "invalid address!\n");
	  return 0;
	}
        
        // send the message to the username specified
        ret = sendto(sockfd, rest_of_msg.c_str(), rest_of_msg.size(), 
        0, (struct sockaddr *)&msg_dest, sizeof(msg_dest));
	
        if (ret == -1) {
	  perror("sendto");
	  return 0;
	}
      
      }
      else {
        std::cerr<<"Address not on record: private message not sent! "; 
	std::cerr<<"Please wait for 60 seconds!"<<std::endl;
      }
    }

    // if not a private message send it to the multicast address
    else if(message[0] != '/'){
      message = "FROM:" + username + " " + message;
      ret = sendto(sockfd, message.c_str(), message.size(), 
      0, (struct sockaddr *)&dest, sizeof(dest));
      if (ret == -1) {
	perror("sendto");
	return 0;
      }
    }
  }  
      
  return 0;

}
