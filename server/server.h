#ifndef SER_TCP_H
#define SER_TCP_H


#define REQUEST_PORT 12345
#define BUFFER_LENGTH 1024
#define MAXPENDING 10
#define MSGHDRSIZE 8 //Message Header Size

#define HOSTNAME_LENGTH 20
#define FILENAME_LENGTH 20

typedef enum {
  REQ_SIZE = 1,
  RESP //Message type
} Type;


#define EMAIL_LENGTH 40
#define SUBJECT_LENGTH 40
#define TIMESTAMP_LENGTH 40
#define BODY_LENGTH 128
#define SH_LENGTH 40

typedef struct
{
  char temail[EMAIL_LENGTH];
  char femail[EMAIL_LENGTH];
  char Subject[SUBJECT_LENGTH];
  char timestamp[TIMESTAMP_LENGTH];
  char Body[BODY_LENGTH];
  char Server[SH_LENGTH];
  char hostname[HOSTNAME_LENGTH];
} Req;  //request


#define RESP_LENGTH 60
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

class TcpServer
{
  int serverSock, clientSock;     /* Socket descriptor for server and client*/
  struct sockaddr_in ClientAddr; /* Client address */
  struct sockaddr_in ServerAddr; /* Server address */
  unsigned short ServerPort;     /* Server port */
  int clientLen;            /* Length of Server address data structure */
  char servername[HOSTNAME_LENGTH];

 public:
  TcpServer();
  ~TcpServer();
  void start();
  static const std::string OK_RESPONSE;
  static const std::string FAIL_RESPONSE;
};

class TcpThread :public Thread
{
  int cs;
 public:
  TcpThread(int clientsocket) :cs(clientsocket) {}
  virtual void run();
  int msg_recv(int, Msg*);
  int msg_send(int, Msg*);
  unsigned long ResolveName(char name[]);
  static void err_sys(const char* fmt, ...);
  bool is_email_valid(const std::string& email);
};

#endif
