#ifndef NNTPSRV_H
#define NNTPSRV_H
#include "vos.h"
#include "sock.h"
#include "strutil.h"
#include "vdb.h"

typedef struct _nn_grp {
    int n;
    char name[200];
    int  _min,_max,_cnt; // stat for a group
    } nn_grp;

extern nn_grp *grp; // declared in nntp_db.c

typedef struct _nntpSrv {
    SocketPool srv; // Òóò âñå ñîêåòû (âêëþ÷àÿ ñëóøàòåëÿ)
    char name[14]; int logLevel; logger *log; // Logging
    //SocketMap **map; // Çàëèíêîâàííûå URL (ôàéëîâûå è ïðîãðàììíûå)
    //vssHttp req; // Òåêóùèé çàïðîñ íà îáðàáîòêó - ???
    //uchar *index; // Êåøèðîâàííûé èíäåêñ (îòäàåòñÿ ìãíîâåííî - èëè ñäåëàòü map?)
    //uchar *buf; // Âðåìåííûé ôàéë (äëÿ çàêà÷êè ôàéëîâ)
    //uchar *mimes;
    //httpMime *mime; // Ïàññèâíàÿ ñòðîêà è ñîáñòâåííî - ðàçîáðàííûå ìàéìû äëÿ áûñòðîãî ïîèñêà
    Counter readLimit; // Limiter for incoming counts
   // SocketPool cli; // Êëèåíòû - äëÿ ðåäèðåêòà???
   // vss defmime; // DefaultMime for a page ???
   // int keepAlive; // Disconenct after send???
    time_t created; // When it has beed created -)))
    time_t runTill;
    void *handle; // any user defined handle
   // httpAuth *defauth; // default auth for all maps
/*#ifdef HTTPSRV_AUTH
    char *realm; // report on 401 Unauthorized
    int  (*auth)(char *UserPass, struct _httpSrv *); // user:password, func must return >0 on success
    int  userId; // last auth result (app must copy it)
#endif
*/
    void (*enumGroups)(Socket *sock,struct _nntpSrv *srv);
    void (*setGroup)(Socket *sock,char *name);
    void (*xover)(Socket *sock,int n1,int n2);
    void (*article)(Socket *sock,char *id);
    void (*onPost)(Socket *sock,char *data);

    // db

    database *db; // sqlite db

    } nntpSrv;


nntpSrv *nntpSrvCreate(char *ini);
Socket *nntpSrvListen(nntpSrv *srv,int port); // 119 by default
int nntpSrvProcess(nntpSrv *srv);

// database
int nntpdb_init() ;
nn_grp *nntp_grpByName(char *name); // find by name


#endif //NNTPSRV_H

