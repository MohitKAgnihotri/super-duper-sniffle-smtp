#pragma once

# pragma comment(lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <iostream>
#include <string>
#include <regex>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include "Thread.h"
#include "server.h"

const std::string TcpServer::OK_RESPONSE = "250 OK";
const std::string TcpServer::FAIL_RESPONSE = "501 Error";

TcpServer::TcpServer()
{
  WSADATA wsadata;

  if (WSAStartup(0x0202, &wsadata) != 0)
    TcpThread::err_sys("Starting WSAStartup() error\n");

  //Display name of local host

  if (gethostname(servername, HOSTNAME_LENGTH) != 0)	//get the hostname
    TcpThread::err_sys("Get the host name error,exit");

  printf("Mail Server Starting at host: %s \n waiting to be contacted for transferring Mail...\n", servername);

  //Create the server socket

  if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    TcpThread::err_sys("Create socket error,exit");

  //Fill-in Server Port and Address info.

  ServerPort = REQUEST_PORT;
  memset(&ServerAddr, 0, sizeof(ServerAddr)); /*Zero out structure */
  ServerAddr.sin_family = AF_INET; /*Internet address family */
  ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY); /*Any incoming interface */
  ServerAddr.sin_port = htons(ServerPort); /*Local port */

  //Bind the server socket

  if (bind(serverSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0)
    TcpThread::err_sys("Bind socket error,exit");

  //Successfully bind, now listen for Server requests.

  if (listen(serverSock, MAXPENDING) < 0)
    TcpThread::err_sys("Listen socket error,exit");

}

TcpServer::~TcpServer()
{
  WSACleanup();
}

void TcpServer::start()

{
  for (;;) /*Run forever */
  {
    /*Set the size of the result-value parameter */
    clientLen = sizeof(ClientAddr);

    /*Wait for a Server to connect */
    if ((clientSock = accept(serverSock, (struct sockaddr*)&ClientAddr, &clientLen)) < 0)
      TcpThread::err_sys("Accept Failed ,exit");

    /*Create a Thread for this new connection and run*/
    TcpThread* pt = new TcpThread(clientSock);
    pt->start();
  }
}

//////////////////////////////TcpThread Class	//////////////////////////////////////////
void TcpThread::err_sys(const char* fmt, ...)
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

bool  TcpThread::is_email_valid(const std::string& email)
{
  // define a regular expression
  const std::regex pattern
      ("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");

  // try to match the string with the regular expression
  return std::regex_match(email, pattern);
}

unsigned long TcpThread::ResolveName(char name[])
{
  struct hostent* host; /*Structure containing host information */
  if ((host = gethostbyname(name)) == NULL)
    err_sys("gethostbyname() failed");

  /*Return the binary, network byte ordered address */
  return *((unsigned long*)host->h_addr_list[0]);

}

/*
   msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/

int TcpThread::msg_recv(int sock, Msg* msg_ptr)

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

/*msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/

int TcpThread::msg_send(int sock, Msg* msg_ptr)
{
  int n;
  if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
    err_sys("Send MSGHDRSIZE+length Error");
  return (n - MSGHDRSIZE);
}

void TcpThread::run()	//cs: Server socket

{
  Resp* respp;	//a pointer to response
  Req* reqp;	//a pointer to the Request Packet
  Msg smsg = { REQ_SIZE,0,{0} }, rmsg = { RESP,0,{0} };	//send_message receive_message

  struct _stat stat_buf;

  int result;

  if (msg_recv(cs, &rmsg) != rmsg.length)
    err_sys("Receive Req error,exit");

  //cast it to the request packet structure

  reqp = (Req*)rmsg.buffer;
  printf("Mail Recieved from %s\n", reqp->hostname);
  printf("FROM: %s\n", reqp->from_email);
  printf("TO: %s\n", reqp->to_email);
  printf("SUBJECT:%s\n", reqp->Subject);
  printf("TIME:%s\n", reqp->timestamp);
  printf("%s\n", reqp->Body);

  //contruct the response and send it out
  smsg.type = RESP;
  smsg.length = sizeof(smsg);

  char* ts = new char[TIMESTAMP_LENGTH];
  memset(ts, 0x00, TIMESTAMP_LENGTH);

  time_t t;
  time(&t);
  ts = ctime(&t);

  if (is_email_valid(reqp->from_email) && is_email_valid(reqp->to_email))
  {
    sprintf(smsg.buffer, "%s %s%s", "Email received successfully at", ts, TcpServer::OK_RESPONSE.c_str());
  }
  else
  {
    sprintf(smsg.buffer, "%s %s%s", "Email received successfully at", ts, TcpServer::FAIL_RESPONSE.c_str());
  }

  if (msg_send(cs, &smsg) != smsg.length)
    err_sys("send Respose failed,exit");

  printf("Response for %s has been sent out \n\n\n", reqp->hostname);
  closesocket(cs);
}


int main(void)
{
  TcpServer ts;
  ts.start();
  return 0;
}
