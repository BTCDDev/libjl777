//
//  kademlia.h
//  libjl777
//
//  Created by jl777 on 10/3/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef libjl777_kademlia_h
#define libjl777_kademlia_h

#define KADEMLIA_MINTHRESHOLD 30
#define KADEMLIA_MAXTHRESHOLD 32

#define KADEMLIA_ALPHA 7
#define NODESTATS_EXPIRATION 600
#define KADEMLIA_BUCKET_REFRESHTIME 3600
#define KADEMLIA_NUMBUCKETS ((int)(sizeof(K_buckets)/sizeof(K_buckets[0])))
#define KADEMLIA_NUMK ((int)(sizeof(K_buckets[0])/sizeof(K_buckets[0][0])))

struct storage_queue_entry { uint64_t keyhash,destbits; int32_t selector; };

struct nodestats *K_buckets[64+1][7];
long Kbucket_updated[KADEMLIA_NUMBUCKETS];
uint64_t Allnodes[10000];
int32_t Numallnodes;
#define MAX_ALLNODES ((int32_t)(sizeof(Allnodes)/sizeof(*Allnodes)))

void update_Allnodes()
{
    int32_t i;
    char NXTaddr[64];
    struct nodestats *stats,*sp;
    for (i=0; i<MAX_ALLNODES; i++)
    {
        if ( Allnodes[i] != 0 )
        {
            stats = get_nodestats(Allnodes[i]);
            expand_nxt64bits(NXTaddr,Allnodes[i]);
            if ( (sp= (struct nodestats *)find_storage(NODESTATS_DATA,NXTaddr,0)) != 0 )
            {
                if ( memcmp(sp,stats,sizeof(*sp)) != 0 )
                    update_nodestats_data(stats);
                free(sp);
            }
        }
    }
}

struct nodestats *find_nodestats(uint64_t nxt64bits)
{
    int32_t i;
    if ( nxt64bits != 0 && Numallnodes > 0 )
    {
        for (i=0; i<MAX_ALLNODES; i++)
        {
            if ( Allnodes[i] == nxt64bits )
                return(get_nodestats(Allnodes[i]));
        }
    }
    return(0);
}

void add_new_node(uint64_t nxt64bits)
{
    if ( nxt64bits != 0 && find_nodestats(nxt64bits) == 0 )
    {
        if ( Debuglevel > 0 )
            printf("[%d of %d] ADDNODE.%llu\n",Numallnodes,MAX_ALLNODES,(long long)nxt64bits);
        Allnodes[Numallnodes % MAX_ALLNODES] = nxt64bits;
        Numallnodes++;
    }
}

struct nodestats *get_random_node()
{
    struct nodestats *stats;
    int32_t n;
    if ( Numallnodes > 0 )
    {
        n = (Numallnodes < MAX_ALLNODES) ? Numallnodes : MAX_ALLNODES;
        if ( (stats= get_nodestats(Allnodes[(rand()>>8) % n])) != 0 )
            return(stats);
    }
    return(0);
}

int32_t calc_np_dist(struct NXT_acct *np,struct NXT_acct *destnp)
{
    uint64_t a,b;
    a = calc_nxt64bits(np->H.U.NXTaddr);
    b = calc_nxt64bits(destnp->H.U.NXTaddr);
    return(bitweight(a ^ b));
}

int32_t verify_addr(uint16_t *prevportp,char *previpaddr,char *refipaddr,int32_t refport)
{
    //char ipaddr[64];
    //*prevportp = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    if ( strcmp(previpaddr,refipaddr) != 0 )//|| refport != port )
    {
        printf("verify_addr error: (%s) vs (%s)\n",refipaddr,previpaddr);
        strcpy(refipaddr,previpaddr);
        return(-1);
    }
    return(0);
}

int32_t ismynxtbits(uint64_t destbits)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        if ( destbits == cp->privatebits || destbits == cp->srvpubnxtbits )
            return(1);
        else return(0);
    } else return(-1);
}

int32_t ismyipaddr(char *ipaddr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        if ( notlocalip(ipaddr) == 0 || strcmp(cp->myipaddr,ipaddr) == 0 )
            return(1);
        else return(0);
    } else return(-1);
}

int32_t ismynode(struct sockaddr *addr)
{
    char ipaddr[64];
    int32_t port;
    if ( addr == 0 )
        return(1);
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    return(ismyipaddr(ipaddr));
}

uint32_t addto_hasips(int32_t recalc_flag,struct pserver_info *pserver,uint32_t ipbits)
{
    int32_t i,n;
    uint32_t xorsum = 0;
    if ( ipbits == 0 )
        return(0);
    n = (pserver->numips < (int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips))) ? pserver->numips : (int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips));
    if ( pserver->numips > 0 )
    {
        for (i=0; i<pserver->numips; i++)
        {
            if ( pserver->hasips[i] == ipbits )
                return(0);
        }
    }
    {
        char ipstr[64];
        expand_ipbits(ipstr,ipbits);
        if ( Debuglevel > 0 )
            printf("addto_hasips %p n.%d num.%d <- %x %s\n",pserver->hasips,n,pserver->numips,ipbits,ipstr);
    }
    pserver->hasips[n % (int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips))] = ipbits;
    pserver->numips++;
    n++;
    if ( recalc_flag != 0 )
    {
        for (i=0; i<n; i++)
            xorsum ^= pserver->hasips[i];
        pserver->xorsum = xorsum;
        pserver->hasnum = pserver->numips;
    }
    return(xorsum);
}

void addto_hasnxt(struct pserver_info *pserver,uint64_t nxtbits)
{
    int32_t i,n;
    if ( nxtbits == 0 )
        return;
    n = (pserver->numnxt < (int)(sizeof(pserver->hasnxt)/sizeof(*pserver->hasnxt))) ? pserver->numnxt : (int)(sizeof(pserver->hasnxt)/sizeof(*pserver->hasnxt));
    if ( pserver->numnxt > 0 )
    {
        for (i=0; i<pserver->numnxt; i++)
        {
            if ( pserver->hasnxt[i] == nxtbits )
                return;
        }
    }
    pserver->hasnxt[n % (int)(sizeof(pserver->hasnxt)/sizeof(*pserver->hasnxt))] = nxtbits;
    pserver->numnxt++;
}

int32_t sort_all_buckets(uint64_t *sortbuf,uint64_t hash)
{
    struct nodestats *stats;
    struct pserver_info *pserver;
    int32_t i,j,n;
    char ipaddr[32];
    for (i=n=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        for (j=0; j<KADEMLIA_NUMK; j++)
        {
            if ( (stats= K_buckets[i][j]) == 0 )
                break;
            if ( stats->ipbits != 0 )
            {
                expand_ipbits(ipaddr,stats->ipbits);
                pserver = get_pserver(0,ipaddr,0,0);
                if ( pserver->decrypterrs == 0 )
                {
                    sortbuf[n<<1] = bitweight(stats->nxt64bits ^ hash);// + ((stats->gotencrypted == 0) ? 64 : 0);
                    sortbuf[(n<<1) + 1] = stats->nxt64bits;
                    n++;
                }
            }
        }
    }
    if ( n == 0 )
        return(0);
    else sort64s(sortbuf,n,sizeof(*sortbuf)*2);
    return(n);
}

int32_t calc_bestdist(uint64_t keyhash)
{
    int32_t i,j,n,dist,bestdist = 10000;
    struct nodestats *stats;
    for (i=n=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        for (j=0; j<KADEMLIA_NUMK; j++)
        {
            if ( (stats= K_buckets[i][j]) == 0 )
                break;
            dist = bitweight(stats->nxt64bits ^ keyhash);
            if ( dist < bestdist )
                bestdist = dist;
        }
    }
    return(bestdist);
}

