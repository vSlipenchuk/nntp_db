#include "nntp_srv.h"
#include "vdb.h"

nntpSrv *n ;

void nntpEnumGroups(Socket *sock, nntpSrv *srv) {
int i;
//for(i=0;i<5;i++)   SocketSendf(sock,"group%d %d %d y\r\n",i, 1,100); // create 5 groups
for(i=0;i<arrLength(grp);i++) {
  SocketSendf(sock,"%s %d %d y\r\n",grp[i].name,grp[i]._min,grp[i]._max); // create 5 groups
  }
}

void nntpSetGroup(Socket *sock,char *name) {
nn_grp *g =nntp_grpByName(name);
printf("select group <%s> found=%p\n",name,g);
//SocketSendf(sock,"211 %d %d %d %s\r\n", 10, 10, 20, name ) ;; // ok, cnt, min, max, groupname

if (g) SocketSendf(sock,"211 %d %d %d %s\r\n", g->_cnt,  g->_min,g->_max,g->name); // ok, cnt, min, max, groupname
   else SocketSendf(sock,"403 no group\r\n");
sock->handle = g; // here groups
}

void nntpXover(Socket *sock,int n1,int n2);/* {
int i;

for(i=n1;i<=n2;i++) {
  int n  = i;
  char subj[80]; sprintf(subj,"subj%d",n);
  char frm[80]; sprintf(frm,"frm%d",n);
  char *date="6 Oct 2019 04:38:40 +0300";
  char msgid[256]; sprintf(msgid,"%d@mygrp",n);
  SocketSendf(sock,"%d\t%s\t%s\t%s\t%s\r\n",n,subj,frm,date,msgid);
  }

}*/

void nntpArticle(Socket *sock,char *id);/* {
// autogenerate....
char *ng="mygroup";
int n=-1; sscanf(id,"%d",&n);
char msgId[80]; sprintf(msgId,"<%d@%s>",n,ng);
char frm[80]; sprintf(frm,"Myfrom%d",n);
char subj[80]; sprintf(subj,"Mysubj%d",n);
  char *date="8 Oct 2019 04:38:40 +0300";
char body[200]; sprintf(body,"That a body of %d message",n);
SocketSendf(sock,"220\r\n");
SocketSendf(sock,"Subject: %s\r\nMessage-Id: %s\r\nFrom: %s\r\nDate: %s\r\nNewsgroups: %s\r\n\r\n",
  subj,msgId,frm,date,ng);
SocketSend(sock,body,-1);
SocketSendf(sock,"\r\n.\r\n"); // Emd of it
}*/

int nntpdb_post_test(char *t,char *set_grp) ;

void nntpPost(Socket *sock,char *body) { // Check firsled & save
printf("NEW POST {{%s}}\n",body);
int ok = nntpdb_post_test(body,0);
  buf2file((uchar*)body,strlen(body),(uchar*)"last_post.txt");
if (ok) SocketSendf(sock,"240 OK\r\n");
   else SocketSendf(sock,"530 Fail\r\n");
}



int main(int npar,char **par) {
net_init();

if (!nntpdb_init()) {
  printf("Database failed\n");
  return 0;
  }

if (npar>1) {
   char *cmd = par[1];
   if (lcmp(&cmd,"port")) {
      nntpdb_post_test(0,0);
      exit(2);
      }
  else if (lcmp(&cmd,"cmd")) { // run console
      nntpdb_console(cmd);
      exit(1);
      }
  exit(1);
  }


n = nntpSrvCreate("");
    printf("Hello world2 n=%p!\n",n);
    n->logLevel=n->srv.logLevel=10;
    n->enumGroups = nntpEnumGroups;
    n->setGroup = nntpSetGroup;
    n->xover = nntpXover;
    n->article = nntpArticle;
    n->onPost = nntpPost;
    nntpSrvListen(n, 119);
    printf("Listened!\n");
nntpSrvProcess(n);
    return 0;
}
