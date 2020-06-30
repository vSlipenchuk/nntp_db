#include "nntp_srv.h"


database *db;
nn_grp *grp; // MyGroups

void nn_grpDone() {}; //empty destrocroe

VS0OBJ(nn_grp);

extern int sq_connect();
int vdb_static(int,char *,void*);

nn_grp *nntp_grpByName(char *name) {
int i;
for(i=0;i<arrLength(grp);i++) if (strcmp(grp[i].name,name)==0) return grp+i;
return 0;
}

int nntp_update_state(nn_grp *g) {
int ok = db_selectf(db,"select min(n),max(n),count(*) from ng_art where ng=%d",g->n);
if (!ok) {
  printf("Select ERR, trye recreate table ERR=%s..\n",db->error);
  nntpdb_create_tables();
   ok = db_selectf(db,"select min(n),max(n),count(*) from ng_art where ng=%d",g->n);
   if (!ok) {
      printf("Failed select anyway %s\n",db->error);
      exit(2);
     }
  }
// ok - need fetch
if (db_fetch(db) ) {
 g->_min= db_int(db->out.cols);
 g->_max= db_int(db->out.cols+1);
 g->_cnt= db_int(db->out.cols+2);
 }
else {
 g->_min = g->_max = g->_cnt = 0;
 }
return 1;
}

int nntp_update_states() { // update all groups
int i;
for(i=0;i<arrLength(grp);i++) nntp_update_state(grp+i);
return 1;
}

int nntpdb_create_tables() {
int ok = db_exec_once(db,"create table ng_grp(N integer,NAME varchar(80))") && db_commit(db);
printf("ng_grp table created:%d status:%s\n",ok,db->error);
ok = db_exec_once(db,"create table ng_art(n integer not null primary key, ng integer,"
    "subj varchar(200), frm varchar(200), h varchar(2000),body varchar(2000), r integer, created TIMESTAMP default CURRENT_TIMESTAMP)");
printf("ng_art table created:%d status:%s\n",ok,db->error);
return 1;
}



int nntpdb_init() { // connect or create a new db
if (!db) {
 db = db_new();
 vdb_static(0,"sqlite",sq_connect);
 }
if (db_connect_string(db,"/@nntp.db")<=0) {
  printf("Connect DB Failed %s!\n",db->error);
  return 0;
  }
// need reload groups from database?
if ( db_select(db,"select N,NAME from ng_grp order by NAME")<=0) {
  printf("No groups table (%s), create new\n",db->error);
  nntpdb_create_tables();
  exit(1); // done
  }
while (db_fetch(db) ) {
  int n = db_int(db->out.cols);
  char *name = db_text(db->out.cols+1);
  printf("N=%d, NAME=%s\n",n,name);
  nn_grp *g = nn_grpAdd(&grp,0); // createw new instance
  g->n = n;
  strNcpy(g->name,name);
}
printf("%d groups here\n",arrLength(grp));
nntp_update_states();
return 1;
}

int GMT = 0;
extern char *   dt_mon_name[13];
char *dt_nntp(char *buf, char *dat) { //"8 Oct 2019 04:38:40 +0300";
  int Year,Month,Day,Hour,Min,Sec;
  printf("IN_DATE: <%s>\n",dat);
  date_time D = dt_scanf(dat,strlen(dat));
  dt_decode(D,&Year,&Month,&Day,&Hour,&Min,&Sec);
  sprintf(buf,"%d %s %04d %02d:%02d:%02d +%02d00",Day, dt_mon_name[Month], Year,
          Hour,Min,Sec, GMT);
printf("OUT_DATE: <%s>\n",buf);
  return buf;
}

char *safe(char *r) {
char *s = r;
while(*r) {
  if (*r=='\r') *r=' ';
  if (*r=='\n') *r=' ';
  if (*r=='\t') *r=' ';
   r++;
  }
return s;
}