uint64_t _send_kademlia_cmd(int32_t encrypted,struct pserver_info *pserver,char *cmdstr,char *NXTACCTSECRET,unsigned char *data,int32_t datalen)
{
    int32_t len = (int32_t)strlen(cmdstr);
    char _tokbuf[4096];
    uint64_t txid;
    if ( strcmp("0.0.0.0",pserver->ipaddr) == 0 )
    {
        printf("_send_kademlia_cmd illegal ip addr %s\n",pserver->ipaddr);
        return(0);
    }
    len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    if ( Debuglevel > 1 )
        printf(">>>>>>>> directsend.[%s] encrypted.%d -> (%s)\n",_tokbuf,encrypted,pserver->ipaddr);
    txid = directsend_packet(encrypted,pserver,_tokbuf,len,data,datalen);
    return(txid);
}

uint8_t *replace_datafield(char *cmdstr,uint8_t *databuf,int32_t *datalenp,char *datastr)
{
    int32_t len = 0;
    uint8_t *data = 0;
    if ( datastr != 0 && datastr[0] != 0 )
    {
        len = (int32_t)strlen(datastr);
        if ( len < 2 || (len & 1) != 0 || is_hexstr(datastr) == 0 )
        {
            if ( datastr[0] == '[' )
                sprintf(cmdstr+strlen(cmdstr),",\"data\":%s",datastr);
            else sprintf(cmdstr+strlen(cmdstr),",\"data\":\"%s\"",datastr);
            len = 0;
        }
        else
        {
            len >>= 1;
            sprintf(cmdstr+strlen(cmdstr),",\"data\":%d",len);
            data = databuf;
            decode_hex(data,len,datastr);
        }
    }
    *datalenp = len;
    return(data);
}

int32_t gen_pingstr(char *cmdstr,int32_t completeflag)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        sprintf(cmdstr,"{\"requestType\":\"ping\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"ver\":\"%s\"",cp->srvNXTADDR,(long)time(NULL),Global_mp->pubkeystr,cp->myipaddr,HARDCODED_VERSION);
        if ( completeflag != 0 )
            strcat(cmdstr,"}");
        return((int32_t)strlen(cmdstr));
    } else return(0);
}

void send_to_ipaddr(char *ipaddr,char *jsonstr,char *NXTACCTSECRET)
{
    char _tokbuf[MAX_JSON_FIELD];
    struct pserver_info *pserver;
    struct sockaddr destaddr;
    struct nodestats *stats;
    int32_t port;
    pserver = get_pserver(0,ipaddr,0,0);
    stats = get_nodestats(pserver->nxt64bits);
    if ( stats != 0 )
        port = stats->supernet_port;
    else port = 0;
    if ( port == 0 )
        port = SUPERNET_PORT;
    uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)&destaddr);
    construct_tokenized_req(_tokbuf,jsonstr,NXTACCTSECRET);
    portable_udpwrite(0,(struct sockaddr *)&destaddr,Global_mp->udp,_tokbuf,strlen(_tokbuf),ALLOCWR_ALLOCFREE);
}

uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *datastr)
{
    //char _tokbuf[MAX_JSON_FIELD];
    //struct sockaddr_in destaddr;
    int32_t i,encrypted,dist,createdflag,len = 0;
    struct nodestats *stats;
    struct NXT_acct *np;
    uint64_t keybits;
    unsigned char databuf[32768],*data = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    char pubkeystr[1024],ipaddr[64],cmdstr[2048],verifiedNXTaddr[64],destNXTaddr[64];
    if ( NXTACCTSECRET[0] == 0 || cp == 0 )
    {
        //C SuperNET_gotpacket.([{"requestType":"ping","NXT":"17572279667799017517","time":1413706981,"pubkey":"0514b7ba1da50363f3ad19ef46611754a1beb9815b9828bbc2ba9f0ea8f73f75","ipaddr":"89.212.19.49"},{"token":"3mtbe505lnpm60t52tgkdropg6srt8akoatih62ruuk0t7tqm65act9vu1v0n1g1hk7aiq2e88b0rp536o25rvkeibck3dmhmq68jc5bqb6gape8c2k21frtcdtq26pku0amfvdcvjrbcihed0jce1u0cq93nvoi"}]) from 89.212.19.49:47717 size.344 ascii txid.14809915081856697906 | flood.0

        printf("send_kademlia_cmd.%s srvpubaddr or cp.%p\n",kadcmd,cp);
        strcpy(NXTACCTSECRET,cp->srvNXTACCTSECRET);
    }
    init_hexbytes_noT(pubkeystr,Global_mp->loopback_pubkey,sizeof(Global_mp->loopback_pubkey));
    verifiedNXTaddr[0] = 0;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    //printf("send_kademlia_cmd (%s) [%s]\n",verifiedNXTaddr,NXTACCTSECRET);
    if ( pserver == 0 )
    {
        expand_nxt64bits(destNXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
        expand_ipbits(ipaddr,np->stats.ipbits);
        pserver = get_pserver(0,ipaddr,0,0);
        if ( pserver->nxt64bits == 0 )
            pserver->nxt64bits = nxt64bits;
    } else nxt64bits = pserver->nxt64bits;
    strcpy(ipaddr,pserver->ipaddr);
    if ( strcmp(kadcmd,"ping") != 0 && nxt64bits == 0 )
    {
        printf("send_kademlia_cmd.(%s) No destination\n",kadcmd);
        return(0);
    }
    else if ( strcmp(kadcmd,"store") == 0 || strncmp("find",kadcmd,4) == 0 ) //
    {
        static int lasti;
        static uint64_t txids[8192]; // filter out localloops, maybe not needed anymore?
        bits256 hash;
        uint64_t txid;
        keybits = calc_nxt64bits(key);
        dist = bitweight(keybits ^ nxt64bits);
        //if ( dist <= KADEMLIA_MAXTHRESHOLD )
        {
            if ( datastr != 0 )
                calc_sha256cat(hash.bytes,(uint8_t *)key,(int32_t)strlen(key),(uint8_t *)datastr,(int32_t)strlen(datastr));
            else calc_sha256(0,hash.bytes,(uint8_t *)key,(int32_t)strlen(key));
            txid = (hash.txid ^ nxt64bits);
            for (i=0; i<(int)(sizeof(txids)/sizeof(*txids)); i++)
            {
                if ( txids[i] == 0 )
                    break;
                else if ( txids[i] == txid )
                {
                    if ( Debuglevel > 2 )
                        printf("send_kademlia_cmd.(%s): duplicate txid.%llu to %llu in slot.%d lasti.%d\n",kadcmd,(long long)txid,(long long)nxt64bits,i,lasti);
                    return(0);
                }
            }
            if ( i == (int)(sizeof(txids)/sizeof(*txids)) )
            {
                i = lasti++;
                if ( lasti >= (int)(sizeof(txids)/sizeof(*txids)) )
                    lasti = 0;
            }
            txids[i] = txid;
        }
    }
    if ( 0 && (pserver->nxt64bits == cp->privatebits || pserver->nxt64bits == cp->srvpubnxtbits) )
    {
        printf("no point to send yourself (%s) dest.%llu pub.%llu srvpub.%llu\n",kadcmd,(long long)pserver->nxt64bits,(long long)cp->privatebits,(long long)cp->srvpubnxtbits);
        return(0);
    }
    encrypted = 2;
    stats = get_nodestats(pserver->nxt64bits);
    /*if ( stats != 0 )
        port = stats->supernet_port;
    else port = 0;
    if ( port == 0 )
        port = SUPERNET_PORT;
    uv_ip4_addr(pserver->ipaddr,port,&destaddr);*/
    if ( strcmp(kadcmd,"ping") == 0 )
    {
        encrypted = 0;
        if ( stats != 0 )
        {
            stats->pingmilli = milliseconds();
            stats->numpings++;
        }
        gen_pingstr(cmdstr,1);
        send_to_ipaddr(pserver->ipaddr,cmdstr,NXTACCTSECRET);
        //len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
        //portable_udpwrite(0,(struct sockaddr *)&destaddr,Global_mp->udp,_tokbuf,strlen(_tokbuf),ALLOCWR_ALLOCFREE);
        return(0);
        //sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"ver\":\"%s\"",kadcmd,verifiedNXTaddr,(long)time(NULL),pubkeystr,cp->myipaddr,HARDCODED_VERSION);
    }
    else
    {
        if ( strcmp(kadcmd,"pong") == 0 )
        {
            encrypted = 1;
            sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"yourip\":\"%s\",\"yourport\":%d,\"ipaddr\":\"%s\",\"pubkey\":\"%s\",\"ver\":\"%s\"",kadcmd,verifiedNXTaddr,(long)time(NULL),pserver->ipaddr,pserver->port,cp->myipaddr,pubkeystr,HARDCODED_VERSION);
            //send_to_ipaddr(pserver->ipaddr,cmdstr,NXTACCTSECRET);
            //len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
            //portable_udpwrite(0,(struct sockaddr *)&destaddr,Global_mp->udp,_tokbuf,strlen(_tokbuf),ALLOCWR_ALLOCFREE);
        }
        else
        {
            if ( strncmp(kadcmd,"have",4) == 0 )
                encrypted = 1;
            else if ( (strcmp(kadcmd,"store") == 0 || strcmp(kadcmd,"findnode") == 0) && datastr != 0 && datastr[0] != 0 )
                encrypted = 4;
            sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld",kadcmd,verifiedNXTaddr,(long)time(NULL));
        }
    }
    if ( key != 0 && key[0] != 0 )
        sprintf(cmdstr+strlen(cmdstr),",\"key\":\"%s\"",key);
    data = replace_datafield(cmdstr,databuf,&len,datastr);
    strcat(cmdstr,"}");
    return(_send_kademlia_cmd(encrypted,pserver,cmdstr,NXTACCTSECRET,data,len));
}

