#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "polymorphic_connection.h"


#define BUF_SIZE 1000


#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION 0
#endif


using namespace std;

typedef string str;


const char *openssl_strerror( ) {
  return ERR_error_string(ERR_get_error(), NULL);
}


SSL_CTX *create_ssl_context( ) {
  SSL_CTX *ret;

  /* create a new SSL context */
  ret = SSL_CTX_new(SSLv23_client_method( ));
  
  if (ret == NULL) {
    fprintf(stderr, "SSL_CTX_new failed!\n");
    return NULL;
  }

  /* set our desired options */
  SSL_CTX_set_options(ret, SSL_OP_NO_SSLv2 | 
  SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

  /* set up certificate verification */
  SSL_CTX_set_verify(ret,SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
  NULL);

  SSL_CTX_set_verify_depth(ret, 4);

  /* Point our context at the root certificates */
  if (SSL_CTX_load_verify_locations(ret, NULL, "/etc/ssl/certs") == 0) {
    fprintf(stderr, "Failed to load root certificates\n");
    SSL_CTX_free(ret);
    return NULL;
  }

  return ret;
}


BIO *open_ssl_connection(SSL_CTX *ctx, const char *server) {
  BIO *ret;

  /* use settings to create a BIO */
  ret = BIO_new_ssl_connect(ctx);
  if (ret == NULL) {
    fprintf(stderr, "BIO_new_ssl_connect failed: %s\n",
    openssl_strerror( ));

    return NULL;
  }

  BIO_set_conn_hostname(ret, server);

  /* try to connect */
  if (BIO_do_connect(ret) != 1) {
    fprintf(stderr, "BIO_do_connect failed: %s\n",
    openssl_strerror( ));

    BIO_free_all(ret);
    return NULL;
  }

  /* try TLS handshake */
  if (BIO_do_handshake(ret) != 1) {
    fprintf(stderr, "BIO_do_handshake failed: %s\n",
    openssl_strerror( ));

    BIO_free_all(ret);
    return NULL;
  }

  return ret;

}

int check_certificate(BIO *conn, const char *hostname) {
  SSL *ssl;
  X509 *cert;
  X509_NAME *subject_name;
  X509_NAME_ENTRY *cn;
  ASN1_STRING *asn1;
  unsigned char *cn_str;
  int pos;
  bool hostname_match;

  /* get this particular connection's TLS/SSL data */
  BIO_get_ssl(conn, &ssl);
  if (ssl == NULL) {
    fprintf(stderr, "BIO_get_ssl failed: %s\n", openssl_strerror( ));
    return -1;
  }

  /* get connection's certificate */
  cert = SSL_get_peer_certificate(ssl);
  if (cert == NULL) {
    /* if no certificate was given*/
    return -1;
  }

  /* check that the certificate was verified */
  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    /* certificate was not successfully verified */
    return -1;
  }

  /* get the name of the certificate subject */
  subject_name = X509_get_subject_name(cert);
  
  /* and print it out */
  X509_NAME_print_ex_fp(stderr, subject_name, 0, 0);

  /* loop through "common names" (hostnames) in cert */
  pos = -1;
  hostname_match = false;
  for (;;) {
    /* move to next CN entry */
    pos = X509_NAME_get_index_by_NID(subject_name, 
    NID_commonName, pos);

    if (pos == -1) { 
      break;
    }

    cn = X509_NAME_get_entry(subject_name, pos);
    asn1 = X509_NAME_ENTRY_get_data(cn);
    if (ASN1_STRING_to_UTF8(&cn_str, asn1) < 0) {
      fprintf(stderr, "ASN1_STRING_to_UTF8 failed: %s",
      openssl_strerror( ));

      return -1;
    }


    if (strcmp((char *) cn_str, hostname) == 0) {
      hostname_match = true;
    }
  }
   

  if (hostname_match) {
    return 0;
  } else {
    fprintf(stderr, "hostnames do not match!\n");
    return -1;
  }

}

/* function which sends requests and headers - http */
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
  int index = 0;
  if(arg.find("https://") != string::npos) index = 8;
  else if(arg.find("http://") != string::npos) index = 7;
  else return false;
  
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
  if(arg.find("http://") != string::npos) {  // for http
    message = "GET " + path + " HTTP/1.1" +
             "\r\n" + "HOST: " + url + "\r\n"
      + user_agent + "\r\n\r\n";
  }


  else {  // for https
    message = "GET " + path + " HTTP/1.1" +
             "\r\n" + "HOST: " + url + "\r\n" + 
             "Connection: close\r\n\r\n";
  }
 
  
  return true;
}
    
   

int main(int argc, char *argv[]) {

  if(argc != 2) {
    cerr<<"Incorrect number of input arguements"<<endl;
    exit(0);
  }

  str arg;
  arg = str(argv[1]);
  // strings for url, path, port and message
  str url(""), path(""), port(""), message("");
  str user_agent = "User-Agent: Ashwin";

  int ret = 0;

  bool parsed = false;
  // parse the input arguement
  parsed = parse_url(message , url , path , port , arg, user_agent);

  if(!parsed) {
    cerr<<"Error parsing input arguement"<<endl;
    exit(0);
  }


  int sockfd = 0, success;

  BIO* conn;
  SSL_CTX* ctx; 

    
  /*  for https URLs - use OpenSSL */
  if(arg.find("https://") != string::npos) { 
  
    SSL_library_init( );
    SSL_load_error_strings( );

    // default port
    if(port.size() == 0) port = "443";
    str destination = url + ":" + port;

    // Create the OpenSSL context 
    ctx = create_ssl_context( );
    if (ctx == NULL) {
      fprintf(stderr, "Failed to create SSL context\n");
      return 0;
    }

  
    // Try to open an SSL connection 
    conn = open_ssl_connection(ctx, destination.c_str( ));
    if (conn == NULL) {
      fprintf(stderr, "Failed to create SSL connection\n");
      SSL_CTX_free(ctx);
      return 0;
    }
    
    // certificate check
    if (check_certificate(conn, url.c_str( )) != 0) {
      fprintf(stderr, "Certificate tests failed\n");
      BIO_free_all(conn);
      SSL_CTX_free(ctx);
      return 0;
    }

    // send request 
    BIO_puts(conn, message.c_str());

    // declare abstract class pointer
    connection* OPENSSL = new openssl(conn , BUF_SIZE);
    OPENSSL->read_write();

  }
  
 
  /* for http URLs */
  else {

    struct addrinfo ai_hints;
    struct addrinfo *ai_results, *j;

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

    connection* HTTP = new http(sockfd , BUF_SIZE);
    HTTP->read_write();
     
  }

  
   return 0; 

}
