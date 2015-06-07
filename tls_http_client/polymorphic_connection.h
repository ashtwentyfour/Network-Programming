#include <iostream>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>


using namespace std;

typedef string str;


/* abstract class connection */
class connection {
 public:

  ~connection() {}
  virtual void read_write() = 0;  // read and write function

};


/* derived class for https connections */
class openssl : public connection {
 public:

  openssl(BIO* C, int buf_size) { // constructor
    conn = C;
    BUF_SIZE = buf_size;
  }
  
  ~openssl() {}
  
  void read_write() {
    int size;
    char BUF[BUF_SIZE];
    
    do {  // receive/read response
     size = BIO_read(conn, BUF, BUF_SIZE);
     if (size > 0) {
       fwrite(BUF, size, 1, stdout); // display/write response
     }
    } while (size > 0 || BIO_should_retry(conn));
    
    cout<<endl;     
    BIO_free_all(conn);
    return;

  }

  
 private:

  BIO* conn;
  int BUF_SIZE; // buffer size

};


/* derived class for http connections */
class http : public connection {
 public:

  http(int SOCK, int buf_size) {  // constructor
    sockfd = SOCK;
    BUF_SIZE = buf_size;
  }

  ~http() {}

  void read_write() {  // read and write function 
    int size = 1;
    char BUF[BUF_SIZE];
    size = recv(sockfd, BUF, sizeof(BUF), 0); // receive 
    if (size > 0) {
      fwrite(BUF, size, 1, stdout); // display 
    }

    // obtain the content length from the header
    char* pch = strstr(BUF , "Content-Length: ");
    if(pch == NULL) {
      cerr<<"Missing content header"<<endl;
      exit(0);
    }


    // create a buffer with size = content length
    int index = 16;
    str content_length("");
    while(*(pch + index) != '\r') {
      content_length = content_length + *(pch + index);
      index++;
    }


    long int x = atoi(content_length.c_str());
    char content[x+1];

    // receive/read content and display/write
    size = 1;
    while(size > 0) {
      size = recv(sockfd, content, sizeof(content), 0);
      fwrite(content , size, 1, stdout);
    }

    cout<<endl;
    close(sockfd);
    return;

  }


 private:
  int sockfd;
  int BUF_SIZE;  // buffer size

};