void kademlia_update_info(char *destNXTaddr,char *ipaddr,int32_t port,char *pubkeystr,uint32_t lastcontact,int32_t p2pflag)
{
    uint64_t nxt64bits;
    uint32_t changed,ipbits = 0;
    struct pserver_info *pserver;
    struct nodestats *stats = 0;
    if ( destNXTaddr != 0 && destNXTaddr[0] != 0 )
        nxt64bits = calc_nxt64bits(destNXTaddr);
    else nxt64bits = 0;
    changed = 0;
    if ( port == BTCD_PORT && p2pflag == 0 )
    {
        printf("warning: kademlia_update_info port is %d?\n",port);
        port = 0;
    }
    if ( ipaddr != 0 && ipaddr[0] != 0 )
    {
        ipbits = calc_ipbits(ipaddr);
        pserver = get_pserver(0,ipaddr,p2pflag==0?port:0,p2pflag!=0?port:0);
        if ( nxt64bits != 0 && (pserver->nxt64bits == 0 || pserver->nxt64bits != nxt64bits) )
        {
            printf("kademlia_update_info: pserver nxt64bits %llu -> %llu\n",(long long)pserver->nxt64bits,(long long)nxt64bits);
            pserver->nxt64bits = nxt64bits;
            changed++;
        }
    }
    if ( nxt64bits != 0 )
    {
        stats = get_nodestats(nxt64bits);
        if ( stats->nxt64bits == 0 || stats->nxt64bits != nxt64bits )
        {
            printf("kademlia_update_info: nxt64bits %llu -> %llu\n",(long long)stats->nxt64bits,(long long)nxt64bits);
            stats->nxt64bits = nxt64bits;
            changed++;
        }
        if ( ipbits != 0 && (stats->ipbits == 0 || stats->ipbits != ipbits) )
        {
            printf("kademlia_update_info: stats ipbits %u -> %u\n",stats->ipbits,ipbits);
            stats->ipbits = ipbits;
            changed++;
        }
        if ( port != 0 )
        {
            if ( p2pflag != 0 )
            {
                if ( stats->p2pport == 0 || stats->p2pport != port )
                {
                    printf("kademlia_update_info: p2pport %u -> %u\n",stats->p2pport,port);
                    stats->p2pport = port;
                    changed++;
                }
            }
            else
            {
                if ( stats->supernet_port == 0 || stats->supernet_port != port )
                {
                    printf("kademlia_update_info: supernet_port %u -> %u\n",stats->supernet_port,port);
                    stats->supernet_port = port;
                    changed++;
                }
            }
        }
        if ( pubkeystr != 0 && pubkeystr[0] != 0 && update_pubkey(stats->pubkey,pubkeystr) != 0 && lastcontact != 0 )
            stats->lastcontact = lastcontact, changed++;
        if ( changed != 0 )
            update_nodestats_data(stats);
    }
}

void change_nodeinfo(char *ipaddr,uint16_t port,uint64_t nxt64bits)
{
    struct nodestats *stats;
    struct pserver_info *pserver;
    stats = get_nodestats(nxt64bits);
    stats->ipbits = calc_ipbits(ipaddr);
    if ( port != 0 )//(stats->supernet_port == 0 || stats->supernet_port == SUPERNET_PORT) )
    {
        printf("OVERRIDE supernet_port.%d -> %d\n",stats->supernet_port,port);
        stats->supernet_port = port;
    }
    pserver = get_pserver(0,ipaddr,port,0);
    pserver->nxt64bits = nxt64bits;
    add_new_node(nxt64bits);
}

void set_myipaddr(struct coin_info *cp,char *ipaddr,uint16_t port)
{
    strcpy(cp->myipaddr,ipaddr);
    change_nodeinfo(ipaddr,port,cp->srvpubnxtbits);
}

