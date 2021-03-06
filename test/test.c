#include "./common/rebrick_common.h"
#include "server_client/udpecho.h"
#include "server_client/tcpecho.h"
#include "./common/rebrick_util.h"
#include "cmocka.h"
#include <unistd.h>

extern int test_rebrick_util();
extern int test_rebrick_resolve();
extern int test_rebrick_filestream();
extern int test_rebrick_timer();
extern int test_rebrick_udpsocket();
extern int test_rebrick_tcpsocket();
extern int test_rebrick_buffer();
extern int test_rebrick_buffers();
extern int test_rebrick_tls();
extern int test_rebrick_tlssocket();
extern int test_rebrick_http();
extern int test_rebrick_httpsocket();
extern int test_rebrick_http2socket();
/*static void test_udpecho_server(){
    fprintf(stdout, "starting udp test server\n");
    udp_echo_start(8888);

   char buffer[512];
   const char *ip="192.168.1.1";
   const char *port="8080";
   rebrick_sockaddr_t addr;
   rebrick_util_to_rebrick_sockaddr(&addr,ip,port);


   udp_echo_send2("deneme",&addr.v4);

   while(1){
     // getchar();
   int result=udp_echo_recv(buffer);
   printf("%d %s\n",result, buffer);
    usleep(1000000);
   }


   exit(0);
}

static void test_tcpecho_server(){
  fprintf(stdout,"starting tcp test server\n");
  tcp_echo_start(9191,1);
  int result;
  while(1){
    result=tcp_echo_listen();
    if(result>=0)
    break;
    usleep(1000000);
  }
  fprintf(stdout,"client connected\n");
  char buf[ECHO_BUF_SIZE]={'\0'};
  while(1){
    result=tcp_echo_recv(buf);
    if(result>=0)
    break;
    usleep(1000000);
  }

  fprintf(stdout,"readed msg:%s\n",buf);

  sprintf(buf,"%s","hello world\n");
  while(1){
    result=tcp_echo_send(buf);
    if(result>=0)
    break;
    usleep(1000000);
  }

  fprintf(stdout,"sended msg:%s\n",buf);
  tcp_echo_close_client();
  tcp_echo_close_server();
  exit(0);

}

static void test_tcpecho_client(){
  fprintf(stdout,"starting tcp test client\n");
  tcp_echo_start(9191,0);
  int result;

  fprintf(stdout,"client connected\n");
  char buf[ECHO_BUF_SIZE]={'\0'};


  sprintf(buf,"%s","hello world\n");
  while(1){
    result=tcp_echo_send(buf);
    if(result>=0)
    break;
    usleep(1000000);
  }

  fprintf(stdout,"sended msg:%s\n",buf);
  memset(buf,0,sizeof(buf));
  while(1){
    result=tcp_echo_recv(buf);
    if(result>=0)
    break;
    usleep(1000000);
  }

  fprintf(stdout,"readed msg:%s\n",buf);
  tcp_echo_close_client();
  tcp_echo_close_server();
  exit(0);

}*/

int main()
{
  fprintf(stdout, "starting test\n");
  // test_udpecho_server();
  //test_tcpecho_server();
  //test_tcpecho_client();

if (test_rebrick_filestream())
    exit(1);

  /*if (test_rebrick_util())
    exit(1);

  if(test_rebrick_resolve())
  exit(1);


  if (test_rebrick_buffer())
    exit(1);

  if (test_rebrick_buffers())
    exit(1);

  if(test_rebrick_timer())
  exit(1);

   if (test_rebrick_udpsocket())
    exit(1);

  if (test_rebrick_tcpsocket())
    exit(1);

  if (test_rebrick_tls())
    exit(1);
  if (test_rebrick_tlssocket())
    exit(1);

   if (test_rebrick_http())
    exit(1);*/


  /*if(test_rebrick_timer())
  exit(1);
  if (test_rebrick_httpsocket())
   exit(1);*/

 // if (test_rebrick_http2socket())
  //  exit(1);
  //extern void http2_socket_as_serverserver_create_get_server_push_streams(void **start);
  //http2_socket_as_serverserver_create_get_server_push_streams(NULL);
    /* kill(getpid(), SIGSEGV);
  getchar(); */


  return 0;
}