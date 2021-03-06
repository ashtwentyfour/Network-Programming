#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>


using namespace std;


#define BUF_SIZE 64


/* struct for the call forwarding 
   thread */
struct thread_arg {
  int sockfd_;
  struct sockaddr_in6 src_;
  struct addrinfo ai_hints_;
  struct addrinfo* ai_results_;
  socklen_t srclen_;
  ssize_t size_;
};


/* table with user records 
   user_id -> address  */
map<string , pair<string , string> > user_records;


/* call forwarding function */
void* call_forwarding(void* ptr) {
  
  int ret = 0;

  /* table which stores caller's 
     addresses */
  map<string , string> call_records;
  map<string , string>::iterator it;

  char buf[BUF_SIZE], host[80], svc[80];
  thread_arg* ptr2 = (thread_arg*)ptr;
 
  string received_msg;  // for the incoming message 
  string original_msg;
  string user_host , user_svc; // to store sender info

  memset(&(ptr2->ai_hints_), 0, sizeof(ptr2->ai_hints_));
  (ptr2->ai_hints_).ai_family = AF_INET6;
  (ptr2->ai_hints_).ai_socktype = SOCK_DGRAM;
  (ptr2->ai_hints_).ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
  
  /* loop which receives a message and forwards it
     to the other peer */

  while(true) {

    memset(buf , 0 , sizeof(buf));
    ptr2->srclen_ = sizeof(ptr2->src_);

    /* receive the message */
    ptr2->size_ = recvfrom(ptr2->sockfd_ , buf, BUF_SIZE, 0, 
    (struct sockaddr*)&(ptr2->src_), &(ptr2->srclen_));

    if (ptr2->size_ == -1) {
      free(ptr);
      perror("recvfrom");
    }

    /* store the buf message as a string */
    received_msg = string(buf);
    original_msg = received_msg.substr(0 , ptr2->size_);
   
    /* get sender info */
    ret = getnameinfo((struct sockaddr *)&(ptr2->src_), sizeof(ptr2->srclen_),
    host, sizeof(host), svc, sizeof(svc), 0 | NI_NUMERICHOST);

    if (ret != 0) {
      fprintf(stderr, "getnameinfo() failed: %s\n", 
      gai_strerror(ret));
    }
 
    /* sender host and service */
    user_host = string(host);
    user_svc = string(svc);

    /* if the sender's info is not on record 
     add the client to the table */

    if(call_records.find(user_svc) == call_records.end()) {
      call_records[user_svc] = user_host;
    }
    
    /* if the sender is on record */
    if(call_records.find(user_svc) != call_records.end()) {
	
         /* if the other peer is also registered */
        if(call_records.size() == 2) {
          it = call_records.begin();
          while((*it).first == user_svc) it++;
          
	  ret = getaddrinfo(((*it).second).c_str(), ((*it).first).c_str(), 
          &(ptr2->ai_hints_), &(ptr2->ai_results_));

	  if (ret != 0) {
	    fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
	    return 0;
	  }

          // forward the message to the peer 
	  ret = sendto(ptr2->sockfd_, received_msg.c_str(), received_msg.size(), 0,
	  ptr2->ai_results_->ai_addr, ptr2->ai_results_->ai_addrlen);
	  
          if (ret == -1) {
	    perror("sendto");
	    return 0;
	  }
        }
     }
  }
}