char *kademlia_ping(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *ipaddr,int32_t port,char *destip,char *origargstr)
{
    uint64_t txid = 0;
    char retstr[1024];
    uint16_t prevport;
    struct coin_info *cp = get_coin_info("BTCD");
    //printf("got ping.%d (%s)\n",ismynode(prevaddr),origargstr);
    retstr[0] = 0;
    if ( is_remote_access(previpaddr) == 0 ) // user invoked
    {
        if ( destip != 0 && destip[0] != 0 )
        {
            if ( ismyipaddr(destip) == 0 )
                txid = send_kademlia_cmd(0,get_pserver(0,destip,0,0),"ping",NXTACCTSECRET,0,0);
            sprintf(retstr,"{\"result\":\"kademlia_ping to %s\",\"txid\":\"%llu\"}",destip,(long long)txid);
        }
        else sprintf(retstr,"{\"error\":\"kademlia_ping no destip\"}");
    }
    else // sender ping'ed us
    {
        if ( notlocalip(cp->myipaddr) == 0 && destip[0] != 0 && calc_ipbits(destip) != 0 )
        {
            printf("AUTO SETTING MYIP <- (%s)\n",destip);
            set_myipaddr(cp,ipaddr,port);
        }
        prevport = 0;
        if ( verify_addr(&prevport,previpaddr,ipaddr,port) < 0 ) // auto-corrects ipaddr
        {
            change_nodeinfo(ipaddr,prevport,calc_nxt64bits(sender));
            sprintf(retstr,"{\"error\":\"kademlia_ping from %s doesnt verify (%s) -> new IP (%s:%d)\"}",sender,origargstr,ipaddr,prevport);
        }
        else sprintf(retstr,"{\"result\":\"kademlia_pong to (%s/%d)\",\"txid\":\"%llu\"}",ipaddr,prevport,(long long)txid);
        txid = send_kademlia_cmd(0,get_pserver(0,ipaddr,prevport,0),"pong",NXTACCTSECRET,0,0);
    }
    if ( is_remote_access(previpaddr) != 0 && retstr[0] != 0 )
        printf("PING.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *kademlia_pong(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *ipaddr,uint16_t port,char *yourip,int32_t yourport)
{
    char retstr[1024];
    struct nodestats *stats;
    struct coin_info *cp = get_coin_info("BTCD");
    stats = get_nodestats(calc_nxt64bits(sender));
    // all the work is already done in update_Kbucket
    if ( cp != 0 && notlocalip(cp->myipaddr) == 0 && yourip[0] != 0 && calc_ipbits(yourip) != 0 )
    {
        printf("AUTOUPDATE IP <= (%s)\n",yourip);
        set_myipaddr(cp,yourip,yourport);
    }
    if ( stats != 0 )
    {
        stats->pongmilli = milliseconds();
        stats->pingpongsum += (stats->pongmilli - stats->pingmilli);
        stats->numpongs++;
        sprintf(retstr,"{\"result\":\"kademlia_pong\",\"NXT\":\"%s\",\"ipaddr\":\"%s\",\"port\":%d\",\"lag\":%.3f,\"numpings\":%d,\"numpongs\":%d,\"ave\":%.3f\"}",sender,ipaddr,port,stats->pongmilli-stats->pingmilli,stats->numpings,stats->numpongs,(2*stats->pingpongsum)/(stats->numpings+stats->numpongs+1));
    }
    else sprintf(retstr,"{\"result\":\"kademlia_pong\",\"NXT\":\"%s\",\"ipaddr\":\"%s\",\"port\":%d\"}",sender,ipaddr,port);
    //if ( Debuglevel > 0 )
        printf("PONG.(%s)\n",retstr);
    return(clonestr(retstr));
}

struct SuperNET_storage *kademlia_getstored(int32_t selector,uint64_t keyhash,char *datastr)
{
    char key[64];
    struct SuperNET_storage *sp;
    expand_nxt64bits(key,keyhash);
    sp = (struct SuperNET_storage *)find_storage(selector,key,0);
    if ( datastr == 0 )
        return(sp);
    if ( sp != 0 )
        free(sp);
    add_storage(selector,key,datastr);
    return((struct SuperNET_storage *)find_storage(selector,key,0));
}

uint64_t *find_closer_Kstored(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    struct SuperNET_storage *sp,**sps;
    int32_t i,numentries,dist,max,m,refdist,n = 0;
    uint64_t *keys = 0;
    max = (int32_t)max_in_db(selector);
    max += 100;
    m = 0;
    sps = (struct SuperNET_storage **)copy_all_DBentries(&numentries,selector);
    if ( sps == 0 )
        return(0);
    keys = (uint64_t *)calloc(sizeof(*keys),max);
    for (i=0; i<numentries; i++)
    {
        sp = sps[i];
        m++;
        refdist = bitweight(refbits ^ sp->H.keyhash);
        dist = bitweight(newbits ^ sp->H.keyhash);
        if ( dist < refdist )
        {
            keys[n++] = sp->H.keyhash;
            if ( n >= max )
            {
                max += 100;
                keys = (uint64_t *)realloc(keys,sizeof(*keys)*max);
            }
        }
        free(sps[i]);
    }
    free(sps);
    if ( m > max_in_db(selector) )
        set_max_in_db(selector,m);
    return(keys);
}

int32_t kademlia_pushstore(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    int32_t n = 0;
    uint64_t *keys,key;
    struct storage_queue_entry *ptr;
    //fprintf(stderr,"pushstore\n");
    if ( (keys= find_closer_Kstored(selector,refbits,newbits)) != 0 )
    {
        while ( (key= keys[n++]) != 0 )
        {
            ptr = calloc(1,sizeof(*ptr));
            ptr->destbits = newbits;
            ptr->selector = selector;
            ptr->keyhash = key;
            //printf("%p queue.%d to %llu\n",ptr,n,(long long)newbits);
            queue_enqueue(&storageQ,ptr);
        }
        //printf("free sps.%p\n",sps);
        free(keys);
        if ( Debuglevel > 1 )
            printf("Queue n.%d pushstore to %llu\n",n,(long long)newbits);
    }
    return(n);
}

uint64_t process_storageQ()
{
    uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *datastr);
    struct storage_queue_entry *ptr;
    char key[64],datastr[8193];
    uint64_t txid = 0;
    struct SuperNET_storage *sp;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( (ptr= queue_dequeue(&storageQ)) != 0 )
    {
        fprintf(stderr,"dequeue StorageQ %p key.(%llu) dest.(%llu) selector.%d\n",ptr,(long long)ptr->keyhash,(long long)ptr->destbits,ptr->selector);
        expand_nxt64bits(key,ptr->keyhash);
        if ( (sp= (struct SuperNET_storage *)find_storage(ptr->selector,key,0)) != 0 )
        {
            init_hexbytes_noT(datastr,sp->data,sp->H.size-sizeof(*sp));
            fprintf(stderr,"dequeued storageQ %p: (%s) len.%ld\n",ptr,datastr,sp->H.size-sizeof(*sp));
            txid = send_kademlia_cmd(ptr->destbits,0,"store",cp->srvNXTACCTSECRET,key,datastr);
            if ( Debuglevel > 1 )
                fprintf(stderr,"txid.%llu send queued push storage key.(%s) to %llu\n",(long long)txid,key,(long long)ptr->destbits);
            free(sp);
        }
        free(ptr);
    }
    return(txid);
}

void do_localstore(uint64_t *txidp,char *keystr,char *datastr,char *NXTACCTSECRET)
{
    DBT key,data;
    cJSON *json;
    double price;
    uint64_t keybits,baseid,relid;
    int32_t ret,len,createdflag;
    struct InstantDEX_quote Q;
    char decoded[MAX_JSON_FIELD],checkstr[64];//,quotedatastr[MAX_JSON_FIELD];
    struct NXT_acct *keynp;
    struct SuperNET_storage *sp;
    keybits = calc_nxt64bits(keystr);
    //printf("halflen.%ld\n",strlen(datastr)/2);
    fprintf(stderr,"do_localstor(%s) <- (%s)\n",keystr,datastr);
    keynp = get_NXTacct(&createdflag,Global_mp,keystr);
    *txidp = 0;
    if ( keynp->bestbits != 0 )
    {
        if ( ismynxtbits(keynp->bestdist) == 0 )
        {
            printf("store at bestbits\n");
            *txidp = send_kademlia_cmd(keynp->bestdist,0,"store",NXTACCTSECRET,keystr,datastr);
        }
        printf("Bestdist.%d bestbits.%llu\n",keynp->bestdist,(long long)keynp->bestbits);
        keynp->bestbits = 0;
        keynp->bestdist = 0;
    }
    len = (int32_t)strlen(datastr)/2;
    decode_hex((uint8_t *)decoded,len,datastr);
    if ( (decoded[len-1] == 0 || decoded[len-1] == '}' || decoded[len-1] == ']') && (decoded[0] == '{' || decoded[0] == '[') )
    {
        json = cJSON_Parse(decoded);
        //({"requestType":"quote","type":0,"NXT":"13434315136155299987","base":"4551058913252105307","srcvol":"1.01000000","rel":"11060861818140490423","destvol":"0.00606000"}) 0x7f24700111c0
        if ( json != 0 )
        {
            double parse_InstantDEX_json(uint64_t *baseidp,uint64_t *relidp,struct InstantDEX_quote *iQ,cJSON *json);
            price = parse_InstantDEX_json(&baseid,&relid,&Q,json);
            expand_nxt64bits(checkstr,baseid ^ relid);
            if ( relid != 0 && baseid != 0 && strcmp(checkstr,keystr) == 0 )
            {
                //init_hexbytes_noT(quotedatastr,(uint8_t *)&Q,sizeof(Q));
                //datastr = quotedatastr;
                clear_pair(&key,&data);
                key.data = keystr;
                key.size = (uint32_t)strlen(keystr) + 1;
                data.data = &Q;
                data.size = sizeof(Q);
                {
                    int z;
                    for (z=0; z<24; z++)
                        printf("%02x ",((uint8_t *)&Q)[z]);
                    
                    printf("Q: %llu -> %llu NXT.%llu %u type.%d\n",(long long)Q.baseamount,(long long)Q.relamount,(long long)Q.nxt64bits,Q.timestamp,Q.type);
                }
                if ( (ret= dbput(INSTANTDEX_DATA,0,&key,&data,0)) != 0 )
                    Storage->err(Storage,ret,"Database put failed.");
                else dbsync(INSTANTDEX_DATA,0);
            }
            free_json(json);
        }
    }
    if ( (sp= kademlia_getstored(PUBLIC_DATA,keybits,datastr)) != 0 )
        free(sp);
}

char *kademlia_storedata(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *datastr)
{
    //static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    char retstr[32768];
    uint64_t sortbuf[2 * KADEMLIA_NUMBUCKETS * KADEMLIA_NUMK];
    uint64_t keybits,destbits,txid = 0;
    int32_t i,n,dist,mydist;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp == 0 || key == 0 || key[0] == 0 || datastr == 0 || datastr[0] == 0 )
    {
        printf("kademlia_storedata null args\n");
        return(0);
    }
    keybits = calc_nxt64bits(key);
    mydist = bitweight(keybits ^ cp->srvpubnxtbits);
    memset(sortbuf,0,sizeof(sortbuf));
    n = sort_all_buckets(sortbuf,keybits);
    retstr[0] = 0;
    if ( n != 0 )
    {
        for (i=0; i<n&&i<KADEMLIA_NUMK; i++)
        {
            destbits = sortbuf[(i<<1) + 1];
            dist = bitweight(destbits ^ keybits);
            if ( ismynxtbits(destbits) == 0 && dist < mydist )
            {
                printf("store i.%d of %d, dist.%d vs mydist.%d\n",i,n,dist,mydist);
                txid = send_kademlia_cmd(destbits,0,"store",NXTACCTSECRET,key,datastr);
            }
        }
        sprintf(retstr,"{\"result\":\"kademlia_store\",\"key\":\"%s\",\"data\":\"%s\",\"len\":%ld,\"txid\":\"%llu\"}",key,datastr,strlen(datastr)/2,(long long)txid);
        //free(sortbuf);
    } else sprintf(retstr,"{\"result\":\"localstore only\"}");
    do_localstore(&txid,key,datastr,NXTACCTSECRET);
    //if ( Debuglevel > 0 )
        printf("STORE.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *kademlia_havenode(int32_t valueflag,char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *value)
{
    char retstr[1024],ipaddr[MAX_JSON_FIELD],destNXTaddr[MAX_JSON_FIELD],pubkeystr[MAX_JSON_FIELD],portstr[MAX_JSON_FIELD],lastcontactstr[MAX_JSON_FIELD];
    int32_t i,n,createdflag,dist,mydist;
    uint32_t lastcontact,port;
    uint64_t keyhash,txid = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver = 0;
    struct NXT_acct *keynp;
    keyhash = calc_nxt64bits(key);
    mydist = bitweight(cp->srvpubnxtbits ^ keyhash);
    if ( key != 0 && key[0] != 0 && value != 0 && value[0] != 0 && (array= cJSON_Parse(value)) != 0 )
    {
        if ( is_remote_access(previpaddr) != 0 )
        {
            //extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)prevaddr);
            pserver = get_pserver(0,previpaddr,0,0);
        } else if ( cp != 0 ) pserver = get_pserver(0,cp->myipaddr,0,0);
        keynp = get_NXTacct(&createdflag,Global_mp,key);
        //printf("parsed value array.%p\n",array);
        if ( is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            //printf("is arraysize.%d\n",n);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                //printf("item.%p isarray.%d num.%d\n",item,is_cJSON_Array(item),cJSON_GetArraySize(item));
                if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 5 )
                {
                    copy_cJSON(destNXTaddr,cJSON_GetArrayItem(item,0));
                    if ( destNXTaddr[0] != 0 )
                        addto_hasnxt(pserver,calc_nxt64bits(destNXTaddr));
                    copy_cJSON(pubkeystr,cJSON_GetArrayItem(item,1));
                    copy_cJSON(ipaddr,cJSON_GetArrayItem(item,2));
                    if ( ipaddr[0] != 0 && notlocalip(ipaddr) != 0 )
                        addto_hasips(1,pserver,calc_ipbits(ipaddr));
                    copy_cJSON(portstr,cJSON_GetArrayItem(item,3));
                    copy_cJSON(lastcontactstr,cJSON_GetArrayItem(item,4));
                    port = (uint32_t)atol(portstr);
                    lastcontact = (uint32_t)atol(lastcontactstr);
                    //printf("[%s ip.%s %s port.%d lastcontact.%d]\n",destNXTaddr,ipaddr,pubkeystr,port,lastcontact);
                    if ( destNXTaddr[0] != 0 && ipaddr[0] != 0 )
                    {
                        kademlia_update_info(destNXTaddr,ipaddr,port,pubkeystr,lastcontact,0);
                        dist = bitweight(keynp->H.nxt64bits ^ calc_nxt64bits(destNXTaddr));
                        if ( dist < calc_bestdist(keyhash) )
                        {
                            if ( Debuglevel > 1 )
                                printf("%s new bestdist %d vs %d\n",destNXTaddr,dist,keynp->bestdist);
                            keynp->bestdist = dist;
                            keynp->bestbits = calc_nxt64bits(destNXTaddr);
                        }
                        if ( keynp->bestbits != 0 && ismynxtbits(keynp->bestbits) == 0 && dist < mydist )
                            txid = send_kademlia_cmd(keynp->bestbits,0,valueflag!=0?"findvalue":"findnode",NXTACCTSECRET,key,0);
                    }
                }
            }
        } else printf("kademlia.(havenode) not array\n");
        free_json(array);
    }
    sprintf(retstr,"{\"result\":\"kademlia_havenode from NXT.%s key.(%s) value.(%s)\"}",sender,key,value);
    //if ( Debuglevel > 0 )
        printf("HAVENODE.%d %s\n",valueflag,retstr);
    return(clonestr(retstr));
}

