#include "nntp_srv.h"
#include "vs0.h"
#include <string.h>

/*
  simple text-line server
*/


void nntpSrvDone() {}; // empty

VS0OBJ0(nntpSrv,nntpSrvDone);


nntpSrv *nntpSrvCreate(char *ini) { // Ñîçäàíèå ñîåäèíåíèÿ ïî óìîë÷àíèþ äëÿ http
nntpSrv *srv;
srv = nntpSrvNew();
TimeUpdate();
strcpy(srv->name,"nntpSrv"); srv->logLevel = 6; // Debug !!!
strcpy(srv->srv.name,"nntp"); srv->srv.logLevel = 6;  // LoggerName
//srv->keepAlive = 1; // Yes, keep it
srv->created = TimeNow;
srv->readLimit.Limit = 1000000; // 1 mln â ñåêóíäó -)))
//srv->maxPPS = 20; // Ïî óìîë÷àíèþ ñòîëüêî çàïðîñîâ â ñåêóíäó ïðèíèìàåòñÿ -)))
 // Ïðèìåíÿåì íàñòðîéêè ini
return srv;
}

int nntpReadyML(char *data) {
char *p;
//if (memcmp(data,"POST",4)==0) { // multline
  p = strstr(data,"\r\n.\r\n");
  if (!p) return 0;
  return (p-data)+5;
//  }
//p = strstr(data,"\r\n"); // by default - end of line
//if (!p) return 0; // no yet
//return (p-data)+2; // line with crlf
}


int nntpReady(char *data) {
char *p;
/*if (memcmp(data,"POST",4)==0) { // multline
  p = strstr(data,"\r\n.\r\n");
  if (!p) return 0;
  return (p-data)+5;
  }*/
p = strstr(data,"\r\n"); // by default - end of line
if (!p) return 0; // no yet
return (p-data)+2; // line with crlf
}


int flag = 0; // ML -- ZU - move inswde socket

int onNttpClientPacket(char *data,int len, Socket *sock) { // CHeck - if packet ready???
//hexdump("check_data",(char*)data,len);
//vssHttp req;

if (flag) len = nntpReadyML(data); // Wait multiline
   else  len = nntpReady((char*)data);
if (len<=0) return len; // Process protocol error or not ready


nntpSrv *srv = (void*)sock->pool; // Åñòü, ó ìåíÿ åñòü ñ÷åò÷èê ïàêåòîâ - ïðÿìî íà ñîêåòå???
CLOG(srv,4,"new nttpRequest#%d/%d client '%s'\n",sock->N,sock->recvNo,sock->szip);
CLOG(srv,6,"new nttpRequest#%d/%d requestBody '%s'\n",sock->N,sock->recvNo,data);
data[len-2]=0;
printf("Process:<%s>\n",data);

if (flag) { // wait nultiline
  data[len-5]=0;
  srv->onPost(sock,data);
  flag = 0; // clear multiline
  return len;
  }
if (lcmp(&data,"QUIT") || lcmp(&data,"exit")) {
  SocketSendf(sock,"250 goodbay\r\n");
  sock->dieOnSend=1;
  SocketSendDataNow(sock,"",0);
  printf("Done connection..!\n");
  }
else if (lcmp(&data,"POST")) { // got multiline resp
  SocketSendf(sock,"340 input article\r\n");
  flag=1;
  }
else if (lcmp(&data,"MODE")) { // mode reader -> hardcode 200 OKK
  SocketSendf(sock,"220 OK\r\n");
   }
else if (lcmp(&data,"LIST")) { // send list groups
  SocketSendf(sock,"215 groups\r\n");
  srv->enumGroups(sock,srv);
  SocketSendf(sock,".\r\n");
  }
else if (lcmp(&data,"GROUP")) {
  srv->setGroup(sock,data);
  //SocketSenf(sock,"")
  }
else if (lcmp(&data,"XOVER") || lcmp(&data, "OVER")) { // min-max
 int n1=-1,n2=-1;
 sscanf(data,"%d-%d",&n1,&n2);
 SocketSendf(sock,"224 overview follows\r\n");
 srv->xover(sock,n1,n2); // must end with \r\n every line!!!
 SocketSendf(sock,".\r\n");
 }
else if (lcmp(&data,"ARTICLE")) {
 srv->article(sock,data); //
 }
else {
  SocketSendf(sock,(char*)"550 command unknown: <%s>\n",data);
  }
return len; // remove from a read buffer & process...
}

int onNttpClientConnect(Socket *lsock, int handle, int ip) {
Socket *sock;
nntpSrv *srv = (void*)lsock->pool; // Here My SocketServer ???
sock = SocketPoolAccept(&srv->srv,handle,ip);
if (!sock) { // can be if wrong parameters or no pem.file
   return 0; // killed by SocketPool
   }
CLOG(srv,3,"new connect#%d from '%s', accepted\n",sock->N,sock->szip);
sock->checkPacket = onNttpClientPacket; // When new packet here...
SocketSendf(sock,"220 nntp ready\r\n",-1);
if (srv->readLimit.Limit) sock->readPacket = &srv->readLimit; // SetLimiter here (if any?)
return 1; // Everything is done...
}


Socket *nntpSrvListen(nntpSrv *srv,int port) {
Socket *sock = SocketNew();
if (SocketListen(sock,port)<=0) {
    CLOG(srv,1,"-fail listen port %d",port);
    SocketClear(&sock); return 0; } // Fail Listen a socket???
CLOG(srv,2,"+listener started on port#%d",port);
sock->onConnect = onNttpClientConnect;
Socket2Pool(sock,&srv->srv);
return sock;
}




int nntpSrvProcess(nntpSrv *srv) { // Dead loop till the end???
while(!aborted) {
  TimeUpdate(); // TimeNow & szTimeNow
  int cnt = SocketPoolRun(&srv->srv);
  //printf("SockPoolRun=%d time:%s\n",cnt,szTimeNow); msleep(1000);
  RunSleep(cnt); // Empty socket circle -)))
  if (srv->runTill && TimeNow>=srv->runTill) break; // Done???
  }
return 0;
}