int main() {
 
  string user_id, user_host, user_svc;
  string received_msg, original_msg;
  string reg_ack = "ACK_REGISTER ";  
  string recv_id;
  string call_failed = "CALL_FAILED";
  string media_port_msg; 
        
  int sockfd;
  struct sockaddr_in6 src, bindaddr;
  socklen_t srclen;
  char buf[BUF_SIZE];
  ssize_t size;
  int ret;
  char host[80], svc[80];
  struct addrinfo ai_hints;
  struct addrinfo *ai_results;  

  /* create and bind the socket to the
     port */
  memset(&bindaddr, 0, sizeof(bindaddr));
  bindaddr.sin6_family = AF_INET6;
  bindaddr.sin6_port = htons(34567);
  memcpy(&bindaddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET6;
  ai_hints.ai_socktype = SOCK_DGRAM;
  ai_hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG; 
  
  sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return 0;
  }
  
  
  if (bind(sockfd, (struct sockaddr *)&bindaddr, 
  sizeof(bindaddr)) != 0) {
    perror("bind");
    return 0;
  }

  srclen = sizeof(src);


  /* continuously receive messages on the server*/
  while(true) {

    // receive message 
    size = recvfrom(sockfd, buf, BUF_SIZE, 0, 
    (struct sockaddr *)&src, &srclen);

    if (size == -1) {
     perror("recvfrom");
     return 0;
    }

    // get sender info 
    ret = getnameinfo((struct sockaddr *)&src, sizeof(src), 
     host, sizeof(host), svc, sizeof(svc), 0 | NI_NUMERICHOST);

    if (ret != 0) {
      fprintf(stderr, "getnameinfo() failed: %s\n", 
      gai_strerror(ret));
    }
     
    else 
      cout<<"received from: :"<<host<<" : "<<svc<<": ";
    
    // print received message 
    fwrite(buf, size, 1, stdout);
    cout<<endl;
     
    // store messages and addresses as strings  
    received_msg = string(buf);
    original_msg = received_msg.substr(0 , size);
    user_host = string(host);
    user_svc = string(svc);

    /* if the message is a registration */
    if(strstr(buf , "REGISTER") != NULL) {

      user_id = original_msg.substr(9 , original_msg.size() - 9);
      user_records[user_id] = make_pair(user_host , user_svc);
    
      ret = getaddrinfo(host, svc, &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }
    
      // acknowledgement message 
      reg_ack = reg_ack + user_id;
    
      // send acknowledgement 
      ret = sendto(sockfd, reg_ack.c_str(), reg_ack.size(), 0, 
      ai_results->ai_addr, ai_results->ai_addrlen);


      if (ret == -1) {
        perror("sendto");
        return 0;
      }
    }
  

    /* if the message is a call request */
    else if(strstr(buf , "CALL FROM") != NULL &&
    strstr(buf , "TO") != NULL && buf[0] == 'C') {
    
      ret = getaddrinfo(host, svc, &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }
    
      /* parsing the string to get sender and 
         recipient names */
      int find = original_msg.find("TO");
      user_id = original_msg.substr(10 , find - 11);
      recv_id = original_msg.substr(find+3 , original_msg.size() - find - 3);

      /* if the recipient is not listed send failure message */
      if(user_records.find(recv_id) == user_records.end()) {

        string fail_msg = call_failed + " unknown peer";
        ret = sendto(sockfd, fail_msg.c_str(), fail_msg.size(), 0,
	ai_results->ai_addr, ai_results->ai_addrlen);
        
        if (ret == -1) {
	 perror("sendto");
	 return 0;
        }
      }

      /* else send the call initiation message 
         to the peer */
      else {
        /* get recipient adddress */
        ret = getaddrinfo((user_records[recv_id].first).c_str(), 
        (user_records[recv_id].second).c_str(), &ai_hints, &ai_results);

        if (ret != 0) {
	 fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
	 return 0;
        }

        // send the message 
        ret = sendto(sockfd, original_msg.c_str(), original_msg.size(), 0,
        ai_results->ai_addr, ai_results->ai_addrlen);

        if (ret == -1) {
          perror("sendto");
          return 0;
        }
      }  
    }
   
    /* if the server receives a call acknowledgement */
    else if(strstr(buf , "ACK_CALL") != NULL) {
  
      // get the sender and recipient names 
      int find = original_msg.find("TO");
      user_id = original_msg.substr(14 , find - 15);
      recv_id = original_msg.substr(find+3 , original_msg.size() - find - 3);
    
      ret = getaddrinfo((user_records[recv_id].first).c_str(), 
      (user_records[recv_id].second).c_str(), &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }          
      
      // send acknowledgement    
      ret = sendto(sockfd, original_msg.c_str(), original_msg.size(), 0,
      ai_results->ai_addr, ai_results->ai_addrlen);
      
      if (ret == -1) {
        perror("sendto");
        return 0;
      }
    
      // generate a random port number  
      int port_no = rand()%1000 + 5000;
      string call_port = to_string(port_no);
      int media_socket;   // media socket
      struct sockaddr_in6 src_2, bindaddr_2;
      socklen_t srclen_2;
      memset(&bindaddr_2, 0, sizeof(bindaddr_2));
      bindaddr_2.sin6_family = AF_INET6;
      bindaddr_2.sin6_port = htons(port_no);
      memcpy(&bindaddr_2.sin6_addr, &in6addr_any, sizeof(in6addr_any));    
     
      // send media port info to the clients 
      ret = getaddrinfo((user_records[recv_id].first).c_str(), 
      (user_records[recv_id].second).c_str(), &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }

      // media port announcement 
      media_port_msg = "MEDIA_PORT FROM:" + user_id + " TO:"+ 
      recv_id + " " + call_port;
      
      ret = sendto(sockfd, media_port_msg.c_str(), media_port_msg.size(), 0,
      ai_results->ai_addr, ai_results->ai_addrlen);
      
      if (ret == -1) {
        perror("sendto");
        return 0;
      }
    
      
      // media port announcement 
      media_port_msg = "MEDIA_PORT FROM:" + recv_id + " TO:"+ 
      user_id + " " + call_port;

      ret = getaddrinfo((user_records[user_id].first).c_str(), 
      (user_records[user_id].second).c_str(), &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }

      ret = sendto(sockfd, media_port_msg.c_str(), media_port_msg.size(), 0,
      ai_results->ai_addr, ai_results->ai_addrlen);
     
      if (ret == -1) {
        perror("sendto");
        return 0;
      }

      // create the media socket 
      media_socket = socket(AF_INET6, SOCK_DGRAM, 0);
      
      if (media_socket == -1) {
        perror("socket");
        return 0;
      }

      // bind the socket to the port 
      if (bind(media_socket, (struct sockaddr *)&bindaddr_2, sizeof(bindaddr_2)) != 0) {
        perror("bind");
        return 0;
      }

      srclen_2 = sizeof(src_2);
    
      pthread_t thread1;  // create a thread for message forwarding 
    
      // initialize the thread arguements
      struct thread_arg* arg;
      arg = (struct thread_arg *) malloc(sizeof(struct thread_arg));
      arg->sockfd_ = media_socket;
      arg->src_ = src_2;
      arg->srclen_ = srclen_2;
      arg->size_ = size;
    
    
      int thread;
      // start thread 
      thread = pthread_create(&thread1, NULL, &call_forwarding, (void*) arg);
      sleep(1);
  
    }    

    // if the client sends a call failed message
    else if(strstr(buf, "CALL_FAILED") != NULL) {

      // get the user IDs
      int find = original_msg.find("TO");
      user_id = original_msg.substr(18 , find - 19);
      recv_id = original_msg.substr(find+3 , original_msg.size() - find - 3);         
      ret = getaddrinfo(host, svc, &ai_hints, &ai_results);

      if (ret != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(ret));
        return 0;
      }
      
      // forward the message to the original sender 
      ret = sendto(sockfd, original_msg.c_str(), original_msg.size(), 0,
      ai_results->ai_addr, ai_results->ai_addrlen);
    
      if (ret == -1) {
        perror("sendto");
        return 0;
      }
    }
  }
  
  close(sockfd);   // close socket   
  return 0;

}