char *kademlia_find(char *cmd,char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *datastr,char *origargstr)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    unsigned char data[MAX_JSON_FIELD];
    char retstr[32768],pubkeystr[256],databuf[32768],numstr[64],ipaddr[64],_previpaddr[64],destNXTaddr[64],*value;
    uint64_t keyhash,senderbits,destbits,txid = 0;
    uint64_t sortbuf[2 * KADEMLIA_NUMBUCKETS * KADEMLIA_NUMK];
    int32_t i,n,isvalue,createdflag,datalen,mydist,dist,remoteflag = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    struct NXT_acct *keynp,*destnp,*np;
    cJSON *array,*item;
    struct SuperNET_storage *sp;
    struct nodestats *stats;
    if ( previpaddr == 0 )
    {
        _previpaddr[0] = 0;
        previpaddr = _previpaddr;
    }
    if ( Debuglevel > 0 )
        printf("myNXT.(%s) kademlia_find.(%s) (%s) data.(%s) is_remote_access.%d (%s)\n",verifiedNXTaddr,cmd,key,datastr!=0?datastr:"",is_remote_access(previpaddr),previpaddr==0?"":previpaddr);
    if ( key != 0 && key[0] != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        keyhash = calc_nxt64bits(key);
        mydist = bitweight(cp->srvpubnxtbits ^ keyhash);
        isvalue = (strcmp(cmd,"findvalue") == 0);
        if ( isvalue == 0 && datastr != 0 && datastr[0] != 0 )
        {
            void process_telepathic(char *key,uint8_t *data,int32_t len,uint64_t senderbits,char *senderip);
            if ( is_remote_access(previpaddr) != 0 )
            {
                datalen = (int32_t)(strlen(datastr) / 2);
                decode_hex(data,datalen,datastr);
                process_telepathic(key,data,datalen,senderbits,previpaddr);
                fprintf(stderr,"back from process_telepathic\n");
            }
            remoteflag = 1;
        }
        memset(sortbuf,0,sizeof(sortbuf));
        n = sort_all_buckets(sortbuf,keyhash);
        if ( n != 0 )
        {
            //printf("search n.%d sorted\n",n);
            if ( is_remote_access(previpaddr) == 0 || remoteflag != 0 ) // user invoked
            {
                keynp = get_NXTacct(&createdflag,Global_mp,key);
                keynp->bestdist = 10000;
                keynp->bestbits = 0;
                int z = 0;
                for (i=0; i<n; i++) //&&i<KADEMLIA_ALPHA
                {
                    destbits = sortbuf[(i<<1) + 1];
                    dist = bitweight(destbits ^ keyhash);
                    if ( ismynxtbits(destbits) == 0 && dist < mydist )
                    {
                        if ( (stats= get_nodestats(destbits)) != 0 && memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) == 0 )
                            send_kademlia_cmd(destbits,0,"ping",NXTACCTSECRET,0,0);
                        if ( Debuglevel > 1 )
                            printf("call %llu (%s) dist.%d mydist.%d\n",(long long)destbits,cmd,bitweight(destbits ^ keyhash),mydist);
                        expand_nxt64bits(destNXTaddr,destbits);
                        np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                        if ( np->stats.ipbits != 0 && np->stats.ipbits != calc_ipbits(previpaddr) )
                        {
                            if ( remoteflag != 0 && origargstr != 0 && datastr != 0 && datastr[0] != 0 )
                            {
                                if ( np->stats.ipbits != 0 )
                                {
                                    if ( np->stats.ipbits != calc_ipbits(previpaddr) )
                                    {
                                        expand_ipbits(ipaddr,np->stats.ipbits);
                                        datalen = (int32_t)(strlen(datastr) / 2);
                                        decode_hex(data,datalen,datastr);
                                        if ( z++ == 0 )
                                            printf("find pass through ip.(%s) (%s) (%s)\n",ipaddr,origargstr,datastr);
                                        if ( origargstr != 0 )
                                        {
                                            //txid = directsend_packet(2,get_pserver(0,ipaddr,0,0),origargstr,(int32_t)strlen(origargstr)+1,data,datalen);
                                            fprintf(stderr,"back from direct_send\n");
                                        }
                                        else printf("no origarg string for pass through?\n");
                                    }
                                } else printf("warning: find doesnt have IP address for %s\n",destNXTaddr);
                            }
                            else txid = send_kademlia_cmd(destbits,0,cmd,NXTACCTSECRET,key,datastr);
                        }
                    }
                }
            }
            if ( is_remote_access(previpaddr) != 0 )//ismynxtbits(senderbits) == 0 && (isvalue == 0 || datastr == 0) ) // need to respond to sender
            {
                array = cJSON_CreateArray();
                for (i=0; i<n&&i<KADEMLIA_NUMK; i++)
                {
                    expand_nxt64bits(destNXTaddr,sortbuf[(i<<1) + 1]);
                    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                    stats = &destnp->stats;
                    item = cJSON_CreateArray();
                    cJSON_AddItemToArray(item,cJSON_CreateString(destNXTaddr));
                    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
                    {
                        init_hexbytes_noT(pubkeystr,stats->pubkey,sizeof(stats->pubkey));
                        cJSON_AddItemToArray(item,cJSON_CreateString(pubkeystr));
                    }
                    if ( stats->ipbits != 0 )
                    {
                        expand_ipbits(ipaddr,stats->ipbits);
                        cJSON_AddItemToArray(item,cJSON_CreateString(ipaddr));
                        sprintf(numstr,"%d",stats->supernet_port==0?SUPERNET_PORT:stats->supernet_port);
                        cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
                        sprintf(numstr,"%u",stats->lastcontact);
                        cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
                    }
                    cJSON_AddItemToArray(array,item);
                }
                value = cJSON_Print(array);
                free_json(array);
                stripwhite_ns(value,strlen(value));
                printf("send back.(%s) to %llu\n",value,(long long)senderbits);
                txid = send_kademlia_cmd(senderbits,0,isvalue==0?"havenode":"havenodeB",NXTACCTSECRET,key,value);
                free(value);
            }
            if ( isvalue != 0 )
            {
                sp = kademlia_getstored(PUBLIC_DATA,keyhash,0);
                if ( sp != 0 )
                {
                    if ( sp->data != 0 )
                    {
                        init_hexbytes_noT(databuf,sp->data,sp->H.size-sizeof(*sp));
                        if ( is_remote_access(previpaddr) != 0 && ismynxtbits(senderbits) == 0 )
                        {
                            printf("found value for (%s)! call store\n",key);
                            txid = send_kademlia_cmd(senderbits,0,"store",NXTACCTSECRET,key,databuf);
                        }
                        sprintf(retstr,"{\"data\":\"%s\"}",databuf);
                    }
                    free(sp);
                    printf("FOUND.(%s)\n",retstr);
                    return(clonestr(retstr));
                }
            }
        } else if ( Debuglevel > 0 )
            printf("kademlia.(%s) no peers\n",cmd);
    }
    sprintf(retstr,"{\"result\":\"kademlia_%s from.(%s) previp.(%s) key.(%s) datalen.%ld txid.%llu\"}",cmd,sender,previpaddr,key,datastr!=0?strlen(datastr):0,(long long)txid);
    //if ( Debuglevel > 0 )
        printf("FIND.(%s)\n",retstr);
    return(clonestr(retstr));
}

