#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <windows.h>
#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <sys/stat.h>
#include <direct.h>
#include <stdlib.h>
#include <io.h>
#include <ctime>

using namespace std;
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 60
#define REQUEST_PORT 12345 //port #
#define BUFFER_LENGTH 1024
#define MSGHDRSIZE 8 //Message Header Size
#define EMAIL_LENGTH 40
#define SUBJECT_LENGTH 40
#define TIMESTAMP_LENGTH 40
#define BODY_LENGTH 128
#define SH_LENGTH 40

typedef enum {
  REQ_SIZE = 1,
  RESP //Message type
} Type;

typedef struct
{
  char to_email[EMAIL_LENGTH];
  char from_email[EMAIL_LENGTH];
  char Subject[SUBJECT_LENGTH];
  char timestamp[TIMESTAMP_LENGTH];
  char Body[BODY_LENGTH];
  char Server[SH_LENGTH];
  char hostname[HOSTNAME_LENGTH];
} Req;  //request

typedef struct
{
  char response[RESP_LENGTH];
} Resp; //response

typedef struct
{
  Type type;
  int  length; //length of effective bytes in the buffer
  char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving

class TcpClient
{
  int sock;                    /* Socket descriptor */
  struct sockaddr_in ServAddr; /* server socket address */
  unsigned short ServPort;     /* server port */
  Req* reqp;               /* pointer to request */
  Resp* respp;          /* pointer to response*/
  Msg smsg, rmsg;               /* receive_message and send_message */
  WSADATA wsadata;
 public:
  TcpClient() {}
  void run(int argc, char* argv[]);
  ~TcpClient();
  int msg_recv(int, Msg*);
  int msg_send(int, Msg*);
  unsigned long ResolveName(char name[]);
  void err_sys(const char* fmt, ...);
};


using namespace std;



void TcpClient::run(int argc, char* argv[])
{

  //initilize winsocket
  if (WSAStartup(0x0202, &wsadata) != 0)
  {
    WSACleanup();
    err_sys("Error in starting WSAStartup()\n");
  }

  smsg = { REQ_SIZE,0,{0} };
  smsg = { RESP,0,{0} };

  reqp = (Req*)smsg.buffer;

  //Display name of local host and copy it to the req
  if (gethostname(reqp->hostname, HOSTNAME_LENGTH) != 0) //get the hostname
    err_sys("can not get the host name,program exit");
  printf("Mail Client starting on host: %s\n", reqp->hostname);

  char inputserverhostname[HOSTNAME_LENGTH] = { 0 };
  char to[EMAIL_LENGTH] = { 0 };
  char from[EMAIL_LENGTH] = { 0 };
  char Subject[SUBJECT_LENGTH] = { 0 };
  char body[BODY_LENGTH] = { 0 };
  char* ts = new char[TIMESTAMP_LENGTH];
  memset(ts, 0x00, TIMESTAMP_LENGTH);

  printf("Type name of Mail server:");
  cin >> inputserverhostname;

  printf("\n");
  printf("Creating New Email.");
  cout << endl;

  printf("To:");
  cin >> to;

  printf("From:");
  cin >> from;

  printf("Subject:");
  cin.ignore();

  cin.getline(Subject, SUBJECT_LENGTH);
  printf("Body:");

  cin.getline(body, BODY_LENGTH);

  printf("\n\n");
  printf("Mail Sent to Server, waiting...\n");

  strncpy(reqp->to_email, to, EMAIL_LENGTH);
  strncpy(reqp->from_email, from, EMAIL_LENGTH);
  strncpy(reqp->Subject, Subject, SUBJECT_LENGTH);
  strncpy(reqp->Body, body, BODY_LENGTH);
  reqp = (Req*)smsg.buffer;
  time_t t;
  time(&t);
  ts = ctime(&t);
  strncpy(reqp->timestamp, ts, TIMESTAMP_LENGTH);


  //Create the socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket
    err_sys("Socket Creating Error");

  //connect to the server
  ServPort = REQUEST_PORT;
  memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
  ServAddr.sin_family = AF_INET;             /* Internet address family */
  ServAddr.sin_addr.s_addr = ResolveName(inputserverhostname);   /* Server IP address */
  ServAddr.sin_port = htons(ServPort); /* Server port */
  if (connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0)
    err_sys("Socket Creating Error");

  //send out the message
  smsg.length = sizeof(Req);
  fprintf(stdout, "Send request to %s\n", inputserverhostname);
  if (msg_send(sock, &smsg) != sizeof(Req))
    err_sys("Sending req packet error.,exit");

  //receive the response
  if (msg_recv(sock, &rmsg) != rmsg.length)
    err_sys("recv response error,exit");

  //cast it to the response structure
  respp = (Resp*)rmsg.buffer;
  printf("Response:%s\n\n\n", respp->response);
  //close the client socket

  system("pause");//wait for reply

  closesocket(sock);
}



TcpClient::~TcpClient()
{
  /* When done uninstall winsock.dll (WSACleanup()) and exit */
  WSACleanup();
}

void TcpClient::err_sys(const char* fmt, ...) //from Richard Stevens's source code
{

  perror(NULL);
  va_list args;
      va_start(args, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
      va_end(args);
  exit(1);

}

unsigned long TcpClient::ResolveName(char name[])
{
  struct hostent* host;            /* Structure containing host information */
  if ((host = gethostbyname(name)) == NULL)
    err_sys("gethostbyname() failed");
  /* Return the binary, network byte ordered address */
  return *((unsigned long*)host->h_addr_list[0]);
}

/*
   msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
   */
int TcpClient::msg_recv(int sock, Msg* msg_ptr)
{
  int rbytes, n;
  for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
    if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
      err_sys("Recv MSGHDR Error");
  for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
    if ((n = recv(sock, (char*)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
      err_sys("Recevier Buffer Error");
  return msg_ptr->length;
}
/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpClient::msg_send(int sock, Msg* msg_ptr)
{
  int n;
  if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
    err_sys("Send MSGHDRSIZE+length Error");
  return (n - MSGHDRSIZE);
}

int main(int argc, char* argv[]) //argv[1]=servername argv[2]=filename argv[3]=time/size
{
  TcpClient* tc = new TcpClient();
  tc->run(argc, argv);
  return 0;
}