void nntpXover(Socket *sock,int n1,int n2) {
//int i;
nn_grp *g = sock->handle; // selected group
if (g) {
  int ok = db_selectf(db,
    "select n,subj,frm,created,r from ng_art where ng=%d and n between %d and %d",g->n,n1,n2);
  if (ok) while (db_fetch(db)) {
      int n = db_int(db->out.cols);
      char *subj = db_text(db->out.cols+1);
      char *frm = db_text(db->out.cols+2);

      subj = safe( trim(subj) );
      frm = trim(frm);
      //char *date = db_text(db->out.cols+3) ; // Need format !!!
      char date[80];
      date[0]=0;
      dt_nntp(date, db_text(db->out.cols+3) );
      int r = db_int(db->out.cols+4);
      char Ref[80]; Ref[0]=0;
      if (r>0) sprintf(Ref,"%d@%s",r,g->name); // references

      //date[0]=0; // TMP?
printf("REF={{%s}}\n",Ref);
     // subj="subj";
      //frm="frm"; strcpy(date,"6 Oct 2019 04:38:40 +0300");
      SocketSendf(sock,"%d\t%s\t%s\t%s\t%d@%s\t%s\t%d\t%d\r\n",n,subj,frm,date,n,g->name,
          Ref,0,0); // references, bytes, lines

        printf("XOVER_SEND: %d\t%s\t%s\t%s\t%d@%s\t%s\t%d\t%d\r\n",n,subj,frm,date,n,g->name,
          Ref,0,0); // references, bytes, lines
      //printf("SEND:%d\t%s\t%s\t%s\t%d@%s\r\n",n,subj,frm,date,n,g->name);
      }

  }
/*
for(i=n1;i<=n2;i++) {
  int n  = i;
  char subj[80]; sprintf(subj,"subj%d",n);
  char frm[80]; sprintf(frm,"frm%d",n);
  char *date="6 Oct 2019 04:38:40 +0300";
  char msgid[256]; sprintf(msgid,"%d@mygrp",n);
  SocketSendf(sock,"%d\t%s\t%s\t%s\t%s\r\n",n,subj,frm,date,msgid);
  }
  */
}

void nntpArticle(Socket *sock,char *id) {
nn_grp *g = sock->handle; // selected group
if (!g) return ; // failed found
// autogenerate....
char *ng=g->name;
int n=-1; sscanf(id,"%d",&n); //
int ok = db_selectf(db,"select frm,subj,h,body,created,r from ng_art where ng=%d and n=%d",g->n,n);
if (ok && db_fetch(db )) {
char msgId[80]; sprintf(msgId,"<%d@%s>",n,ng);
char *frm = db_text(db->out.cols);
char *subj=db_text(db->out.cols+1);
char *heads =  db_text(db->out.cols+2);
char *body = db_text(db->out.cols+3);
int r = db_int(db->out.cols+4);
char Ref[200]; Ref[0]=0;
//if (r>0) sprintf(Ref,"References: <%d@%s>\r\n",r,g->name);
//char *date = db_text(db->out.cols+4);
    char date[80];
      dt_nntp(date, db_text(db->out.cols+4) );

//char frm[80]; sprintf(frm,"Myfrom%d",n);
//char subj[80]; sprintf(subj,"Mysubj%d",n);
  //char *date="8 Oct 2019 04:38:40 +0300";
//char body[200]; sprintf(body,"That a body of %d message",n);
SocketSendf(sock,"220\r\n");
SocketSendf(sock,"%s%s%sSubject: %s\r\nMessage-Id: %s\r\nFrom: %s\r\nDate: %s\r\nNewsgroups: %s\r\n\r\n",
  heads,(heads[0]?"\r\n":""),Ref,subj,msgId,frm,date,ng);
SocketSend(sock,body,-1);
SocketSendf(sock,"\r\n.\r\n"); // Emd of it
}
}

#include "vss.h"


int get_mime_field(char *h,char *field,char *buf, int sz) {
vss H = vssCreate(h,strlen(h));
vss f;
memset(buf,0,sz);
if (!vssFindMimeHeader(&H,(uchar*)field,&f)) return 0;
if (f.len>=sz-1) f.len=sz-1;
memcpy(buf,f.data,f.len);
return 1;
}