void update_Kbucket(int32_t bucketid,struct nodestats *buckets[],int32_t n,struct nodestats *stats)
{
    int32_t j,k,matchflag = -1;
    uint64_t txid;
    char ipaddr[64];
    struct coin_info *cp = get_coin_info("BTCD");
    struct nodestats *eviction;
    if ( stats->ipbits == 0 || stats->nxt64bits == 0 )
        return;
    expand_ipbits(ipaddr,stats->ipbits);
    for (j=n-1; j>=0; j--)
    {
        if ( buckets[j] != 0 && buckets[j]->eviction == stats )
        {
            printf("bucketid.%d: Got pong back from eviction candidate in slot.%d\n",bucketid,j);
            buckets[j] = buckets[j]->eviction;
            break;
        }
    }
    for (j=0; j<n; j++)
    {
        if ( buckets[j] == 0 )
        {
            if ( matchflag < 0 )
            {
                buckets[j] = stats;
                add_new_node(stats->nxt64bits);
                printf("APPEND.%d: bucket[%d] <- %llu %s then call pushstore\n",bucketid,j,(long long)stats->nxt64bits,ipaddr);
                if ( cp != 0 && ismynxtbits(stats->nxt64bits) == 0 )
                    kademlia_pushstore(PUBLIC_DATA,mynxt64bits(),stats->nxt64bits);
            }
            else if ( j > 0 )
            {
                if ( matchflag != j-1 )
                {
                    for (k=matchflag; k<j-1; k++)
                        buckets[k] = buckets[k+1];
                    buckets[k] = stats;
                    for (k=0; k<j; k++)
                        printf("%llu ",(long long)buckets[k]->nxt64bits);
                    printf("ROTATE.%d: bucket[%d] <- %llu %s | bucketid.%d\n",j-1,matchflag,(long long)stats->nxt64bits,ipaddr,bucketid);
                }
            } else printf("update_Kbucket.%d: impossible case matchflag.%d j.%d\n",bucketid,matchflag,j);
            return;
        }
        else if ( buckets[j] == stats )
            matchflag = j;
    }
    if ( matchflag < 0 ) // stats is new and the bucket is full
    {
        add_new_node(stats->nxt64bits);
        eviction = buckets[0];
        for (k=0; k<n-1; k++)
            buckets[k] = buckets[k+1];
        buckets[n-1] = stats;
        printf("bucket[%d] <- %llu, check for eviction of %llu %s | bucketid.%d\n",n-1,(long long)stats->nxt64bits,(long long)eviction->nxt64bits,ipaddr,bucketid);
        stats->eviction = eviction;
        if ( cp != 0 && ismynxtbits(eviction->nxt64bits) == 0 )
        {
            kademlia_pushstore(PUBLIC_DATA,mynxt64bits(),stats->nxt64bits);
            txid = send_kademlia_cmd(eviction->nxt64bits,0,"ping",cp->srvNXTACCTSECRET,0,0);
            printf("check for eviction with destip.%u txid.%llu | bucketid.%d\n",eviction->ipbits,(long long)txid,bucketid);
        }
    }
}

void update_Kbuckets(struct nodestats *stats,uint64_t nxt64bits,char *ipaddr,int32_t port,int32_t p2pflag,int32_t lastcontact)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t xorbits;
    int32_t bucketid;
    char pubkeystr[512],NXTaddr[64],tmpipaddr[64],*ptr;
    struct pserver_info *pserver;
    if ( stats == 0 )
    {
        printf("update_Kbuckets null stats\n");
        return;
    }
    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) == 0 )
        ptr = 0;
    else
    {
        init_hexbytes_noT(pubkeystr,stats->pubkey,sizeof(stats->pubkey));
        ptr = pubkeystr;
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    kademlia_update_info(NXTaddr,ipaddr,port,ptr,lastcontact,p2pflag);
    if ( cp != 0 )
    {
        pserver = get_pserver(0,cp->myipaddr,0,0);
        //fprintf(stderr,"call addto_hasips\n");
        expand_ipbits(tmpipaddr,stats->ipbits);
        if ( notlocalip(tmpipaddr) != 0 )
            addto_hasips(1,pserver,stats->ipbits);
        xorbits = cp->srvpubnxtbits;
        if ( stats->nxt64bits != 0 )
        {
            xorbits ^= stats->nxt64bits;
            bucketid = bitweight(xorbits);
            Kbucket_updated[bucketid] = time(NULL);
            //fprintf(stderr,"call update_Kbucket\n");
            update_Kbucket(bucketid,K_buckets[bucketid],KADEMLIA_NUMK,stats);
        }
    }
}

uint64_t refresh_bucket(struct nodestats *buckets[],long lastcontact,char *NXTACCTSECRET)
{
    uint64_t txid = 0;
    int32_t i,r;
    char key[64];
    if ( buckets[0] != 0 && lastcontact >= KADEMLIA_BUCKET_REFRESHTIME )
    {
        for (i=0; i<KADEMLIA_NUMK; i++)
            if ( buckets[i] == 0 )
                break;
        if ( i > 0 )
        {
            r = ((rand()>>8) % i);
            if ( ismynxtbits(buckets[r]->nxt64bits) == 0 )
            {
                expand_nxt64bits(key,buckets[r]->nxt64bits);
                txid = send_kademlia_cmd(buckets[r]->nxt64bits,0,"findnode",NXTACCTSECRET,key,0);
            }
        }
    }
    return(txid);
}

void refresh_buckets(char *NXTACCTSECRET)
{
    static int didinit;
    long now = time(NULL);
    int32_t i,firstbucket,n = 0;
    firstbucket = -1;
    for (i=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        if ( firstbucket < 0 && K_buckets[i][0] != 0 )
            firstbucket = (i + 1);
        if ( refresh_bucket(K_buckets[i],now - Kbucket_updated[i],NXTACCTSECRET) != 0 )
            n++;
    }
    if ( n != 0 && didinit == 0 && firstbucket < KADEMLIA_NUMBUCKETS-1 )
    {
        printf("initial bucket refresh\n");
        for (i=firstbucket+1; i<KADEMLIA_NUMBUCKETS; i++)
            refresh_bucket(K_buckets[i],KADEMLIA_BUCKET_REFRESHTIME,NXTACCTSECRET);
        didinit = 1;
    }
}

cJSON *gen_pserver_json(struct pserver_info *pserver)
{
    cJSON *array,*json = cJSON_CreateObject();
    int32_t i;
    char ipaddr[64],NXTaddr[64];
    uint32_t *ipaddrs;
    struct nodestats *stats;
    double millis = milliseconds();
    if ( pserver != 0 )
    {
        if ( (ipaddrs= pserver->hasips) != 0 && pserver->numips > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<pserver->numips&&i<(int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips)); i++)
            {
                expand_ipbits(ipaddr,ipaddrs[i]);
                cJSON_AddItemToArray(array,cJSON_CreateString(ipaddr));
            }
            cJSON_AddItemToObject(json,"hasips",array);
            cJSON_AddItemToObject(json,"hasnum",cJSON_CreateNumber(pserver->hasnum));
        }
        if ( pserver->numnxt > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<pserver->numnxt&&i<(int)(sizeof(pserver->hasnxt)/sizeof(*pserver->hasnxt)); i++)
            {
                expand_nxt64bits(NXTaddr,pserver->hasnxt[i]);
                cJSON_AddItemToArray(array,cJSON_CreateString(NXTaddr));
            }
            cJSON_AddItemToObject(json,"hasnxt",array);
            cJSON_AddItemToObject(json,"numnxt",cJSON_CreateNumber(pserver->numnxt));
        }
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            if ( stats->p2pport != 0 && stats->p2pport != BTCD_PORT )
                cJSON_AddItemToObject(json,"p2p",cJSON_CreateNumber(stats->p2pport));
            if ( stats->supernet_port != 0 && stats->supernet_port != SUPERNET_PORT )
                cJSON_AddItemToObject(json,"port",cJSON_CreateNumber(stats->supernet_port));
            if ( stats->numsent != 0 )
                cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(stats->numsent));
            if ( stats->sentmilli != 0 )
                cJSON_AddItemToObject(json,"lastsent",cJSON_CreateNumber((millis - stats->sentmilli)/60000.));
            if ( stats->numrecv != 0 )
                cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(stats->numrecv));
            if ( stats->recvmilli != 0 )
                cJSON_AddItemToObject(json,"lastrecv",cJSON_CreateNumber((millis - stats->recvmilli)/60000.));
            if ( stats->numpings != 0 )
                cJSON_AddItemToObject(json,"pings",cJSON_CreateNumber(stats->numpings));
            if ( stats->numpongs != 0 )
                cJSON_AddItemToObject(json,"pongs",cJSON_CreateNumber(stats->numpongs));
            if ( stats->pingmilli != 0 && stats->pongmilli != 0 )
                cJSON_AddItemToObject(json,"pingtime",cJSON_CreateNumber(stats->pongmilli - stats->pingmilli));
            if ( stats->numpings != 0 && stats->numpongs != 0 && stats->pingpongsum != 0 )
                cJSON_AddItemToObject(json,"avetime",cJSON_CreateNumber((2. * stats->pingpongsum)/(stats->numpings + stats->numpongs)));
        }
    }
    return(json);
}

char *_coins_jsonstr(char *coinsjson,uint64_t coins[4])
{
    int32_t i,n = 0;
    char *str;
    if ( coins == 0 )
        return(0);
    strcpy(coinsjson,",\"coins\":[");
    for (i=0; i<4*64; i++)
        if ( (coins[i>>6] & (1L << (i&63))) != 0 )
        {
            str = coinid_str(i);
            if ( strcmp(str,ILLEGAL_COIN) != 0 )
            {
                if ( n++ != 0 )
                    strcat(coinsjson,", ");
                sprintf(coinsjson+strlen(coinsjson),"\"%s\"",str);
            }
        }
    if ( n == 0 )
        coinsjson[0] = coinsjson[1] = 0;
    else strcat(coinsjson,"]");
    return(coinsjson);
}