int nntpdb_add_post(int g,char *f,char *s,char *h,char *b,int r) {
int n = 1;
if (db_select(db,"select max(n) from ng_art") && db_fetch(db)) n = db_int(db->out.cols)+1;
int ok = db_compile(db,"insert into ng_art(n,ng,subj,frm,h,body,r) values(:n,:g,:s,:f,:h,:b,:r)")
   && db_bind(db,"n",dbInt, 0, &n, 0)
   && db_bind(db,"g",dbInt, 0, &g, 0)
   && db_bind(db,"f",dbChar, 0, f, strlen(f))
   && db_bind(db,"s",dbChar, 0, s, strlen(s))
   && db_bind(db,"h", dbChar, 0, h, strlen(h))
   && db_bind(db,"b", dbChar, 0, b, strlen(b))
   && db_bind(db,"r",dbInt, 0, &r, 0)
   && db_exec(db);
db_commit(db);
if (ok) {
     printf("Atricle %d added ok\n",n);
     nntp_update_states(); // update cash min-max
     }
 else {printf("Article %d failed %s\n",n,db->error); n=0;}
return n;
}

// make a new post here
int nntpdb_post_test(char *t,char *set_grp) {
if (!t) t = (char*)strLoad((uchar*)"post.txt"); // last report
char heads[512];
printf("Try insert = {{%s}}\n",t);
char *b = strstr(t,"\r\n\r\n"); // where body begin?
if (b) {*b=0; b+=4;} else
  {
 // b = strstr(t,"\n\n"); // ZU ! - failed !!!
  if (b) { *b=0; b+=2;} else b=""; // header-body split
  }
printf("Heads:{{%s}} Body:{{%s}}\n",t,b);

char ng_name[256], subj[256],frm[256],Dat[256],Ref[80];
get_mime_field(t,"Newsgroups",ng_name,sizeof(ng_name));
get_mime_field(t,"Subject",subj,sizeof(subj));
get_mime_field(t,"From",frm,sizeof(frm));
get_mime_field(t,"Date",Dat,sizeof(Dat));
//get_mime_field(t,"References",Ref,sizeof(Ref));
get_mime_field(t,"In-Reply-To",Ref,sizeof(Ref));

int ref = 0; if (Ref[0]=='<') sscanf(Ref+1,"%d",&ref);
heads[0]=0;
char buf[200];
if (get_mime_field(t,"MIME-Version",buf,sizeof(buf))) {
   strcat(heads,"MIME-Version"); strcat(heads,": "); strcat(heads,buf); strcat(heads,"\r\n");
   }
if (get_mime_field(t,"Content-Type",buf,sizeof(buf))) {
   strcat(heads,"Content-Type"); strcat(heads,": "); strcat(heads,buf);  strcat(heads,"\r\n");
   }
if (get_mime_field(t,"Content-Transfer-Encoding",buf,sizeof(buf))) {
   strcat(heads,"Content-Transfer-Encoding"); strcat(heads,": "); strcat(heads,buf); strcat(heads,"\r\n");
   }
printf("NG=%s, SUBJ=%s From=%s Date=%s Heads={{%s}}\n",ng_name,subj,frm,Dat,heads);
if (!set_grp)  set_grp = ng_name;
nn_grp * g = nntp_grpByName(set_grp);
if (!g) {
   printf("Cant find group <%s>\n",set_grp);
   return 0;
   }
nntpdb_add_post(g->n,safe(frm),safe(subj),heads,b,ref); //int g,char *f,char *s,char *h,char *b) {
return 1;
}

int db_next_n(database *db,char *tbl) {
if (db_selectf(db,"select max(n) from %s",tbl)<=0) return -1;
if (db_fetch(db)<=0) return 1;
return db_int(db->out.cols)+1;
}

int nntpdb_addgrp(char *name) {
int n = db_next_n(db,"ng_grp");
if (db_execf(db,"insert into ng_grp(n,name) values(%d,'%s')",n,name))
   printf("OK, added n=%d\n",n);
   else printf("failed with code %s\n",db->error);
db_commit(db);
return 0;
}

int nntpdb_console(char *cmd) {
printf("run console command: %s\n",cmd);
if (lcmp(&cmd,"addgrp")) {
   printf("add group %s\n",cmd);
   nntpdb_addgrp(cmd);
   }
return 0;
}