cJSON *gen_peerinfo_json(struct nodestats *stats)
{
    char srvipaddr[64],srvnxtaddr[64],numstr[64],hexstr[512],coinsjsonstr[1024];
    cJSON *coins,*json = cJSON_CreateObject();
    struct pserver_info *pserver;
    expand_ipbits(srvipaddr,stats->ipbits);
    expand_nxt64bits(srvnxtaddr,stats->nxt64bits);
    if ( stats->ipbits != 0 )
    {
        //cJSON_AddItemToObject(json,"is_privacyServer",cJSON_CreateNumber(1));
        cJSON_AddItemToObject(json,"srvNXT",cJSON_CreateString(srvnxtaddr));
        cJSON_AddItemToObject(json,"srvipaddr",cJSON_CreateString(srvipaddr));
        sprintf(numstr,"%d",stats->supernet_port);
        if ( stats->supernet_port != 0 && stats->supernet_port != SUPERNET_PORT )
            cJSON_AddItemToObject(json,"srvport",cJSON_CreateString(numstr));
        sprintf(numstr,"%d",stats->p2pport);
        if ( stats->p2pport != 0 && stats->p2pport != BTCD_PORT )
            cJSON_AddItemToObject(json,"p2pport",cJSON_CreateString(numstr));
        if ( stats->numsent != 0 )
            cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(stats->numsent));
        if ( stats->numrecv != 0 )
            cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(stats->numrecv));
        if ( (pserver= get_pserver(0,srvipaddr,0,0)) != 0 )
        {
            //printf("%s pserver.%p\n",srvipaddr,pserver);
            cJSON_AddItemToObject(json,"pserver",gen_pserver_json(pserver));
        }
    }
    else cJSON_AddItemToObject(json,"privateNXT",cJSON_CreateString(srvnxtaddr));
    init_hexbytes_noT(hexstr,stats->pubkey,sizeof(stats->pubkey));
    cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(hexstr));
    if ( _coins_jsonstr(coinsjsonstr,stats->coins) != 0 )
    {
        coins = cJSON_Parse(coinsjsonstr+9);
        if ( coins != 0 )
            cJSON_AddItemToObject(json,"coins",coins);
        //else printf("warning no coin networks.(%s) probably no peerinfo yet\n",coinsjsonstr);
    }
    return(json);
}

int32_t update_newaccts(uint64_t *newaccts,int32_t num,uint64_t nxtbits)
{
    int32_t i;
    if ( nxtbits == 0 )
        return(num);
    for (i=0; i<num; i++)
        if ( newaccts[i] == nxtbits )
            return(num);
    //printf("update_newaccts[%d] <- %llu\n",num,(long long)nxtbits);
    newaccts[num++] = nxtbits;
    return(num);
}

int32_t scan_nodes(uint64_t *newaccts,int32_t max,char *NXTACCTSECRET)
{
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver,*mypserver;
    uint32_t ipbits,newips[16];
    int32_t i,j,k,m,n,num = 0;
    uint64_t otherbits;
    char ipaddr[64];
    struct nodestats *stats;
    n = (Numallnodes < MAX_ALLNODES) ? Numallnodes : MAX_ALLNODES;
    if ( n > 0 && cp != 0 && cp->myipaddr[0] != 0 )
    {
        mypserver = get_pserver(0,cp->myipaddr,0,0);
        num = update_newaccts(newaccts,num,mypserver->nxt64bits);
        if ( mypserver->hasips != 0 && n != 0 )
        {
            memset(newips,0,sizeof(newips));
            for (i=m=0; i<n; i++)
            {
                if ( (otherbits= Allnodes[i]) != cp->privatebits )
                {
                    if ( num < max )
                        num = update_newaccts(newaccts,num,otherbits);
                    if ( (stats= get_nodestats(otherbits)) != 0 )
                    {
                        expand_ipbits(ipaddr,stats->ipbits);
                        pserver = get_pserver(0,ipaddr,0,0);
                        if ( num < max && pserver->numnxt > 0 )
                        {
                            for (j=0; j<pserver->numnxt&&j<(int)(sizeof(pserver->hasnxt)/sizeof(*pserver->hasnxt)); j++)
                            {
                                num = update_newaccts(newaccts,num,pserver->hasnxt[j]);
                                if ( num >= max )
                                    break;
                            }
                        }
                        if ( pserver->hasips != 0 && pserver->numips > 0 )
                        {
                            for (j=0; j<pserver->numips&&j<(int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips)); j++)
                            {
                                ipbits = pserver->hasips[j];
                                for (k=0; k<mypserver->numips&&k<(int)(sizeof(mypserver->hasips)/sizeof(*mypserver->hasips)); k++)
                                    if ( mypserver->hasips[k] == ipbits )
                                        break;
                                if ( k == mypserver->numips || k == (int)(sizeof(mypserver->hasips)/sizeof(*mypserver->hasips)) )
                                {
                                    newips[m++] = ipbits;
                                    if ( m >= (int)(sizeof(newips)/sizeof(*newips)) )
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            if ( m > 0 )
            {
                for (i=0; i<m; i++)
                {
                    expand_ipbits(ipaddr,newips[i]);
                    //printf("ping new ip.%s\n",ipaddr);
                    send_kademlia_cmd(0,get_pserver(0,ipaddr,0,0),"ping",NXTACCTSECRET,0,0);
                }
            }
        }
    }
    return(num);
}

cJSON *gen_peers_json(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,int32_t scanflag)
{
    int32_t i,n,numservers = 0;
    char pubkeystr[512],key[64],*retstr;
    cJSON *json,*array;
    struct nodestats *stats;
    struct coin_info *cp = get_coin_info("BTCD");
    //printf("inside gen_peer_json.%d\n",only_privacyServers);
    if ( cp == 0 )
        return(0);
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    n = (Numallnodes < MAX_ALLNODES) ? Numallnodes : MAX_ALLNODES;
    if ( n > 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( Allnodes[i] != 0 )
            {
                stats = get_nodestats(Allnodes[i]);
                if ( stats->ipbits != 0 )
                    numservers++;
                cJSON_AddItemToArray(array,gen_peerinfo_json(stats));
            }
        }
        cJSON_AddItemToObject(json,"peers",array);
        //cJSON_AddItemToObject(json,"only_privacyServers",cJSON_CreateNumber(only_privacyServers));
        cJSON_AddItemToObject(json,"num",cJSON_CreateNumber(numservers));
        cJSON_AddItemToObject(json,"Numpservers",cJSON_CreateNumber(Numallnodes));
    }
    if ( scanflag != 0 )
    {
        memset(cp->nxtaccts,0,sizeof(cp->nxtaccts));
        cp->numnxtaccts = n = scan_nodes(cp->nxtaccts,sizeof(cp->nxtaccts)/sizeof(*cp->nxtaccts),NXTACCTSECRET);
        for (i=0; i<n; i++)
        {
            expand_nxt64bits(key,cp->nxtaccts[i]);
            init_hexbytes_noT(pubkeystr,Global_mp->loopback_pubkey,sizeof(Global_mp->loopback_pubkey));
            retstr = kademlia_find("findnode",previpaddr,verifiedNXTaddr,NXTACCTSECRET,sender,key,0,0);
            if ( retstr != 0 )
                free(retstr);
        }
    }
    cJSON_AddItemToObject(json,"Numnxtaccts",cJSON_CreateNumber(cp->numnxtaccts));
    return(json);
}


#endif
