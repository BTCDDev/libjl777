//
//  mgw.h
//
//  Created by jl777 2014, refactored MGW
//  Copyright (c) 2014 jl777. MIT License.
//
// tighten security, consensus triggers for deposit  +withdraw
// +autoconvert!
// save redeems in HTML (moneysent AM) and share multisigs
// BTC, BTCD, DOGE, VRC, OPAL, BITS, VPN


#ifndef mgw_h
#define mgw_h

#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'

int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig);
struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
char *create_multisig_json(struct multisig_addr *msig,int32_t truncated);

void set_MGW_msigfname(char *fname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"/var/www/MGW/msig/ALL");
    else sprintf(fname,"/var/www/MGW/msig/%s",NXTaddr);
}

void set_MGW_statusfname(char *fname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"/var/www/MGW/status/ALL");
    else sprintf(fname,"/var/www/MGW/status/%s",NXTaddr);
}

void save_MGW_status(char *NXTaddr,char *jsonstr)
{
    FILE *fp;
    char fname[1024],cmd[1024];
    set_MGW_statusfname(fname,NXTaddr);
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        fwrite(jsonstr,1,strlen(jsonstr),fp);
        fclose(fp);
        sprintf(cmd,"chmod +r %s",fname);
        system(cmd);
        printf("fname.(%s) cmd.(%s)\n",fname,cmd);
    }
}

void update_MGW_files(char *fname,struct multisig_addr *refmsig,char *jsonstr)
{
    FILE *fp;
    long fsize;
    cJSON *json = 0,*newjson;
    int32_t i,n;
    struct multisig_addr *msig;
    char cmd[1024],sender[MAX_JSON_FIELD],*buf,*str;
    if ( (newjson= cJSON_Parse(jsonstr)) == 0 )
    {
        printf("update_MGW_files: cant parse.(%s)\n",jsonstr);
        return;
    }
    if ( (fp= fopen(fname,"rb+")) == 0 )
    {
        fp = fopen(fname,"wb");
        if ( fp != 0 )
        {
            if ( (json = cJSON_CreateArray()) != 0 )
            {
                cJSON_AddItemToArray(json,newjson), newjson = 0;
                str = cJSON_Print(json);
                fprintf(fp,"%s",str);
                free(str);
                free_json(json);
            }
            sprintf(cmd,"chmod +r %s",fname);
            system(cmd);
            fclose(fp);
        } else printf("couldnt open (%s)\n",fname);
        if ( newjson != 0 )
            free_json(newjson);
        return;
    }
    else
    {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        buf = calloc(1,fsize);
        fread(buf,1,fsize,fp);
        json = cJSON_Parse(buf);
        if ( json != 0 )
        {
            copy_cJSON(sender,cJSON_GetObjectItem(json,"sender"));
            if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
            {
                msig = 0;
                for (i=0; i<n; i++)
                {
                    if  ( (msig= decode_msigjson(0,cJSON_GetArrayItem(json,i),sender)) != 0 )
                    {
                        if ( msigcmp(refmsig,msig) == 0 )
                            break;
                        free(msig), msig = 0;
                    }
                }
                if ( msig != 0 )
                    free(msig);
                if ( i == n )
                {
                    cJSON_AddItemToArray(json,newjson), newjson = 0;
                    str = cJSON_Print(json);
                    rewind(fp);
                    fprintf(fp,"%s",str);
                    free(str);
                    printf("updated (%s)\n",fname);
                }
            }
            free_json(json);
        }
        else
        {
            printf("file.(%s) doesnt parse (%s)\n",fname,buf);
            rewind(fp);
            fprintf(fp,"[%s]\n%s",jsonstr,buf);
        }
        free(buf);
    }
    if ( fp != 0 )
        fclose(fp);
    if ( newjson != 0 )
        free_json(newjson);
}

void update_MGW_msig(struct multisig_addr *msig,char *sender)
{
    char fname[1024],*retstr;
    if ( msig != 0 )
    {
        retstr = create_multisig_json(msig,0);
        if ( retstr != 0 )
        {
            if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                printf("add_MGWaddr(%s) from (%s)\n",retstr,sender!=0?sender:"");
            //broadcast_bindAM(msig->NXTaddr,msig,origargstr);
            set_MGW_msigfname(fname,0);
            update_MGW_files(fname,msig,retstr);
            set_MGW_msigfname(fname,msig->NXTaddr);
            update_MGW_files(fname,msig,retstr);
            free(retstr);
        }
    }
}

double enough_confirms(double redeemed,double estNXT,int32_t numconfs,int32_t minconfirms)
{
    double metric;
    if ( numconfs < minconfirms )
        return(0);
    metric = log(estNXT + sqrt(redeemed));
    if ( metric < 1 )
        metric = 1.;
    return(((double)numconfs/minconfirms) - metric);
 }

int32_t in_specialNXTaddrs(char *specialNXTaddrs[],char *NXTaddr)
{
    int32_t i;
    for (i=0; specialNXTaddrs[i]!=0; i++)
        if ( strcmp(specialNXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

int32_t is_limbo_redeem(struct coin_info *cp,uint64_t txidbits)
{
    int32_t j;
    if ( cp != 0 && cp->limboarray != 0 )
    {
        for (j=0; cp->limboarray[j]!=0; j++)
            if ( txidbits == cp->limboarray[j] )
                return(1);
    }
    return(0);
}

static int32_t _cmp_vps(const void *a,const void *b)
{
#define vp_a (*(struct coin_txidind **)a)
#define vp_b (*(struct coin_txidind **)b)
	if ( vp_b->value > vp_a->value )
		return(1);
	else if ( vp_b->value < vp_a->value )
		return(-1);
	return(0);
#undef vp_a
#undef vp_b
}

void sort_vps(struct coin_txidind **vps,int32_t num)
{
	qsort(vps,num,sizeof(*vps),_cmp_vps);
}

void update_unspent_funds(struct coin_info *cp,struct coin_txidind *cointp,int32_t n)
{
    struct unspent_info *up = &cp->unspent;
    uint64_t value;
    if ( n == 0 )
        n = 1000;
    if ( n > up->maxvps )
    {
        up->vps = realloc(up->vps,n * sizeof(*up->vps));
        up->maxvps = n;
    }
    if ( cointp == 0 )
    {
        up->num = 0;
        up->maxvp = up->minvp = 0;
        memset(up->vps,0,up->maxvps * sizeof(*up->vps));
        up->maxunspent = up->unspent = up->maxavail = up->minavail = 0;
    }
    else
    {
        value = cointp->value;
        up->maxunspent += value;
        if ( cointp->entry.spent == 0 )
        {
            up->vps[up->num++] = cointp;
            up->unspent += value;
            if ( value > up->maxavail )
            {
                up->maxvp = cointp;
                up->maxavail = value;
            }
            if ( up->minavail == 0 || value < up->minavail )
            {
                up->minavail = value;
                up->minvp = cointp;
            }
        }
    }
}

struct multisig_addr *find_msigaddr(char *msigaddr)
{
    int32_t createdflag;
    if ( MTsearch_hashtable(&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr) == HASHSEARCH_ERROR )
        return(0);
    return(MTadd_hashtable(&createdflag,&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr));
    //return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

int32_t map_msigaddr(char *redeemScript,struct coin_info *cp,char *normaladdr,char *msigaddr)
{
    struct coin_info *refcp = get_coin_info("BTCD");
    int32_t i,n,ismine;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( cp == 0 || refcp == 0 || (msig= find_msigaddr(msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        return(0);
    }
   /* {
        "isvalid" : true,
        "address" : "bUNry9zFx9EQnukpUNDgHRsw6zy3eUs8yR",
        "ismine" : true,
        "isscript" : true,
        "script" : "multisig",
        "hex" : "522103a07d28c8d4eaa7e90dc34133fec204f9cf7740d5fd21acc00f9b0552e6bd721e21036d2b86cb74aaeaa94bb82549c4b6dd9666355241d37c371b1e0a17d060dad1c82103ceac7876e4655cf4e39021cf34b7228e1d961a2bcc1f8e36047b40149f3730ff53ae",
        "addresses" : [
                       "RGjegNGJDniYFeY584Adfgr8pX2uQegfoj",
                       "RQWB6GWe67EHCYurSiffYbyZPi7RGcrZa2",
                       "RWVebRCCVMz3YWrZEA9Lc3VWKH9kog5wYg"
                       ],
        "sigsrequired" : 2,
        "account" : ""
    }
*/
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        //printf("got retstr.(%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender)
{
    DBT key,data,*datap;
    int32_t i,ret,createdflag;
    struct multisig_addr *msigram;
    struct SuperNET_db *sdb = &SuperNET_dbs[MULTISIG_DATA];
    update_MGW_msig(msig,sender);
    if ( IS_LIBTEST <= 0 )
        return(-1);
    if ( msig == 0 && syncflag != 0 )
        return(dbsync(sdb,0));
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->H.size == 0 )
        msig->H.size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    msigram = MTadd_hashtable(&createdflag,&sdb->ramtable,msig->multisigaddr);
    if ( msigram->created != 0 && msig->created != 0 )
    {
        if ( msigram->created < msig->created )
            msig->created = msigram->created;
        else msigram->created = msig->created;
    }
    else if ( msig->created == 0 )
        msig->created = msigram->created;
    if ( msigram->NXTpubkey[0] != 0 && msig->NXTpubkey[0] == 0 )
        safecopy(msig->NXTpubkey,msigram->NXTpubkey,sizeof(msig->NXTpubkey));
    //if ( msigram->sender == 0 && msig->sender != 0 )
    //    createdflag = 1;
    if ( createdflag != 0 || memcmp(msigram,msig,msig->H.size) != 0 )
    {
        clear_pair(&key,&data);
        key.data = msig->multisigaddr;
        key.size = (int32_t)(strlen(msig->multisigaddr) + 1);
        data.size = msig->H.size;
        if ( sdb->overlap_write != 0 )
        {
            data.data = calloc(1,msig->H.size);
            memcpy(data.data,msig,msig->H.size);
            datap = calloc(1,sizeof(*datap));
            *datap = data;
        }
        else
        {
            data.data = msig;
            datap = &data;
        }
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("add (%s) NXTpubkey.(%s) sdb.%p\n",msig->multisigaddr,msig->NXTpubkey,sdb->dbp);
        if ( (ret= dbput(sdb,0,&key,datap,0)) != 0 )
            sdb->storage->err(sdb->storage,ret,"Database put for quote failed.");
        else if ( syncflag != 0 ) ret = dbsync(sdb,0);
    } return(1);
    return(ret);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *refpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->H.size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    safecopy(msig->NXTpubkey,refpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_json(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    char jsontxt[65536],pubkeyjsontxt[65536],rsacct[64];
    if ( msig != 0 )
    {
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"coinid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"MGWaddr\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,conv_coinstr(msig->coinstr),pubkeyjsontxt);
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        return(clonestr(jsontxt));
    }
    else return(0);
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender)
{
    int32_t j,M,n,coinid;
    char nxtstr[512],coinstr[64],ipaddr[64],NXTpubkey[128];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*nxtobj,*nameobj;
    if ( obj == 0 )
        return(0);
    coinid = (int)get_cJSON_int(obj,"coinid");
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(coinstr,nameobj);
    if ( coinstr[0] != 0 )
    {
        addrobj = cJSON_GetObjectItem(obj,"address");
        redeemobj = cJSON_GetObjectItem(obj,"redeemScript");
        pubkeysobj = cJSON_GetObjectItem(obj,"pubkey");
        nxtobj = cJSON_GetObjectItem(obj,"NXTaddr");
        if ( nxtobj != 0 )
        {
            copy_cJSON(nxtstr,nxtobj);
            if ( NXTaddr != 0 && strcmp(nxtstr,NXTaddr) != 0 )
                printf("WARNING: mismatched NXTaddr.%s vs %s\n",nxtstr,NXTaddr);
        }
        //printf("msig.%p %p %p %p\n",msig,addrobj,redeemobj,pubkeysobj);
        if ( nxtstr[0] != 0 && addrobj != 0 && redeemobj != 0 && pubkeysobj != 0 )
        {
            n = cJSON_GetArraySize(pubkeysobj);
            M = (int32_t)get_API_int(cJSON_GetObjectItem(obj,"M"),n-1);
            set_NXTpubkey(NXTpubkey,nxtstr);
            msig = alloc_multisig_addr(coinstr,M,n,nxtstr,NXTpubkey,sender);
            safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            msig->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(obj,"buyNXT"),10);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    copy_cJSON(msig->pubkeys[j].coinaddr,cJSON_GetObjectItem(pobj,"address"));
                    copy_cJSON(msig->pubkeys[j].pubkey,cJSON_GetObjectItem(pobj,"pubkey"));
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(pobj,"ipaddr"));
                    if ( Debuglevel > 3 )
                        fprintf(stderr,"{(%s) (%s) %llu ip.(%s)}.%d ",msig->pubkeys[j].coinaddr,msig->pubkeys[j].pubkey,(long long)msig->pubkeys[j].nxt64bits,ipaddr,j);
                    if ( ipaddr[0] == 0 && j < 3 )
                        strcpy(ipaddr,Server_names[j]);
                    msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
                } else { free(msig); msig = 0; }
            }
            //printf("NXT.%s -> (%s)\n",nxtstr,msig->multisigaddr);
            if ( Debuglevel > 3 )
                fprintf(stderr,"for msig.%s\n",msig->multisigaddr);
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        return(msig);
    } else fprintf(stderr,"decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
    return(0);
}

char *createmultisig_json_params(struct multisig_addr *msig,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<msig->n; i++)
    {
        key = cJSON_CreateString(msig->pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(msig->m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        free_json(array);
    }
    //printf("createmultisig_json_params.%s\n",paramstr);
    return(paramstr);
}

int32_t issue_createmultisig(struct coin_info *cp,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char addr[256];
    cJSON *json,*msigobj,*redeemobj;
    char *params,*retstr = 0;
    params = createmultisig_json_params(msig,msig->NXTaddr);
    flag = 0;
    if ( params != 0 )
    {
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("multisig params.(%s)\n",params);
        if ( cp->use_addmultisig != 0 )
        {
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"addmultisigaddress",params);
            if ( retstr != 0 )
            {
                strcpy(msig->multisigaddr,retstr);
                free(retstr);
                sprintf(addr,"\"%s\"",msig->multisigaddr);
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",addr);
                if ( retstr != 0 )
                {
                    json = cJSON_Parse(retstr);
                    if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                    else
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                        {
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                        free_json(json);
                    }
                    free(retstr);
                }
            } else printf("error creating multisig address\n");
        }
        else
        {
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createmultisig",params);
            if ( retstr != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                        {
                            copy_cJSON(msig->multisigaddr,msigobj);
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                    } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                        printf("addmultisig.(%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            } else printf("error issuing createmultisig.(%s)\n",params);
        }
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits)
{
    int32_t i,n;
    char acctcoinaddr[1024],pubkey[1024];
    for (i=n=0; i<msig->n; i++)
    {
        if ( srvbits[i] != 0 && refbits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(srvbits[i],acctcoinaddr,pubkey,refbits,msig->coinstr) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
            {
                strcpy(msig->pubkeys[i].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[i].pubkey,pubkey);
                msig->pubkeys[i].nxt64bits = srvbits[i];
                n++;
            }
        }
    }
    if ( n != msig->n )
        free(msig), msig = 0;
    return(msig);
}

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,struct coin_info *cp,char *refNXTaddr,char *refpubkey,struct contact_info **contacts)
{
    uint64_t refbits,srvbits[16];
    int32_t i,flag = 0;
    struct multisig_addr *msig;
    if ( cp == 0 )
        return(0);
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(cp->name,M,N,refNXTaddr,refpubkey,sender);
    for (i=0; i<N; i++)
        srvbits[i] = contacts[i]->nxt64bits;
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(cp,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

void broadcast_bindAM(char *refNXTaddr,struct multisig_addr *msig,char *origargstr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char *jsontxt,*AMtxid,AM[4096];
    struct json_AM *ap = (struct json_AM *)AM;
    if ( cp != 0 && (jsontxt= create_multisig_json(msig,0)) != 0 )
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>> send bind address AM\n");
        set_json_AM(ap,GATEWAY_SIG,BIND_DEPOSIT_ADDRESS,refNXTaddr,0,origargstr!=0?origargstr:jsontxt,1);
        AMtxid = submit_AM(0,cp->srvNXTADDR,&ap->H,0,cp->srvNXTACCTSECRET);
        if ( AMtxid != 0 )
            free(AMtxid);
        free(jsontxt);
    }
}

int32_t update_MGWaddr(cJSON *argjson,char *sender)
{
    int32_t i,retval = 0;
    uint64_t senderbits;
    struct multisig_addr *msig;
    if  ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        for (i=0; i<msig->n; i++)
        {
            if ( msig->pubkeys[i].nxt64bits == senderbits )
            {
                update_msig_info(msig,1,sender);
                retval = 1;
                break;
            }
        }
        free(msig);
    }
    return(retval);
}

int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr)
{
    cJSON *origargjson,*argjson;
    if ( valid > 0 && (origargjson= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(origargjson) != 0 )
            argjson = cJSON_GetArrayItem(origargjson,0);
        else argjson = origargjson;
        return(update_MGWaddr(argjson,sender));
    }
    return(0);
}

int32_t init_multisig(struct coin_info *cp)
{
    FILE *fp;
    long len,n;
    int32_t i,num = 0;
    cJSON *json;
    char fname[512],*buf;
    set_MGW_msigfname(fname,0);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        len = ftell(fp);
        rewind(fp);
        buf = calloc(1,len);
        if ( (n= fread(buf,1,len,fp)) == len )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                        num += update_MGWaddr(cJSON_GetArrayItem(json,i),Global_mp->myNXTADDR);

                } else printf("(%s) (%s) is not array or n.%ld is too small\n",fname,buf,n);
            } else printf("error parsing (%s) (%s)\n",fname,buf);
        } else printf("error reading in (%s) len %ld != size %ld\n",fname,n,len);
        fclose(fp);
        free(buf);
    }
    printf("loaded %d multisig addrs locally\n",num);
    return(num);
}

int32_t pubkeycmp(struct pubkey_info *ref,struct pubkey_info *cmp)
{
    if ( strcmp(ref->pubkey,cmp->pubkey) != 0 )
        return(1);
    if ( strcmp(ref->coinaddr,cmp->coinaddr) != 0 )
        return(2);
    if ( ref->nxt64bits != cmp->nxt64bits )
        return(3);
    return(0);
}

int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig)
{
    int32_t i,x;
    if ( ref == 0 )
        return(-1);
    if ( strcmp(ref->multisigaddr,msig->multisigaddr) != 0 || msig->m != ref->m || msig->n != ref->n )
    {
        if ( Debuglevel > 3 )
            printf("A ref.(%s) vs msig.(%s)\n",ref->multisigaddr,msig->multisigaddr);
        return(1);
    }
    if ( strcmp(ref->NXTaddr,msig->NXTaddr) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("B ref.(%s) vs msig.(%s)\n",ref->NXTaddr,msig->NXTaddr);
        return(2);
    }
    if ( strcmp(ref->redeemScript,msig->redeemScript) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("C ref.(%s) vs msig.(%s)\n",ref->redeemScript,msig->redeemScript);
        return(3);
    }
    for (i=0; i<ref->n; i++)
        if ( (x= pubkeycmp(&ref->pubkeys[i],&msig->pubkeys[i])) != 0 )
        {
            if ( Debuglevel > 3 )
            {
                switch ( x )
                {
                    case 1: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].pubkey,msig->pubkeys[i].pubkey); break;
                    case 2: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].coinaddr,msig->pubkeys[i].coinaddr); break;
                    case 3: printf("P.%d pubkey ref.(%llu) vs msig.(%llu)\n",x,(long long)ref->pubkeys[i].nxt64bits,(long long)msig->pubkeys[i].nxt64bits); break;
                    default: printf("unexpected retval.%d\n",x);
                }
            }
            return(4+i);
        }
    return(0);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,struct contact_info **contacts,int32_t n,char *userpubkey,char *email,uint32_t buyNXT)
{
    struct coin_info *cp = get_coin_info(coinstr);
    struct multisig_addr *msig;//,*dbmsig;
    struct nodestats *stats;
    struct contact_info *contact;
    char refNXTaddr[64],hopNXTaddr[64],destNXTaddr[64],mypubkey[1024],myacctcoinaddr[1024],pubkey[1024],acctcoinaddr[1024],buf[1024],*retstr = 0;
    uint64_t refbits = 0;
    int32_t i,iter,flag,valid = 0;
    refbits = conv_acctstr(refacct);
    expand_nxt64bits(refNXTaddr,refbits);
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("GENMULTISIG.%d from (%s) refacct.(%s) %llu %s email.(%s) buyNXT.%u\n",N,previpaddr,refacct,(long long)refbits,refNXTaddr,email,buyNXT);
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    flag = 0;
    stats = get_nodestats(refbits);
    for (iter=0; iter<2; iter++)
    for (i=0; i<n; i++)
    {
        if ( (contact= contacts[i]) != 0 && contact->nxt64bits != 0 )
        {
            if ( iter == 0 && ismynxtbits(contact->nxt64bits) != 0 )//|| (stats->ipbits != 0 && calc_ipbits(cp->myipaddr) == stats->ipbits)) )
            {
                myacctcoinaddr[0] = mypubkey[0] = 0;
                if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                    printf("Is me.%llu\n",(long long)contact->nxt64bits);
                if ( cp != 0 && get_acct_coinaddr(myacctcoinaddr,cp,refNXTaddr) != 0 && get_bitcoind_pubkey(mypubkey,cp,myacctcoinaddr) != 0 && myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                {
                    flag++;
                    add_NXT_coininfo(contact->nxt64bits,refbits,cp->name,myacctcoinaddr,mypubkey);
                    valid++;
                }
                else printf("error getting msigaddr for cp.%p ref.(%s) addr.(%s) pubkey.(%s)\n",cp,refNXTaddr,myacctcoinaddr,mypubkey);
            }
            else if ( iter == 1 && ismynxtbits(contact->nxt64bits) == 0 )
            {
                acctcoinaddr[0] = pubkey[0] = 0;
                if ( get_NXT_coininfo(contact->nxt64bits,acctcoinaddr,pubkey,refbits,cp->name) == 0 || acctcoinaddr[0] == 0 || pubkey[0] == 0 )
                {
                    if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                        sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"myaddr\":\"%s\",\"mypubkey\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,myacctcoinaddr,mypubkey,coinstr,refNXTaddr,userpubkey);
                    else sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,coinstr,refNXTaddr,userpubkey);
                    printf("SENDREQ.(%s)\n",buf);
                    hopNXTaddr[0] = 0;
                    expand_nxt64bits(destNXTaddr,contact->nxt64bits);
                    retstr = send_tokenized_cmd(!prevent_queueing("getmsigpubkey"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destNXTaddr);
                }
                else
                {
                    printf("already have %llu:%llu (%s %s)\n",(long long)contact->nxt64bits,(long long)refbits,acctcoinaddr,pubkey);
                    valid++;
                }
                if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                    printf("check %llu with get_NXT_coininfo i.%d valid.%d\n",(long long)contact->nxt64bits,i,valid);
            } //else printf("iter.%d reject %llu\n",iter,(long long)contact->nxt64bits);
        }
    }
    if ( (msig= gen_multisig_addr(NXTaddr,M,N,cp,refNXTaddr,userpubkey,contacts)) != 0 )
    {
        msig->valid = valid;
        safecopy(msig->email,email,sizeof(msig->email));
        msig->buyNXT = buyNXT;
        update_msig_info(msig,1,NXTaddr);
        if ( valid == N )
        {
            retstr = create_multisig_json(msig,0);
            if ( retstr != 0 )
            {
                if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                    printf("retstr.(%s) previp.(%s)\n",retstr,previpaddr);
                if ( retstr != 0 && previpaddr != 0 && previpaddr[0] != 0 )
                    send_to_ipaddr(0,1,previpaddr,retstr,NXTACCTSECRET);
                if ( msig != 0 )
                {
                    update_MGW_msig(msig,NXTaddr);
                    if ( 0 && flag != 0 ) // let the client do this
                        broadcast_bindAM(refNXTaddr,msig,0);
                    free(msig);
                }
            }
        }
    }
    if ( valid != N || retstr == 0 )
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

struct multisig_addr *find_NXT_msig(int32_t fixflag,char *NXTaddr,char *coinstr,struct contact_info **contacts,int32_t n)
{
    struct multisig_addr **msigs,*retmsig = 0;
    int32_t i,j,nummsigs;
    uint64_t srvbits[16],nxt64bits;
    for (i=0; i<n; i++)
        srvbits[i] = contacts[i]->nxt64bits;
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        nxt64bits = (NXTaddr != 0) ? calc_nxt64bits(NXTaddr) : 0;
        for (i=0; i<nummsigs; i++)
        {
            if ( fixflag != 0 && msigs[i]->valid != msigs[i]->n )
            {
                if ( finalize_msig(msigs[i],srvbits,nxt64bits) == 0 )
                    continue;
                printf("FIXED %llu -> %s\n",(long long)nxt64bits,msigs[i]->multisigaddr);
                update_msig_info(msigs[i],1,0);
            }
            if ( nxt64bits != 0 && strcmp(coinstr,msigs[i]->coinstr) == 0 && strcmp(NXTaddr,msigs[i]->NXTaddr) == 0 )
            {
                for (j=0; j<n; j++)
                    if ( srvbits[j] != msigs[i]->pubkeys[j].nxt64bits )
                        break;
                if ( j == n )
                {
                    if ( retmsig != 0 )
                        free(retmsig);
                    retmsig = msigs[i];
                }
            }
            if ( msigs[i] != retmsig )
                free(msigs[i]);
        }
        free(msigs);
    }
    return(retmsig);
}

void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid)
{
    struct coin_info *cp,*refcp = get_coin_info("BTCD");
    int32_t i,n,M,N=3;
    struct multisig_addr *msig;
    char coinaddr[512],NXTaddr[64],NXTpubkey[128],*retstr;
    cJSON *coinjson;
    struct contact_info *contacts[4];
    expand_nxt64bits(NXTaddr,nxt64bits);
    memset(contacts,0,sizeof(contacts));
    M = (N - 1);
    //if ( Global_mp->gatewayid < 0 || refcp == 0 )
    //    return;
    if ( (n= get_MGW_contacts(contacts,N)) != N )
    {
        printf("get_MGW_contacts(%d) only returned %d\n",N,n);
        for (i=0; i<n; i++)
            if ( contacts[i] != 0 )
                free(contacts[i]);
        return;
    }
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("update_coinacct_addresses.(%s)\n",NXTaddr);
    for (i=0; i<Numcoins; i++)
    {
        cp = Daemons[i];
        if ( (cp= Daemons[i]) != 0 && is_active_coin(cp->name) != 0 )
        {
            coinjson = cJSON_GetObjectItem(json,cp->name);
            if ( coinjson == 0 )
                continue;
            copy_cJSON(coinaddr,coinjson);
            if ( (msig= find_NXT_msig(0,NXTaddr,cp->name,contacts,N)) == 0 )
            {
                set_NXTpubkey(NXTpubkey,NXTaddr);
                retstr = genmultisig(refcp->srvNXTADDR,refcp->srvNXTACCTSECRET,0,cp->name,NXTaddr,M,N,contacts,N,NXTpubkey,0,0);
                if ( retstr != 0 )
                {
                    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                        printf("UPDATE_COINACCT_ADDRESSES (%s) -> (%s)\n",cp->name,retstr);
                    free(retstr);
                }
            }
            else free(msig);
         }
    }
}

int32_t process_directnet_syncwithdraw(struct batch_info *wp)
{
    int32_t gatewayid;
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinid_str(wp->W.coinid))) == 0 )
        printf("cant find coinid.%d\n",wp->W.coinid);
    else
    {
        gatewayid = (wp->W.srcgateway % NUM_GATEWAYS);
        cp->withdrawinfos[gatewayid] = *wp;
        *wp = cp->withdrawinfos[Global_mp->gatewayid];
        printf("GOT <<<<<<<<<<<< publish_withdraw_info.%d coinid.%d %.8f crc %08x\n",gatewayid,wp->W.coinid,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc);
    }
    return(sizeof(*wp));
}

void MGW_handler(struct transfer_args *args)
{
    printf("MGW_handler(%s %d bytes) vs %ld\n",args->name,args->totallen,sizeof(struct batch_info));
    if ( args->totallen == sizeof(struct batch_info) )
        process_directnet_syncwithdraw((struct batch_info *)args->data);
    //getchar();
}

void set_batchname(char *batchname,char *coinstr,int32_t gatewayid)
{
    sprintf(batchname,"%s.MGW%d",coinstr,gatewayid);
}

void publish_withdraw_info(struct coin_info *cp,struct batch_info *wp)
{
    struct coin_info *refcp = get_coin_info("BTCD");
    char batchname[512],fname[512],*retstr;
    struct batch_info W;
    int32_t gatewayid;
    FILE *fp;
    wp->W.coinid = conv_coinstr(cp->name);
    set_batchname(batchname,cp->name,Global_mp->gatewayid);
    set_handler_fname(fname,"mgw",batchname);
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        fwrite(wp,1,sizeof(*wp),fp);
        fclose(fp);
        printf("created (%s)\n",fname);
    }
    if ( wp->W.coinid < 0 || refcp == 0 )
    {
        printf("unknown coin.(%s) refcp.%p\n",cp->name,refcp);
        return;
    }
    wp->W.srcgateway = Global_mp->gatewayid;
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        wp->W.destgateway = gatewayid;
        W = *wp;
        fprintf(stderr,"publish_withdraw_info.%d -> %d coinid.%d %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinid,dstr(wp->W.amount),W.rawtx.batchcrc);
        if ( gatewayid == Global_mp->gatewayid )
        {
            process_directnet_syncwithdraw(wp);
            cp->withdrawinfos[gatewayid] = *wp;
        }
        else
        {
            retstr = start_transfer(0,refcp->srvNXTADDR,refcp->srvNXTADDR,refcp->srvNXTACCTSECRET,Server_names[gatewayid],batchname,(uint8_t *)&cp->BATCH,(int32_t)sizeof(cp->BATCH),300,"mgw");
            if ( retstr != 0 )
                free(retstr);
        }
        fprintf(stderr,"got publish_withdraw_info.%d -> %d coinid.%d %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinid,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc);
    }
}

uint64_t add_pendingxfer(int32_t removeflag,uint64_t txid)
{
    static int numpending;
    static uint64_t *pendingxfers;
    int32_t nonz,i = 0;
    uint64_t pendingtxid = 0;
    nonz = 0;
    if ( numpending > 0 )
    {
        for (i=0; i<numpending; i++)
        {
            if ( removeflag == 0 )
            {
                if ( pendingxfers[i] == 0 )
                {
                    pendingxfers[i] = txid;
                    break;
                } else nonz++;
            }
            else if ( pendingxfers[i] == txid )
            {
                printf("PENDING.(%llu) removed\n",(long long)txid);
                pendingxfers[i] = 0;
                return(0);
            }
        }
    }
    if ( i == numpending && txid != 0 && removeflag == 0 )
    {
        pendingxfers = realloc(pendingxfers,sizeof(*pendingxfers) * (numpending+1));
        pendingxfers[numpending++] = txid;
        printf("(%d) PENDING.(%llu) added\n",numpending,(long long)txid);
    }
    if ( numpending > 0 )
    {
        for (i=0; i<numpending; i++)
        {
            if ( pendingtxid == 0 && pendingxfers[i] != 0 )
            {
                pendingtxid = pendingxfers[i];
                break;
            }
        }
    }
    return(pendingtxid);
}

uint64_t get_deposittxid(struct NXT_asset *ap,char *txidstr,int32_t vout)
{
    int32_t i;
    if ( ap->num > 0 )
    {
        for (i=0; i<ap->num; i++)
            if ( ap->txids[i]->cointxid != 0 && strcmp(ap->txids[i]->cointxid,txidstr) == 0 && ap->txids[i]->coinv == vout )
                return(ap->txids[i]->txidbits);
    }
    return(0);
}

uint64_t get_sentAM_cointxid(char *txidstr,struct coin_info *cp,cJSON *autojson,char *withdrawaddr,uint64_t redeemtxid,uint64_t AMtxidbits)
{
    char AMtxidstr[64],coinstr[512],redeemstr[MAX_JSON_FIELD],comment[MAX_JSON_FIELD*4],*retstr;
    char checkcointxid[512],checkcoinaddr[512];
    cJSON *json,*commentjson,*array;
    int32_t i,n,matched = 0;
    uint64_t value = 0;
    uint32_t blocknum,txind,vout;
    txidstr[0] = 0;
    blocknum = txind = vout = 0;
    expand_nxt64bits(AMtxidstr,AMtxidbits);
    if ( (retstr= issue_getTransaction(0,AMtxidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(comment,cJSON_GetObjectItem(json,"comment"));
            if ( comment[0] != 0 && (comment[0] == '{' || comment[0] == '[') && (commentjson= cJSON_Parse(comment)) != 0 )
            {
                if ( extract_cJSON_str(redeemstr,sizeof(redeemstr),commentjson,"redeemtxid") > 0 && calc_nxt64bits(redeemstr) != redeemtxid )
                {
                    array = cJSON_GetObjectItem(commentjson,"redeems");
                    if ( array != 0 && is_cJSON_Array(array) != 0 )
                    {
                        n = cJSON_GetArraySize(array);
                        for (i=0; i<n; i++)
                        {
                            copy_cJSON(redeemstr,cJSON_GetArrayItem(array,i));
                            if ( redeemstr[0] != 0 && calc_nxt64bits(redeemstr) == redeemtxid )
                            {
                                matched++;
                                break;
                            }
                        }
                    }
                }
                if ( autojson == 0 )
                {
                    copy_cJSON(txidstr,cJSON_GetObjectItem(commentjson,"cointxid"));
                    if ( extract_cJSON_str(coinstr,sizeof(coinstr),commentjson,"coin") > 0 && strcmp(coinstr,cp->name) == 0 )
                    {
                        //sprintf(comment,"{\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u,\"amount\":\"%.8f\"}",coinaddr,txidstr,entry->blocknum,entry->txind,entry->v,dstr(value));
                        extract_cJSON_str(checkcoinaddr,sizeof(checkcoinaddr),commentjson,"coinaddr");
                        extract_cJSON_str(checkcointxid,sizeof(checkcointxid),commentjson,"cointxid");
                        if ( strcmp(checkcoinaddr,withdrawaddr) == 0 )
                            matched++;
                        if ( strcmp(checkcointxid,txidstr) == 0 )
                            matched++;
                        blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"coinblocknum"),0);
                        txind = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"cointxind"),0);
                        vout = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"coinv"),0);
                    } else matched++;
                }
                free_json(commentjson);
            }
            free_json(json);
        }
        free(retstr);
    }
    if ( autojson != 0 )
    {
        if ( matched == 0 )
            value = 0;
    }
    else if ( matched == 3 && txidstr[0] != 0 )
    {
        int32_t numvouts;
        value = get_txoutstr(&numvouts,checkcointxid,checkcoinaddr,0,cp,blocknum,txind,vout);
        if ( strcmp(checkcoinaddr,withdrawaddr) == 0 )
            matched++;
        if ( strcmp(checkcointxid,txidstr) == 0 )
            matched++;
        if ( matched != 5 )
            value = 0;
    } else value = 0;
    return(value);
}

void _update_redeembits(struct coin_info *cp,uint64_t redeembits,uint64_t AMtxidbits)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    int32_t i,n = 0;
    if ( cp == 0 )
        return;
    if ( cp->limboarray != 0 )
        for (n=0; cp->limboarray[n]!=0; n++)
            ;
    cp->limboarray = realloc(cp->limboarray,sizeof(*cp->limboarray) * (n+2));
    cp->limboarray[n++] = redeembits;
    cp->limboarray[n] = 0;
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("n.%d set AMtxidbits.%llu -> %s.(%llu)\n",n,(long long)AMtxidbits,cp->name,(long long)redeembits);
    if ( cp != 0 )
    {
        ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        if ( ap->num > 0 )
        {
            for (i=0; i<ap->num; i++)
                if ( ap->txids[i]->redeemtxid == redeembits )
                    ap->txids[i]->AMtxidbits = AMtxidbits;
        }
    }
}

void update_redeembits(cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    int32_t i,n;
    struct coin_info *cp;
    char coinstr[1024],redeemtxid[1024];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return;
    cp = get_coin_info(coinstr);
    array = cJSON_GetObjectItem(argjson,"redeems");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            copy_cJSON(redeemtxid,cJSON_GetArrayItem(array,i));
            if ( redeemtxid[0] != 0 && is_limbo_redeem(cp,calc_nxt64bits(redeemtxid)) == 0 )
                _update_redeembits(cp,calc_nxt64bits(redeemtxid),AMtxidbits);
        }
    }
    if ( extract_cJSON_str(redeemtxid,sizeof(redeemtxid),argjson,"redeemtxid") > 0 && is_limbo_redeem(cp,calc_nxt64bits(redeemtxid)) == 0 )
        _update_redeembits(cp,calc_nxt64bits(redeemtxid),AMtxidbits);
}

void process_MGW_message(char *specialNXTaddrs[],struct json_AM *ap,char *sender,char *receiver,char *txid,int32_t syncflag,char *coinstr)
{
    char NXTaddr[64];
    cJSON *argjson;
    struct multisig_addr *msig;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( ap->funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                //fprintf(stderr,"GENADDRESS: func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                update_coinacct_addresses(ap->H.nxt64bits,argjson,txid);
                break;
            case BIND_DEPOSIT_ADDRESS:
                if ( (in_specialNXTaddrs(specialNXTaddrs,sender) != 0 || strcmp(sender,receiver) == 0) && (msig= decode_msigjson(0,argjson,sender)) != 0 )
                {
                    if ( strcmp(msig->coinstr,coinstr) == 0 )
                    {
                        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                            fprintf(stderr,"BINDFUNC: %s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                        if ( update_msig_info(msig,syncflag,sender) == 0 )
                            fprintf(stderr,"%s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                    }
                    free(msig);
                } //else printf("WARNING: sender.%s == NXTaddr.%s\n",sender,NXTaddr);
                break;
            case DEPOSIT_CONFIRMED:
                // need to mark cointxid with AMtxid to prevent confirmation process generating AM each time
                /*if ( is_gateway_addr(sender) != 0 && (coinid= decode_depositconfirmed_json(argjson,txid)) >= 0 )
                 {
                 printf("deposit confirmed for coinid.%d %s\n",coinid,coinid_str(coinid));
                 }*/
                break;
            case MONEY_SENT:
                //if ( is_gateway_addr(sender) != 0 )
                //    update_money_sent(argjson,txid,height);
                  if ( in_specialNXTaddrs(specialNXTaddrs,sender) != 0 )
                    update_redeembits(argjson,calc_nxt64bits(txid));
                break;
            default: printf("funcid.(%c) not handled\n",ap->funcid);
        }
        if ( argjson != 0 )
            free_json(argjson);
    } else if ( Debuglevel > 2 ) printf("can't JSON parse (%s)\n",ap->U.jsonstr);
}

uint64_t process_NXTtransaction(char *specialNXTaddrs[],char *sender,char *receiver,cJSON *item,char *refNXTaddr,char *assetid,int32_t syncflag,struct coin_info *refcp)
{
    int32_t conv_coinstr(char *);
    char AMstr[4096],txid[4096],comment[4096],*assetidstr,*commentstr = 0;
    cJSON *senderobj,*attachment,*message,*assetjson,*commentobj,*cointxidobj;
    char cointxid[128];
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp;
    struct coin_info *cp = 0;
    uint64_t retbits = 0;
    int32_t numconfs,height,buyNXT,timestamp=0;
    int64_t type,subtype,n,assetoshis = 0;
    assetid[0] = 0;
    cointxid[0] = 0;
    buyNXT = 0;
    if ( item != 0 )
    {
        hdr = 0; assetidstr = 0;
        sender[0] = receiver[0] = 0;
        numconfs = (int32_t)get_API_int(cJSON_GetObjectItem(item,"confirmations"),0);
        copy_cJSON(txid,cJSON_GetObjectItem(item,"transaction"));
        type = get_cJSON_int(item,"type");
        subtype = get_cJSON_int(item,"subtype");
        /*if ( strcmp(txid,"11393134458431817279") == 0 )
        {
            fprintf(stderr,"[%s] start type.%d subtype.%d txid.(%s)\n",cJSON_Print(item),(int)type,(int)subtype,txid);
            getchar();
        }*/
        timestamp = (int32_t)get_cJSON_int(item,"blockTimestamp");
        height = (int32_t)get_cJSON_int(item,"height");
        senderobj = cJSON_GetObjectItem(item,"sender");
        if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(item,"accountId");
        else if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(item,"account");
        copy_cJSON(sender,senderobj);
        copy_cJSON(receiver,cJSON_GetObjectItem(item,"recipient"));
        attachment = cJSON_GetObjectItem(item,"attachment");
        if ( attachment != 0 )
        {
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            if ( message != 0 && type == 1 )
            {
                copy_cJSON(AMstr,message);
                //printf("txid.%s AM message.(%s).%ld\n",txid,AMstr,strlen(AMstr));
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    memset(buf,0,sizeof(buf));
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    hdr = (struct NXT_AMhdr *)buf;
                    process_MGW_message(specialNXTaddrs,(void *)hdr,sender,receiver,txid,syncflag,refcp->name);
                }
            }
            else if ( assetjson != 0 && type == 2 && subtype <= 1 )
            {
                commentobj = cJSON_GetObjectItem(attachment,"comment");
                if ( commentobj == 0 )
                    commentobj = message;
                copy_cJSON(comment,commentobj);
                if ( comment[0] != 0 )
                    commentstr = clonestr(replace_backslashquotes(comment));
                tp = add_NXT_assettxid(&ap,assetid,assetjson,txid,timestamp);
                if ( tp != 0 )
                {
                    tp->numconfs = numconfs;
                    if ( tp->comment != 0 )
                        free(tp->comment);
                    tp->comment = commentstr;
                    tp->timestamp = timestamp;
                    if ( type == 2 )
                    {
                        tp->quantity = get_cJSON_int(attachment,"quantityQNT");
                        assetoshis = tp->quantity;
                        switch ( subtype )
                        {
                            case 0:
                                break;
                            case 1:
                                tp->senderbits = calc_nxt64bits(sender);
                                tp->receiverbits = calc_nxt64bits(receiver);
                                if ( ap != 0 )
                                {
                                    //coinid = conv_coinstr(ap->name);
                                    cp = get_coin_info(ap->name);
                                    commentobj = 0;
                                    if ( ap->mult != 0 )
                                        assetoshis *= ap->mult;
                                    else printf("ERROR asset.(%s) has no mult??\n",ap->name);
                                    //printf("case1 sender.(%s) receiver.(%s) comment.%p cmp.%d\n",sender,receiver,tp->comment,strcmp(receiver,refNXTaddr)==0);
                                    if ( tp->comment != 0 && (tp->comment[0] == '{' || tp->comment[0] == '[') && (commentobj= cJSON_Parse(tp->comment)) != 0 )
                                    {
                                        buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(commentobj,"buyNXT"),0);
                                        cointxidobj = cJSON_GetObjectItem(commentobj,"cointxid");
                                        if ( cointxidobj != 0 )
                                        {
                                            copy_cJSON(cointxid,cointxidobj);
                                            if ( Debuglevel > 2 )
                                                printf("got.(%s) comment.(%s) cointxidstr.(%s)\n",txid,tp->comment,cointxid);
                                            if ( cointxid[0] != 0 )
                                            {
                                                tp->cointxid = clonestr(cointxid);
                                                if ( in_specialNXTaddrs(specialNXTaddrs,sender) == 0 )
                                                    buyNXT = 0;
                                            }
                                            else buyNXT = 0;
                                        } else buyNXT = 0;
                                        free_json(commentobj);
                                    } else if ( tp->comment != 0 )
                                        free(tp->comment), tp->comment = 0;
                                    if ( cp != 0 )//&& is_limbo_redeem(ap->name,txid) == 0 )
                                    {
                                        cp->boughtNXT += buyNXT;
                                        if ( cp->NXTfee_equiv != 0 && cp->txfee != 0 )
                                            tp->estNXT = (((double)cp->NXTfee_equiv / cp->txfee) * assetoshis / SATOSHIDEN);
                                        if ( tp->comment != 0 && tp->comment[0] != 0 )
                                            tp->redeemtxid = calc_nxt64bits(txid);
                                        if ( Debuglevel > 1 )
                                            printf("%s txid.(%s) got comment.(%s) gotpossibleredeem.(%s) %.8f/%.8f NXTequiv %.8f -> redeemtxid.%llu\n",ap->name,txid,tp->comment!=0?tp->comment:"",cointxid,dstr(tp->quantity * ap->mult),dstr(assetoshis),tp->estNXT,(long long)tp->redeemtxid);
                                    }
                                }
                                break;
                            case 2:
                            case 3: // bids and asks, no indication they are filled at this point, so nothing to do
                                break;
                        }
                    }
                    tp->U.assetoshis = assetoshis;
                    add_pendingxfer(1,tp->txidbits);
                    retbits = tp->txidbits;
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    //fprintf(stderr,"finish type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
    return(retbits);
}

int32_t update_NXT_transactions(char *specialNXTaddrs[],int32_t txtype,char *refNXTaddr,struct coin_info *cp)
{
    char sender[1024],receiver[1024],assetid[1024],cmd[1024],*jsonstr;
    int32_t createdflag,coinid,i,n = 0;
    int32_t timestamp,numconfs;
    struct NXT_acct *np;
    cJSON *item,*json,*array;
    if ( refNXTaddr == 0 )
    {
        printf("illegal refNXT.(%s)\n",refNXTaddr);
        return(0);
    }
    sprintf(cmd,"%s=getAccountTransactions&account=%s",_NXTSERVER,refNXTaddr);
    if ( txtype >= 0 )
        sprintf(cmd+strlen(cmd),"&type=%d",txtype);
    coinid = conv_coinstr(cp->name);
    np = get_NXTacct(&createdflag,Global_mp,refNXTaddr);
    if ( coinid > 0 && np->timestamps[coinid] != 0 && coinid < 64 )
        sprintf(cmd + strlen(cmd),"&timestamp=%d",np->timestamps[coinid]);
    if ( Debuglevel > 2 )
        printf("minconfirms.%d update_NXT_transactions.(%s) for (%s) cmd.(%s) type.%d\n",MIN_NXTCONFIRMS,refNXTaddr,cp->name,cmd,txtype);
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //if ( strcmp(refNXTaddr,"7117166754336896747") == 0 )
        //    printf("(%s)\n",jsonstr);//, getchar();
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"%d/%d ",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
                    numconfs = (int32_t)get_API_int(cJSON_GetObjectItem(item,"confirmations"),0);
                    if ( in_specialNXTaddrs(specialNXTaddrs,sender) > 0 || numconfs >= MIN_NXTCONFIRMS )
                    {
                        process_NXTtransaction(specialNXTaddrs,sender,receiver,item,refNXTaddr,assetid,0,cp);
                        timestamp = (int32_t)get_cJSON_int(item,"blockTimestamp");
                        if ( timestamp > 0 && (timestamp - 3600) > np->timestamps[coinid] )
                        {
                            printf("new timestamp.%d %d -> %d\n",coinid,np->timestamps[coinid],timestamp-3600);
                            np->timestamps[coinid] = (timestamp - 3600); // assumes no hour long block
                        } //else if ( timestamp < 0 ) genesis tx dont have any timestamps!
                          //  printf("missing blockTimestamp.(%s)\n",jsonstr), getchar();
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    } else printf("error with update_NXT_transactions.(%s)\n",cmd);
    return(n);
}

int32_t ready_to_xferassets(uint64_t *txidp)
{
    // if fresh reboot, need to wait the xfer max duration + 1 block before running this
    static int32_t firsttime,firstNXTblock;
    *txidp = 0;
    printf("(%d %d) lag.%ld %d\n",firsttime,firstNXTblock,time(NULL)-firsttime,get_NXTblock(0)-firstNXTblock);
    if ( firsttime == 0 )
        firsttime = (uint32_t)time(NULL);
    if ( firstNXTblock <= 0 )
        firstNXTblock = get_NXTblock(0);
    if ( time(NULL) < (firsttime + DEPOSIT_XFER_DURATION*60) )
        return(0);
    if ( firstNXTblock <= 0 || get_NXTblock(0) < (firstNXTblock + DEPOSIT_XFER_DURATION) )
        return(0);
    if ( (*txidp= add_pendingxfer(0,0)) != 0 )
    {
        printf("waiting for pendingxfer\n");
        return(0);
    }
    return(1);
}

uint64_t conv_address_entry(char *coinaddr,char *txidstr,char *script,struct coin_info *cp,struct address_entry *entry)
{
    char txidstr_v0[1024],coinaddr_v0[1024],script_v0[4096],_script[4096];
    int32_t numvouts;
    uint64_t value = 0;
    coinaddr[0] = txidstr[0] = 0;
    if ( script != 0 )
        script[0] = 0;
    if ( entry->vinflag == 0 )
    {
        if ( script == 0 )
            script = _script;
        value = get_txoutstr(&numvouts,txidstr,coinaddr,script,cp,entry->blocknum,entry->txind,entry->v);
        if ( strcmp("31dcbc5b7cfd7fc8f2c1cedf65f38ec166b657cc9eb15e7d1292986eada35ea9",txidstr) == 0 ) // due to uncommented tx
            return(0);
        if ( entry->v == numvouts-1 ) // the last output when there is a marker is internal change, need to get numvouts first
        {
            get_txoutstr(0,txidstr_v0,coinaddr_v0,script_v0,cp,entry->blocknum,entry->txind,0);
            if ( strcmp(coinaddr_v0,cp->marker) == 0 )
                return(0);
        }
    }
    return(value);
}

uint64_t process_msigdeposits(cJSON **transferjsonp,int32_t forceflag,struct coin_info *cp,struct address_entry *entry,uint64_t nxt64bits,struct NXT_asset *ap,char *msigaddr,char *depositors_pubkey,uint32_t *buyNXTp)
{
    double get_current_rate(char *base,char *rel);
    char buf[MAX_JSON_FIELD],txidstr[1024],coinaddr[1024],script[4096],comment[4096],NXTaddr[64],numstr[64],rsacct[64],*errjsontxt,*str;
    struct NXT_assettxid *tp;
    uint64_t depositid,value,convamount,total = 0;
    int32_t j,haspubkey,iter;
    double rate;
    cJSON *pair,*errjson,*item;
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //printf("%d of %d: process.(%s) isinternal.%d %llu (%llu -> %llu)\n",j,ap->num,msigaddr,entry->isinternal,(long long)nxt64bits,(long long)tp->senderbits,(long long)tp->receiverbits);
        if ( tp->receiverbits == nxt64bits && tp->coinblocknum == entry->blocknum && tp->cointxind == entry->txind && tp->coinv == entry->v )
            break;
    }
    if ( j == ap->num )
    {
        if ( (value= conv_address_entry(coinaddr,txidstr,script,cp,entry)) == 0 )
            return(0);
        if ( strcmp(msigaddr,coinaddr) == 0 && txidstr[0] != 0 && value >= (cp->NXTfee_equiv * MIN_DEPOSIT_FACTOR) )
        {
            for (j=0; j<ap->num; j++)
            {
                tp = ap->txids[j];
                if ( tp->receiverbits == nxt64bits && tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
                {
                    if ( Debuglevel > 0 )
                        printf("%llu set cointxid.(%s) <-> (%u %d %d)\n",(long long)nxt64bits,txidstr,entry->blocknum,entry->txind,entry->v);
                    tp->cointxind = entry->txind;
                    tp->coinv = entry->v;
                    tp->coinblocknum = entry->blocknum;
                    break;
                }
            }
            if ( j == ap->num )
            {
                issue_getpubkey(&haspubkey,rsacct);
                conv_rsacctstr(rsacct,nxt64bits);
                //printf("UNPAID cointxid.(%s) <-> (%u %d %d)\n",txidstr,entry->blocknum,entry->txind,entry->v);
                sprintf(comment,"{\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u,\"amount\":\"%.8f\"}",coinaddr,txidstr,entry->blocknum,entry->txind,entry->v,dstr(value));
                pair = cJSON_Parse(comment);
                cJSON_AddItemToObject(pair,"NXT",cJSON_CreateString(rsacct));
                printf("forceflag.%d >>>>>>>>>>>>>> Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",forceflag,dstr(value),(long)(value/ap->mult),cp->name,(long long)nxt64bits,txidstr,comment);
                total += value;
                if ( haspubkey == 0 && *buyNXTp > 0 && (rate = get_current_rate(cp->name,"NXT")) != 0. )
                {
                    if ( *buyNXTp > MAX_BUYNXT )
                        *buyNXTp = MAX_BUYNXT;
                    convamount = ((double)(*buyNXTp+2) * SATOSHIDEN) / rate; // 2 NXT extra to cover the 2 NXT txfees
                    if ( convamount >= value )
                    {
                        convamount = value;
                        *buyNXTp = ((convamount * rate) / SATOSHIDEN);
                    }
                    cJSON_AddItemToObject(pair,"rate",cJSON_CreateNumber(rate));
                    cJSON_AddItemToObject(pair,"conv",cJSON_CreateNumber(dstr(convamount)));
                    cJSON_AddItemToObject(pair,"buyNXT",cJSON_CreateNumber(*buyNXTp));
                    value -= convamount;
                } else convamount = 0;
                str = cJSON_Print(pair);
                if ( forceflag > 0 && value > 0 )
                {
                    expand_nxt64bits(NXTaddr,nxt64bits);
                    for (iter=(value==0); iter<2; iter++)
                    {
                        errjsontxt = 0;
                        depositid = issue_transferAsset(&errjsontxt,0,cp->srvNXTACCTSECRET,NXTaddr,(iter == 0) ? cp->assetid : NXT_ASSETIDSTR,(iter == 0) ? (value/ap->mult) : *buyNXTp*SATOSHIDEN,MIN_NQTFEE,DEPOSIT_XFER_DURATION,str,depositors_pubkey);
                        if ( depositid != 0 && errjsontxt == 0 )
                        {
                            printf("%s worked.%llu\n",(iter == 0) ? "deposit" : "convert",(long long)depositid);
                            if ( iter == 1 )
                                *buyNXTp = 0;
                            add_pendingxfer(0,depositid);
                            if ( transferjsonp != 0 )
                            {
                                if ( *transferjsonp == 0 )
                                    *transferjsonp = cJSON_CreateArray();
                                sprintf(numstr,"%llu",(long long)depositid);
                                cJSON_AddItemToObject(pair,(iter == 0) ? "depositid" : "convertid",cJSON_CreateString(numstr));
                            }
                        }
                        else if ( errjsontxt != 0 )
                        {
                            printf("%s failed.(%s)\n",(iter == 0) ? "deposit" : "convert",errjsontxt);
                            if ( 1 && (errjson= cJSON_Parse(errjsontxt)) != 0 )
                            {
                                if ( (item= cJSON_GetObjectItem(errjson,"error")) != 0 )
                                {
                                    copy_cJSON(buf,item);
                                    cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(buf));
                                }
                                free_json(errjson);
                            }
                            else cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(errjsontxt));
                            free(errjsontxt);
                        }
                        if ( *buyNXTp == 0 )
                            break;
                    }
                }
                free(str);
                if ( transferjsonp != 0 )
                    cJSON_AddItemToArray(*transferjsonp,pair);
            }
        }
    }
    return(total);
}

struct coin_txidmap *get_txid(struct coin_info *cp,char *txidstr,uint32_t blocknum,int32_t txind,int32_t v)
{
    char buf[1024],coinaddr[1024],script[4096],checktxidstr[1024];
    int32_t createdflag;
    struct coin_txidmap *tp;
    sprintf(buf,"%s_%s_%d",txidstr,cp->name,v);
    tp = MTadd_hashtable(&createdflag,Global_mp->coin_txidmap,buf);
    if ( createdflag != 0 )
    {
        if ( blocknum == 0xffffffff || txind < 0 )
        {
            blocknum = get_txidind(&txind,cp,txidstr,v);
            if ( txind >= 0 && blocknum < 0xffffffff )
            {
                get_txoutstr(0,checktxidstr,coinaddr,script,cp,blocknum,txind,v);
                if ( strcmp(checktxidstr,txidstr) != 0 )
                    printf("checktxid.(%s) != (%s)???\n",checktxidstr,txidstr);
                else printf("txid.(%s) (%d %d %d) verified\n",txidstr,blocknum,txind,v);
            }
        }
        tp->blocknum = blocknum;
        tp->txind = txind;
        tp->v = v;
    }
    return(tp);
}

struct coin_txidind *_get_cointp(int32_t *createdflagp,char *coinstr,uint32_t blocknum,uint16_t txind,uint16_t v)
{
    char indstr[32];
    uint64_t ind;
    ind = ((uint64_t)blocknum << 32) | ((uint64_t)txind << 16) | v;
    strcpy(indstr,coinstr);
    expand_nxt64bits(indstr+strlen(indstr),ind);
    return(MTadd_hashtable(createdflagp,Global_mp->coin_txidinds,indstr));
}

struct coin_txidind *conv_txidstr(struct coin_info *cp,char *txidstr,int32_t v)
{
    int32_t txind,createdflag;
    uint32_t blocknum;
    blocknum = get_txidind(&txind,cp,txidstr,v);
    return(_get_cointp(&createdflag,cp->name,blocknum,txind,v));
}

struct coin_txidind *get_cointp(struct coin_info *cp,struct address_entry *entry)
{
    char script[MAX_JSON_FIELD],origtxidstr[256];
    struct coin_txidind *cointp;
    struct coin_txidmap *tp;
    int32_t createdflag,spentflag;
    uint32_t blocknum;
    uint16_t txind,v;
    if ( entry->vinflag != 0 )
    {
        v = get_txinstr(origtxidstr,cp,entry->blocknum,entry->txind,entry->v);
        tp = get_txid(cp,origtxidstr,0xffffffff,-1,v);
        blocknum = tp->blocknum;
        txind = tp->txind;
        if ( v != tp->v )
            fprintf(stderr,"error (%d != %d)\n",v,tp->v);
        if ( Debuglevel > 2 )
            printf("get_cointpspent.(%016llx) (%d %d %d) -> (%s).%d (%d %d %d)\n",*(long long *)entry,entry->blocknum,entry->txind,entry->v,origtxidstr,v,blocknum,txind,v);
        spentflag = 1;
    }
    else
    {
        blocknum = entry->blocknum;
        txind = entry->txind;
        v = entry->v;
        spentflag = 0;
    }
    cointp = _get_cointp(&createdflag,cp->name,blocknum,txind,v);
    if ( createdflag != 0 || cointp->value == 0 )
    {
        cointp->entry = *entry;
        cointp->value = get_txoutstr(&cointp->numvouts,cointp->txid,cointp->coinaddr,script,cp,blocknum,txind,v);
        if ( cointp->script != 0 )
            free(cointp->script);
        cointp->script = clonestr(script);
        if ( entry->vinflag == 0 )
            get_txid(cp,cointp->txid,blocknum,txind,v);
    }
    if ( spentflag != 0 )
        cointp->entry.spent = 1;
    if ( cointp->entry.spent != 0 && cointp->script != 0 )
        free(cointp->script), cointp->script = 0;
    return(cointp);
}

uint64_t process_msigaddr(int32_t *numunspentp,uint64_t *unspentp,cJSON **transferjsonp,int32_t forceflag,struct NXT_asset *ap,char *NXTaddr,struct coin_info *cp,char *msigaddr,char *depositors_pubkey,uint32_t *buyNXTp)
{
    void set_NXTpubkey(char *,char *);
    struct address_entry *entries,*entry;
    int32_t i,n;
    //uint32_t createtime = 0;
    struct coin_txidind *cointp;
    uint64_t nxt64bits,unspent,pendingdeposits = 0;
    if ( ap->mult == 0 )
    {
        printf("ap->mult is ZERO for %s?\n",ap->name);
        return(0);
    }
    nxt64bits = calc_nxt64bits(NXTaddr);
    if ( depositors_pubkey != 0 && depositors_pubkey[0] == 0 )
        set_NXTpubkey(depositors_pubkey,NXTaddr);
    if ( (entries= get_address_entries(&n,cp->name,msigaddr)) != 0 )
    {
        if ( Debuglevel > 2 )
            printf(">>>>>>>>>>>>>>>> %d address entries for (%s)\n",n,msigaddr);
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            if ( entry->vinflag == 0 )
                pendingdeposits += process_msigdeposits(transferjsonp,forceflag,cp,entry,nxt64bits,ap,msigaddr,depositors_pubkey,buyNXTp);
            if ( Debuglevel > 2 )
                printf("process_msigaddr.(%s) %d of %d: vin.%d internal.%d spent.%d (%d %d %d)\n",msigaddr,i,n,entry->vinflag,entry->isinternal,entry->spent,entry->blocknum,entry->txind,entry->v);
            get_cointp(cp,entry);
        }
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            cointp = get_cointp(cp,entry);
            if ( cointp != 0 && cointp->entry.spent == 0 )
            {
                unspent = cointp->value;
                /*if ( (unspent= check_txout(&createtime,cp,cp->minconfirms,0,cointp->txid,cointp->entry.v,0)) == 0 )
                    cointp->entry.spent = 1;
                else if ( unspent != cointp->value )
                    printf("ERROR: %.8f != %.8f | %s %s.%d\n",dstr(unspent),dstr(cointp->value),cp->name,cointp->txid,cointp->entry.v);
                else*/
                {
                    cointp->unspent = unspent;
                    (*numunspentp)++;
                    (*unspentp) += unspent;
                    printf("%s | %16.8f unspenttotal %.8f\n",cointp->txid,dstr(cointp->unspent),dstr((*unspentp)));
                    update_unspent_funds(cp,cointp,0);
                }
            }
        }
        free(entries);
    } else printf("no entries for (%s)\n",msigaddr);
    return(pendingdeposits);
}

int32_t valid_msig(struct multisig_addr *msig,char *coinstr,char *specialNXT,char *gateways[],char *ipaddrs[],int32_t M,int32_t N)
{
    int32_t i,match = 0;
    char NXTaddr[64],gatewayNXTaddr[64],ipaddr[64];
    //printf("%s %s M.%d N.%d %llu vs %s (%s %s %s)\n",msig->coinstr,coinstr,msig->m,msig->n,(long long)msig->sender,specialNXT,gateways[0],gateways[1],gateways[2]);
    if ( strcmp(msig->coinstr,coinstr) == 0 && msig->m == M && msig->n == N )
    {
        expand_nxt64bits(NXTaddr,msig->sender);
        if ( strcmp(NXTaddr,specialNXT) == 0 )
            match++;
        else
        {
            for (i=0; i<N; i++)
                if ( strcmp(NXTaddr,gateways[i]) == 0 )
                    match++;
        }
        //printf("match.%d check for sender.(%s) vs special %s %s %s %s\n",match,NXTaddr,specialNXT,gateways[0],gateways[1],gateways[2]);
return(match);
        if ( match > 0 )
        {
            for (i=0; i<N; i++)
            {
                expand_nxt64bits(gatewayNXTaddr,msig->pubkeys[i].nxt64bits);
                if ( strcmp(gateways[i],gatewayNXTaddr) != 0 )
                {
                    printf("(%s != %s) ",gateways[i],gatewayNXTaddr);
                    break;
                }
            }
            printf("i.%d\n",i);
            if ( i == N )
                return(1);
            for (i=0; i<N; i++)
            {
                expand_ipbits(ipaddr,msig->pubkeys[i].ipbits);
                printf("(%s) ",ipaddr);
                if ( strcmp(ipaddrs[i],ipaddr) != 0 )
                    break;
            }
            printf("j.%d\n",i);
            if ( i == N )
                return(1);
        }
    }
    return(0);
}

char *get_default_MGWstr(char *str,int32_t ind)
{
    if ( str == 0 || str[0] == 0 )
        str = MGW_whitelist[ind];
    return(str);
}

void init_specialNXTaddrs(char *specialNXTaddrs[],char *ipaddrs[],char *specialNXT,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1,char *exclude2)
{
    int32_t i,n = 0;
    NXT0 = get_default_MGWstr(NXT0,0);
    NXT1 = get_default_MGWstr(NXT1,1);
    NXT2 = get_default_MGWstr(NXT2,2);
    exclude0 = get_default_MGWstr(exclude0,3);
    exclude1 = get_default_MGWstr(exclude1,4);
    exclude2 = get_default_MGWstr(exclude2,5);
    
    specialNXTaddrs[n++] = clonestr(NXT0), specialNXTaddrs[n++] = clonestr(NXT1), specialNXTaddrs[n++] = clonestr(NXT2);
    ipaddrs[0] = ip0, ipaddrs[1] = ip1, ipaddrs[2] = ip2;
    for (i=0; i<n; i++)
    {
        if ( specialNXTaddrs[i] == 0 )
            specialNXTaddrs[i] = "";
        if ( ipaddrs[i] == 0 )
            strcpy(ipaddrs[i],Server_names[i]);
    }
    if ( exclude0 != 0 && exclude0[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude0);
    if ( exclude1 != 0 && exclude1[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude1);
    if ( exclude2 != 0 && exclude2[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude2);
    specialNXTaddrs[n++] = clonestr(GENESISACCT);
    specialNXTaddrs[n++] = clonestr(specialNXT);
    specialNXTaddrs[n] = 0;
    for (i=0; i<n; i++)
        fprintf(stderr,"%p ",specialNXTaddrs[i]);
    fprintf(stderr,"numspecialNXT.%d\n",n);
}

uint64_t update_NXTblockchain_info(struct coin_info *cp,char *specialNXTaddrs[],char *refNXTaddr)
{
    struct coin_info *btcdcp;
    uint64_t pendingtxid;
    int32_t i;
    ready_to_xferassets(&pendingtxid);
    if ( (btcdcp= get_coin_info("BTCD")) != 0 )
    {
        update_NXT_transactions(specialNXTaddrs,-1,btcdcp->srvNXTADDR,cp);
        update_NXT_transactions(specialNXTaddrs,-1,btcdcp->privateNXTADDR,cp);
    }
    update_NXT_transactions(specialNXTaddrs,-1,refNXTaddr,cp);
    //update_NXT_transactions(specialNXTaddrs,2,refNXTaddr,cp);
    for (i=0; i<specialNXTaddrs[i][0]!=0; i++)
        update_NXT_transactions(specialNXTaddrs,-1,specialNXTaddrs[i],cp); // first numgateways of specialNXTaddrs[] are gateways
    update_msig_info(0,1,0); // sync MULTISIG_DATA
    return(pendingtxid);
}

char *wait_for_pendingtxid(struct coin_info *cp,char *specialNXTaddrs[],char *refNXTaddr,uint64_t pendingtxid)
{
    char txidstr[64],sender[64],receiver[64],assetstr[64],retbuf[1024],*retstr;
    cJSON *json;
    uint64_t val;
    expand_nxt64bits(txidstr,pendingtxid);
    sprintf(retbuf,"{\"result\":\"pendingtxid\",\"waitingfor\":\"%llu\"}",(long long)pendingtxid);
    if ( (retstr= issue_getTransaction(0,txidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (val= process_NXTtransaction(specialNXTaddrs,sender,receiver,json,refNXTaddr,assetstr,1,cp)) != 0 )
                sprintf(retbuf,"{\"result\":\"pendingtxid\",\"processed\":\"%llu\"}",(long long)val);
            free_json(json);
        }
        free(retstr);
    }
    return(clonestr(retbuf));
}

uint64_t process_deposits(cJSON **jsonp,uint64_t *unspentp,struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,char *ipaddrs[],char *specialNXTaddrs[],char *refNXTaddr,struct NXT_asset *ap,int32_t transferassets,uint64_t circulation,char *depositor,char *depositors_pubkey)
{
    uint64_t pendingtxid,unspent = 0,total = 0;
    int32_t i,m,max,readyflag,tmp,tmp2,nonz,numunspent;
    struct multisig_addr *msig;
    struct address_entry *entries;
    struct unspent_info *up;
    char numstr[128];
    cJSON *array;
    *unspentp = 0;
    array = cJSON_CreateArray();
    if ( msigs != 0 )
    {
        readyflag = ready_to_xferassets(&pendingtxid);
        printf("readyflag.%d depositor.(%s) (%s)\n",readyflag,depositor,depositors_pubkey);
        for (i=max=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 && (entries= get_address_entries(&m,cp->name,msig->multisigaddr)) != 0 )
            {
                max += m;
                free(entries);
            }
        }
        if ( Debuglevel > 2 )
            printf("got n.%d msigs readyflag.%d | max.%d pendingtxid.%llu depositor.(%s)\n",nummsigs,readyflag,max,(long long)pendingtxid,depositor);
        unspent = nonz = numunspent = 0;
        up = &cp->unspent;
        update_unspent_funds(cp,0,max);
        for (i=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 )
            {
                if ( max > 0 && valid_msig(msig,cp->name,refNXTaddr,specialNXTaddrs,ipaddrs,2,3) != 0 )//&& (depositor == 0 || strcmp(depositor,refNXTaddr) == 0) )
                {
                    if ( Debuglevel > 2 )
                        printf("MULTISIG: %s: %d of %d %s %s (%s)\n",cp->name,i,nummsigs,msig->coinstr,msig->multisigaddr,msig->NXTpubkey);
                    update_NXT_transactions(specialNXTaddrs,2,msig->NXTaddr,cp);
                    if ( transferassets == 0 || (readyflag > 0 && pendingtxid == 0) )
                    {
                        char tmp3[64];
                        tmp = numunspent;
                        tmp2 = msig->buyNXT;
                        total += process_msigaddr(&numunspent,&unspent,&array,transferassets,ap,msig->NXTaddr,cp,msig->multisigaddr,msig->NXTpubkey,&msig->buyNXT);
                        if ( numunspent > tmp )
                            nonz++;
                        if ( msig->buyNXT == 0 && tmp2 != 0 )
                        {
                            expand_nxt64bits(tmp3,msig->sender);
                            update_msig_info(msig,1,tmp3);
                        }
                    }
                }
            }
        }
        if ( up->num > 1 )
        {
            //fprintf(stderr,"call sort_vps with %p num.%d\n",up,up->num);
            sort_vps(up->vps,up->num);
            for (i=0; i<10&&i<up->num; i++)
                fprintf(stderr,"%d (%s) (%s).%d %.8f\n",i,up->vps[i]->txid,up->vps[i]->coinaddr,up->vps[i]->entry.v,dstr(up->vps[i]->value));
        }
        fprintf(stderr,"max %.8f min %.8f median %.8f |unspent %.8f numunspent.%d in nonz.%d accts\n",dstr(up->maxavail),dstr(up->minavail),dstr((up->maxavail+up->minavail)/2),dstr(up->unspent),numunspent,nonz);
    }
    sprintf(numstr,"%.8f",dstr(circulation)), cJSON_AddItemToObject(*jsonp,"circulation",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(unspent)), cJSON_AddItemToObject(*jsonp,"unspent",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(total)), cJSON_AddItemToObject(*jsonp,"pendingdeposits",cJSON_CreateString(numstr));
    if ( cJSON_GetArraySize(array) > 0 )
        cJSON_AddItemToObject(*jsonp,"alldeposits",array);
    else free_json(array);
    *unspentp = unspent;
    return(total);
}

uint64_t get_accountassets(int32_t height,struct NXT_asset *ap,char *NXTacct)
{
    cJSON *json;
    uint64_t quantity = 0;
    char cmd[4096],*retstr = 0;
    sprintf(cmd,"%s=getAccountAssets&asset=%llu&account=%s",_NXTSERVER,(long long)ap->assetbits,NXTacct);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%d",height);
    if ( (retstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (quantity= get_API_nxt64bits(cJSON_GetObjectItem(json,"quantityQNT"))) != 0 && ap->mult != 0 )
                quantity *= ap->mult;
            free_json(json);
        }
        free(retstr);
    }
    return(quantity);
}

uint64_t calc_circulation(int32_t height,struct NXT_asset *ap,char *specialNXTaddrs[])
{
    uint64_t quantity,circulation = 0;
    char cmd[4096],acct[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json,*array,*item;
    int32_t i,n;
    sprintf(cmd,"%s=getAssetAccounts&asset=%llu",_NXTSERVER,(long long)ap->assetbits);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%d",height);
    if ( (retstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        fprintf(stderr,"circ.(%s) <- (%s)\n",retstr,cmd);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                //fprintf(stderr,"n.%d\n",n);
                for (i=0; i<n; i++)
                {
                    //fprintf(stderr,"i.%d of n.%d\n",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    //printf("%s ",acct);
                    if ( acct[0] != 0 && in_specialNXTaddrs(specialNXTaddrs,acct) == 0 )
                    {
                        if ( (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                        {
                            circulation += quantity;
                            if ( quantity*ap->mult > 1000*SATOSHIDEN )
                                printf("BIGACCT.%d %s %.8f\n",i,acct,dstr(quantity*ap->mult));
                        //printf("%llu, ",(long long)quantity);
                        }
                    }
                }
            }
        }
        free(retstr);
    }
    return(circulation * ap->mult);
}

cJSON *gen_autoconvert_json(struct NXT_assettxid *tp)
{
    cJSON *json;
    int32_t delta;
    char typestr[64],numstr[64];
    if ( tp->convname[0] == 0 )
        return(0);
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"name",cJSON_CreateString(tp->convname));
    if ( tp->convwithdrawaddr != 0 )
        cJSON_AddItemToObject(json,"coinaddr",cJSON_CreateString(tp->convwithdrawaddr));
    if ( tp->teleport[0] != 0 )
    {
        if ( strcmp(tp->teleport,"send") == 0 )
            strcpy(typestr,"telepod");
        else strcpy(typestr,"emailpod");
    } else strcpy(typestr,"convert");
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(typestr));
    if ( tp->minconvrate != 0 )
        sprintf(numstr,"%.8f",tp->minconvrate), cJSON_AddItemToObject(json,"convrate",cJSON_CreateString(numstr));
    if ( tp->convexpiration != 0 )
    {
        delta = (int32_t)(tp->convexpiration - time(NULL));
        cJSON_AddItemToObject(json,"expires",cJSON_CreateNumber(delta));
    }
    return(json);
}

char *calc_withdrawaddr(char *withdrawaddr,struct coin_info *cp,struct NXT_assettxid *tp,cJSON *argjson)
{
    cJSON *json;
    int32_t convert = 0;
    struct coin_info *newcp;
    char buf[MAX_JSON_FIELD],autoconvert[MAX_JSON_FIELD],issuer[MAX_JSON_FIELD],*retstr;
    copy_cJSON(withdrawaddr,cJSON_GetObjectItem(argjson,"withdrawaddr"));
//if ( withdrawaddr[0] != 0 )
//    return(withdrawaddr);
//else return(0);
    if ( tp->convname[0] != 0 )
    {
        withdrawaddr[0] = 0;
        return(0);
    }
    copy_cJSON(autoconvert,cJSON_GetObjectItem(argjson,"autoconvert"));
    copy_cJSON(buf,cJSON_GetObjectItem(argjson,"teleport")); // "send" or <emailaddr>
    safecopy(tp->teleport,buf,sizeof(tp->teleport));
    tp->convassetid = tp->assetbits;
    if ( autoconvert[0] != 0 )
    {
        if ( (newcp= get_coin_info(autoconvert)) == 0 )
        {
            if ( (retstr= issue_getAsset(0,autoconvert)) != 0 )
            {
                if ( (json= cJSON_Parse(retstr)) != 0 )
                {
                    copy_cJSON(issuer,cJSON_GetObjectItem(json,"account"));
                    if ( is_trusted_issuer(issuer) > 0 )
                    {
                        copy_cJSON(tp->convname,cJSON_GetObjectItem(json,"name"));
                        convert = 1;
                    }
                    free_json(json);
                }
            }
        }
        else
        {
            strcpy(tp->convname,newcp->name);
            convert = 1;
        }
        if ( convert != 0 )
        {
            tp->minconvrate = get_API_float(cJSON_GetObjectItem(argjson,"rate"));
            tp->convexpiration = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"expiration"),0);
            if ( withdrawaddr[0] != 0 ) // no address means to create user credit
                tp->convwithdrawaddr = clonestr(withdrawaddr);
        }
        else withdrawaddr[0] = autoconvert[0] = 0;
    }
    //printf("withdrawaddr.(%s) autoconvert.(%s)\n",withdrawaddr,autoconvert);
    if ( withdrawaddr[0] == 0 || autoconvert[0] != 0 )
        return(0);
    //printf("return.(%s)\n",withdrawaddr);
    return(withdrawaddr);
}

char *parse_withdraw_instructions(char *destaddr,char *NXTaddr,struct coin_info *cp,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",cp->name,cJSON_Print(argjson));
        retstr = 0;
    }
    else
    {
        amount = tp->quantity * ap->mult;
        if ( tp->comment != 0 && (tp->comment[0] == '{' || tp->comment[0] == '[') && (argjson= cJSON_Parse(tp->comment)) != 0 )
        {
            if ( calc_withdrawaddr(withdrawaddr,cp,tp,argjson) == 0 )
            {
                printf("no withdraw.(%s) or autoconvert.(%s)\n",withdrawaddr,tp->comment);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = cp->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("minimum withdrawal must be more than %.8f %s\n",dstr(minwithdraw),cp->name);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("no withdraw address for %.8f | ",dstr(amount));
                retstr = 0;
            }
            else if ( cp != 0 && validate_coinaddr(pubkey,cp,withdrawaddr) < 0 )
            {
                printf("invalid address.(%s) for NXT.%s %.8f validate.%d\n",withdrawaddr,NXTaddr,dstr(amount),validate_coinaddr(pubkey,cp,withdrawaddr));
                retstr = 0;
            }
        }
    }
    printf("withdraw addr.(%s) for (%s)\n",withdrawaddr,NXTaddr);
    if ( retstr != 0 )
        strcpy(retstr,withdrawaddr);
    if ( argjson != 0 )
        free_json(argjson);
    return(retstr);
}

int32_t add_redeem(char *destaddrs[],uint64_t *destamounts,uint64_t *redeems,uint64_t *pending_withdrawp,struct coin_info *cp,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems)
{
    int32_t j;
    uint64_t amount;
    if ( destaddr == 0 || destaddr[0] == 0 || numredeems >= MAX_MULTISIG_OUTPUTS-1 )
    {
        printf("add_redeem with null destaddr.%p numredeems.%d\n",destaddr,numredeems);
        return(numredeems);
    }
    amount = tp->quantity * ap->mult - (cp->txfee + cp->NXTfee_equiv);
    (*pending_withdrawp) += amount;
    if ( numredeems > 0 )
    {
        for (j=0; j<numredeems; j++)
            if ( redeems[j] == tp->txidbits )
                break;
    } else j = 0;
    if ( j == numredeems )
    {
        destaddrs[numredeems] = clonestr(destaddr);
        destamounts[numredeems] = amount;
        redeems[numredeems] = tp->txidbits;
        numredeems++;
        printf("withdraw_addr.%d R.(%llu %.8f %s)\n",j,(long long)tp->txidbits,dstr(destamounts[j]),destaddrs[j]);
    }
    else printf("ERROR: duplicate redeembits.%llu numredeems.%d j.%d\n",(long long)tp->txidbits,numredeems,j);
    return(numredeems);
}

int32_t add_destaddress(struct rawtransaction *rp,char *destaddr,uint64_t amount)
{
    int32_t i;
    if ( rp->numoutputs > 0 )
    {
        for (i=0; i<rp->numoutputs; i++)
        {
            printf("compare.%d of %d: %s vs %s\n",i,rp->numoutputs,rp->destaddrs[i],destaddr);
            if ( strcmp(rp->destaddrs[i],destaddr) == 0 )
                break;
        }
    } else i = 0;
    printf("add[%d] of %d: %.8f -> %s\n",i,rp->numoutputs,dstr(amount),destaddr);
    if ( i == rp->numoutputs )
    {
        if ( rp->numoutputs >= (int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)) )
        {
            printf("overflow %d vs %ld\n",rp->numoutputs,(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)));
            return(-1);
        }
        printf("create new output.%d (%s)\n",rp->numoutputs,destaddr);
        rp->destaddrs[rp->numoutputs++] = destaddr;
    }
    rp->destamounts[i] += amount;
    return(i);
}

int32_t process_destaddr(int32_t *alreadysentp,cJSON **arrayp,char *destaddrs[MAX_MULTISIG_OUTPUTS],uint64_t destamounts[MAX_MULTISIG_OUTPUTS],uint64_t redeems[MAX_MULTISIG_OUTPUTS],uint64_t *pending_withdrawp,struct coin_info *cp,uint64_t nxt64bits,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems,char *sender)
{
    struct address_entry *entries,*entry;
    struct coin_txidind *cointp;
    struct unspent_info *up;
    struct NXT_acct *np;
    int32_t j,n,createdflag;
    char numstr[128],rsacct[64];
    cJSON *item;
    double pending;
    *alreadysentp = 0;
    np = get_NXTacct(&createdflag,Global_mp,sender);
    fprintf(stderr,"[");
    if ( (entries= get_address_entries(&n,cp->name,destaddr)) != 0 )
    {
        fprintf(stderr,"].%d ",n);
        *alreadysentp = 1;
        for (j=0; j<n; j++)
        {
            entry = &entries[j];
            if ( entry->vinflag == 0 )
            {
                // would save on _get_cointp calls, but since they are so fast, no point to do this
                /*for (i=0; i<n; i++)
                    if ( entries[i].vinflag != 0 && entries[i].blocknum == entry->blocknum && entries[i].txind == entry->txind && entries[i].v == entry->v )
                        break;
                if ( i == n )*/
                {
                    cointp = _get_cointp(&createdflag,cp->name,entry->blocknum,entry->txind,entry->v);
                    if ( cointp->redeemtxid == tp->redeemtxid )
                        break;
                }
            }
        }
        if ( j == n )
        {
            // following is too unproven, messing with bitcoin tx format, could be good to flag tx as MGW tx
            /*for (j=0; j<n; j++)
            {
                entry = &entries[j];
                if ( entry->vinflag == 0 )
                {
                    cointp = get_cointp(cp,entry);
                    if ( cointp->seq0 == 0 )
                    {
                        cointp->redeemtxid = -1;
                        printf("check txid.(%s)\n",cointp->txid);
                        if ( (rawtx= get_rawtransaction(cp,cointp->txid)) != 0 )
                        {
                            cointp->seq0 = extract_sequenceid(&cointp->numinputs,cp,rawtx,0);
                            cointp->seq1 = extract_sequenceid(&cointp->numinputs,cp,rawtx,1);
                            cointp->redeemtxid = ((uint64_t)cointp->seq1 << 32) | cointp->seq0;
                            printf("%x %x -> redeem.%llu\n",cointp->seq0,cointp->seq1,(long long)cointp->redeemtxid);
                            free(rawtx);
                        }
                    }
                    if ( cointp->redeemtxid == tp->redeemtxid )
                        break;
                }
            }
            if ( j == n )*/
            if ( is_limbo_redeem(cp,tp->redeemtxid) == 0 )
            {
                *alreadysentp = 0;
                up = &cp->unspent;
                tp->numconfs = get_NXTconfirms(tp->redeemtxid);
                printf("numredeems.%d (%p %p) PENDING REDEEM numconfs.%d %s %s %llu %llu %.8f %.8f | %llu\n",numredeems,up->maxvp,up->minvp,tp->numconfs,cp->name,destaddr,(long long)nxt64bits,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
                item = cJSON_CreateObject();
                conv_rsacctstr(rsacct,np->H.nxt64bits);
                cJSON_AddItemToObject(item,"NXT",cJSON_CreateString(rsacct));
                sprintf(numstr,"%llu",(long long)tp->redeemtxid), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(numstr));
                cJSON_AddItemToObject(item,"destaddr",cJSON_CreateString(destaddr));
                sprintf(numstr,"%.8f",dstr(tp->quantity * ap->mult)), cJSON_AddItemToObject(item,"amount",cJSON_CreateString(numstr));
                cJSON_AddItemToObject(item,"confirms",cJSON_CreateNumber(tp->numconfs));
                cJSON_AddItemToArray(*arrayp,item);
                if ( (pending= enough_confirms(np->redeemed,tp->estNXT,tp->numconfs,1)) > 0 )
                {
                    numredeems = add_redeem(destaddrs,destamounts,redeems,pending_withdrawp,cp,ap,destaddr,tp,numredeems);
                    np->redeemed += tp->estNXT;
                    printf("NXT.(%s) redeemed %.8f %p numredeems.%d (%s) %.8f %llu\n",sender,np->redeemed,&destaddrs[numredeems-1],numredeems,destaddrs[numredeems-1],dstr(destamounts[numredeems-1]),(long long)redeems[numredeems-1]);
                }
                else
                {
                    sprintf(numstr,"%.2f",dstr(np->redeemed)), cJSON_AddItemToObject(item,"already",cJSON_CreateString(numstr));
                    sprintf(numstr,"%.2f",dstr(tp->estNXT)), cJSON_AddItemToObject(item,"estNXT",cJSON_CreateString(numstr));
                    sprintf(numstr,"%.3f",pending), cJSON_AddItemToObject(item,"wait",cJSON_CreateString(numstr));
                }
            }
        }
        free(entries);
    }
    return(numredeems);
}

char *create_batch_jsontxt(struct coin_info *cp,int *firstitemp)
{
    struct rawtransaction *rp = &cp->BATCH.rawtx;
    cJSON *json,*obj,*array = 0;
    char *jsontxt,redeemtxid[128];
    int32_t i,ind;
    json = cJSON_CreateObject();
    obj = cJSON_CreateNumber(cp->coinid); cJSON_AddItemToObject(json,"coinid",obj);
    obj = cJSON_CreateNumber(issue_getTime(0)); cJSON_AddItemToObject(json,"timestamp",obj);
    obj = cJSON_CreateString(coinid_str(cp->coinid)); cJSON_AddItemToObject(json,"coin",obj);
    obj = cJSON_CreateString(cp->BATCH.W.cointxid); cJSON_AddItemToObject(json,"cointxid",obj);
    obj = cJSON_CreateNumber(cp->BATCH.rawtx.batchcrc); cJSON_AddItemToObject(json,"batchcrc",obj);
    if ( rp->numredeems > 0 )
    {
        ind = *firstitemp;
        for (i=0; i<32; i++)    // 32 * 22 = 768 bytes AM total limit 1000 bytes
        {
            ind = *firstitemp + i;
            if ( ind >= rp->numredeems )
                break;
            if ( array == 0 )
                array = cJSON_CreateArray();
            expand_nxt64bits(redeemtxid,rp->redeems[ind]);
            cJSON_AddItemToArray(array,cJSON_CreateString(redeemtxid));
        }
        *firstitemp = ind + 1;
        if ( array != 0 )
            cJSON_AddItemToObject(json,"redeems",array);
    }
    jsontxt = cJSON_Print(json);
    free_json(json);
    return(jsontxt);
}

uint64_t broadcast_moneysentAM(struct coin_info *cp,int32_t height)
{
    cJSON *argjson;
    uint64_t AMtxidbits = 0;
    int32_t i,firstitem = 0;
    char AM[4096],*jsontxt,*AMtxid = 0;
    struct json_AM *ap = (struct json_AM *)AM;
    if ( cp == 0 || Global_mp->gatewayid < 0 )
        return(0);
    //jsontxt = create_moneysent_jsontxt(coinid,wp);
    i = 0;
    while ( firstitem < cp->BATCH.rawtx.numredeems )
    {
        jsontxt = create_batch_jsontxt(cp,&firstitem);
        if ( jsontxt != 0 )
        {
            set_json_AM(ap,GATEWAY_SIG,MONEY_SENT,NXTISSUERACCT,Global_mp->timestamp,jsontxt,1);
            printf("%d BATCH_AM.(%s)\n",i,jsontxt);
            i++;
            AMtxid = submit_AM(0,NXTISSUERACCT,&ap->H,0,cp->srvNXTACCTSECRET);
            if ( AMtxid == 0 )
            {
                printf("Error submitting moneysent for (%s)\n",jsontxt);
                for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                    printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
                printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,cp->BATCH.W.cointxid,jsontxt), sleep(60);
                fprintf(stderr,"broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,cp->BATCH.W.cointxid,jsontxt), sleep(60);
                exit(-1);
            }
            else
            {
                AMtxidbits = calc_nxt64bits(AMtxid);
                free(AMtxid);
                if ( AMtxidbits != 0 )
                    add_pendingxfer(0,AMtxidbits);
                argjson = cJSON_Parse(jsontxt);
                if ( argjson != 0 )
                    update_redeembits(argjson,AMtxidbits); //update_money_sent(argjson,AMtxid,height);
                else
                {
                    for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                        printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
                    printf("broadcast_moneysentAM: %s failed. AMtxid.%llu FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,(long long)AMtxid,cp->BATCH.W.cointxid,jsontxt);
                    fprintf(stderr,"broadcast_moneysentAM: %s failed. AMtxid.%llu FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,(long long)AMtxid,cp->BATCH.W.cointxid,jsontxt);
                    exit(-1);
                }
            }
            free(jsontxt);
        }
        else
        {
            for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
            printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s\n",coinid_str(cp->coinid),cp->BATCH.W.cointxid);
            fprintf(stderr,"broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s\n",coinid_str(cp->coinid),cp->BATCH.W.cointxid);
            exit(-1);
        }
    }
    return(AMtxidbits);
}

char *sign_and_sendmoney(uint64_t *AMtxidp,struct coin_info *cp,int32_t height)
{
    char *retstr = 0;
    *AMtxidp = 0;
    fprintf(stderr,"achieved consensus and sign! %s\n",cp->BATCH.rawtx.batchsigned);
    if ( (retstr= submit_withdraw(cp,&cp->BATCH,&cp->withdrawinfos[(Global_mp->gatewayid + 1) % NUM_GATEWAYS])) != 0 )
    {
        safecopy(cp->BATCH.W.cointxid,retstr,sizeof(cp->BATCH.W.cointxid));
        *AMtxidp = broadcast_moneysentAM(cp,height);
        //backupwallet(cp,cp->coinid);
        return(retstr);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",cp->BATCH.rawtx.batchsigned);
    return(0);
}

uint64_t process_consensus(cJSON **jsonp,struct coin_info *cp,int32_t sendmoney)
{
    struct batch_info *otherwp;
    int32_t i,readyflag,gatewayid,matches = 0;
    char numstr[64],*cointxid;
    struct rawtransaction *rp;
    cJSON *array,*item;
    uint64_t pendingtxid,AMtxid = 0;
    if ( Global_mp->gatewayid < 0 )
        return(0);
    readyflag = ready_to_xferassets(&pendingtxid);
    printf("%s: readyflag.%d gateway.%d\n",cp->name,readyflag,Global_mp->gatewayid);
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        otherwp = &cp->withdrawinfos[gatewayid];
        if ( cp->BATCH.rawtx.batchcrc != otherwp->rawtx.batchcrc )
        {
            fprintf(stderr,"%08x miscompares with gatewayid.%d which has crc %08x\n",cp->BATCH.rawtx.batchcrc,gatewayid,otherwp->rawtx.batchcrc);
        }
        else if ( cp->BATCH.rawtx.batchcrc != 0 )
            matches++;
    }
    rp = &cp->withdrawinfos[Global_mp->gatewayid].rawtx;
    array = cJSON_CreateArray();
    printf("json.%p numredeems.%d\n",*jsonp,rp->numredeems);
    if ( rp->numredeems > 0 )
    {
        for (i=0; i<rp->numredeems; i++)
        {
            fprintf(stderr,"redeem entry i.%d (%llu %p %.8f)\n",i,(long long)rp->redeems[i],rp->destaddrs[i],dstr(rp->destamounts[i]));
            item = cJSON_CreateObject();
            sprintf(numstr,"%llu",(long long)rp->redeems[i]), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(numstr));
            if ( rp->destaddrs[i+1] != 0 )
                cJSON_AddItemToObject(item,"destaddr",cJSON_CreateString(rp->destaddrs[i+1]));
            sprintf(numstr,"%.8f",dstr(rp->destamounts[i+1])), cJSON_AddItemToObject(item,"amount",cJSON_CreateString(numstr));
            cJSON_AddItemToArray(array,item);
        }
    }
    if ( cJSON_GetArraySize(array) > 0 )
        cJSON_AddItemToObject(*jsonp,"allredeems",array);
    else free_json(array);
    if ( sendmoney != 0 && matches == NUM_GATEWAYS )
    {
        fprintf(stderr,"all gateways match\n");
        if ( readyflag != 0 )//Global_mp->gatewayid == 0 )
        {
            if ( rp->batchsigned != 0 && (cointxid= sign_and_sendmoney(&AMtxid,cp,(uint32_t)cp->RTblockheight)) != 0 )
            {
                cJSON_AddItemToObject(*jsonp,"batchsigned",cJSON_CreateString(rp->batchsigned));
                cJSON_AddItemToObject(*jsonp,"cointxid",cJSON_CreateString(cointxid));
                sprintf(numstr,"%llu",(long long)AMtxid), cJSON_AddItemToObject(*jsonp,"AMtxid",cJSON_CreateString(numstr));
                fprintf(stderr,"done and publish coin.(%s) AM.%llu\n",cointxid,(long long)AMtxid);
                //publish_withdraw_info(cp,&cp->BATCH);
                free(cointxid);
            }
            else
            {
                fprintf(stderr,"error signing?\n");
                cp->BATCH.rawtx.batchcrc = 0;
                cp->withdrawinfos[Global_mp->gatewayid].rawtx.batchcrc = 0;
                cp->withdrawinfos[Global_mp->gatewayid].W.cointxid[0] = 0;
            }
        }
    }
    printf("last array AMtxid.%llu\n",(long long)AMtxid);
    array = cJSON_CreateArray();
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        cJSON_AddItemToArray(array,cJSON_CreateNumber(cp->withdrawinfos[gatewayid].rawtx.batchcrc));
    cJSON_AddItemToObject(*jsonp,"crcs",array);
    cJSON_AddItemToObject(*jsonp,"allwithdraws",cJSON_CreateNumber(rp->numredeems));
    sprintf(numstr,"%.8f",dstr(rp->amount)), cJSON_AddItemToObject(*jsonp,"pending_withdraw",cJSON_CreateString(numstr));
    return(AMtxid);
}

void process_withdraws(cJSON **jsonp,struct multisig_addr **msigs,int32_t nummsigs,uint64_t unspent,struct coin_info *cp,struct NXT_asset *ap,char *specialNXT,int32_t sendmoney,uint64_t circulation,uint64_t pendingdeposits,char *redeemer)
{
    struct NXT_assettxid *tp;
    struct rawtransaction *rp;
    cJSON *array;
    int32_t i,j,numredeems,alreadyspent;
    uint64_t destamounts[MAX_MULTISIG_OUTPUTS],redeems[MAX_MULTISIG_OUTPUTS],nxt64bits,balance,sum,pending_withdraw = 0;
    char withdrawaddr[64],sender[64],redeemtxid[64],numstr[64],*destaddrs[MAX_MULTISIG_OUTPUTS],*destaddr="",*batchsigned,*str;
    if ( ap->num <= 0 )
        return;
    rp = &cp->BATCH.rawtx;
    clear_BATCH(rp);
    rp->numoutputs = init_batchoutputs(cp,rp,cp->txfee);
    numredeems = 0;
    memset(redeems,0,sizeof(redeems));
    memset(destaddrs,0,sizeof(destaddrs));
    memset(destamounts,0,sizeof(destamounts));
    nxt64bits = calc_nxt64bits(specialNXT);
    array = cJSON_CreateArray();
    for (i=sum=0; i<ap->num; i++)
    {
        tp = ap->txids[i];
        if ( Debuglevel > 2 )
            printf("%d of %d: (%s) redeem.%llu (%llu vs %llu) (%llu vs %llu)\n",i,ap->num,tp->comment,(long long)tp->redeemtxid,(long long)tp->receiverbits,(long long)nxt64bits,(long long)tp->assetbits,(long long)ap->assetbits);
        str = (tp->AMtxidbits != 0) ? ": REDEEMED" : " <- redeem";
        if ( tp->redeemtxid != 0 && tp->receiverbits == nxt64bits && tp->assetbits == ap->assetbits && tp->U.assetoshis >= MIN_DEPOSIT_FACTOR*(cp->txfee + cp->NXTfee_equiv) )
        {
            expand_nxt64bits(sender,tp->senderbits);
            if ( tp->AMtxidbits == 0 && (destaddr= parse_withdraw_instructions(withdrawaddr,sender,cp,tp,ap)) != 0 && destaddr[0] != 0 )
            {
                stripwhite(destaddr,strlen(destaddr));
                printf("i.%d of %d: process_destaddr.(%s) %.8f\n",i,ap->num,destaddr,dstr(tp->U.assetoshis));
                numredeems = process_destaddr(&alreadyspent,&array,destaddrs,destamounts,redeems,&pending_withdraw,cp,nxt64bits,ap,destaddr,tp,numredeems,sender);
                tp->AMtxidbits = alreadyspent;
                if ( numredeems >= MAX_MULTISIG_OUTPUTS-1 )
                    break;
            }
        }
        if ( Debuglevel > 2 )
            printf("%s (%s, %s) %llu %s %llu %.8f %.8f | %llu\n",cp->name,destaddr,withdrawaddr,(long long)nxt64bits,str,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
    }
    cJSON_AddItemToObject(*jsonp,"redeems",array);
    balance = unspent - pending_withdraw - circulation;
    sprintf(numstr,"%.8f",dstr(balance)), cJSON_AddItemToObject(*jsonp,"revenues",cJSON_CreateString(numstr));
    if ( cp->boughtNXT > 0 && balance != 0. )
    {
        cJSON_AddItemToObject(*jsonp,"boughtNXT",cJSON_CreateNumber(cp->boughtNXT));
        sprintf(numstr,"%.8f",dstr(cp->boughtNXT / balance)), cJSON_AddItemToObject(*jsonp,"costbasis",cJSON_CreateString(numstr));
    }
    array = cJSON_CreateArray();
    if ( (int64_t)pending_withdraw >= ((5 * cp->NXTfee_equiv) - (numredeems * (cp->txfee + cp->NXTfee_equiv))) )
    {
        for (sum=j=0; j<numredeems&&rp->numoutputs<(int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs))-1; j++)
        {
            fprintf(stderr,"rp->numoutputs.%d/%d j.%d of %d: (%s) [%llu %.8f]\n",rp->numoutputs,(int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs))-1,j,numredeems,destaddrs[j],(long long)redeems[j],dstr(destamounts[j]));
            if ( add_destaddress(rp,destaddrs[j],destamounts[j]) < 0 )
            {
                printf("error adding %s %.8f | numredeems.%d numoutputs.%d\n",destaddrs[j],dstr(destamounts[j]),rp->numredeems,rp->numoutputs);
                break;
            }
            sum += destamounts[j];
            expand_nxt64bits(redeemtxid,redeems[j]);
            rp->redeems[rp->numredeems++] = redeems[j];
            if ( rp->numredeems >= (int)(sizeof(rp->redeems)/sizeof(*rp->redeems)) )
            {
                printf("max numredeems\n");
                break;
            }
        }
        printf("pending_withdraw %.8f -> sum %.8f numredeems.%d numoutputs.%d\n",dstr(pending_withdraw),dstr(sum),rp->numredeems,rp->numoutputs);
        pending_withdraw = sum;
        batchsigned = calc_batchwithdraw(msigs,nummsigs,cp,rp,pending_withdraw,unspent,ap);
        if ( batchsigned != 0 )
        {
            printf("BATCHSIGNED.(%s)\n",batchsigned);
            cp->BATCH.balance = balance;
            cp->BATCH.circulation = circulation;
            cp->BATCH.unspent = unspent;
            cp->BATCH.pendingdeposits = pendingdeposits;
            cp->BATCH.boughtNXT = cp->boughtNXT;
            if ( sendmoney == 0 )
                publish_withdraw_info(cp,&cp->BATCH);
            process_consensus(jsonp,cp,sendmoney);
            free(batchsigned);
        }
    }
    else
    {
        if ( numredeems > 0 )
            printf("%.8f is not enough to pay for MGWfees.%s %.8f for %d redeems\n",dstr(pending_withdraw),cp->name,dstr(cp->NXTfee_equiv),numredeems);
        pending_withdraw = 0;
    }
}

int32_t cmp_batch_depositinfo(struct batch_info *refbatch,struct batch_info *batch)
{
    if ( refbatch->balance != batch->balance || refbatch->circulation != batch->circulation || refbatch->unspent != batch->unspent || refbatch->pendingdeposits != batch->pendingdeposits || refbatch->boughtNXT != batch->boughtNXT )
        return(-1);
    return(0);
}

cJSON *process_MGW(int32_t actionflag,struct coin_info *cp,struct NXT_asset *ap,char *ipaddrs[3],char **specialNXTaddrs,char *issuer,double startmilli,char *NXTaddr,char *depositors_pubkey)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[128];
    uint64_t pendingdeposits,circulation,unspent = 0;
    char *retstr;
    int32_t i,nummsigs;
    struct multisig_addr **msigs;
    pendingdeposits = 0;
    circulation = calc_circulation(0,ap,specialNXTaddrs);
    fprintf(stderr,"circulation %.8f\n",dstr(circulation));
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        printf("nummsigs.%d\n",nummsigs);
        if ( actionflag >= 0 )
        {
            pendingdeposits = process_deposits(&json,&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,issuer,ap,actionflag > 0,circulation,NXTaddr,depositors_pubkey);
            retstr = cJSON_Print(json);
            fprintf(stderr,"actionflag.%d retstr.(%s)\n",actionflag,retstr);
            free(retstr), retstr = 0;
        }
        if ( actionflag <= 0 )
        {
            if ( actionflag < 0 )
                pendingdeposits = process_deposits(&json,&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,issuer,ap,actionflag > 0,circulation,NXTaddr,depositors_pubkey);
            process_withdraws(&json,msigs,nummsigs,unspent,cp,ap,issuer,actionflag < 0,circulation,pendingdeposits,NXTaddr);
        }
        sprintf(numstr,"%.3f",(milliseconds()-startmilli)/1000.), cJSON_AddItemToObject(json,"seconds",cJSON_CreateString(numstr));
        for (i=0; i<nummsigs; i++)
            free(msigs[i]);
        free(msigs);
    }
    return(json);
}

void MGW_useracct_str(cJSON **jsonp,int32_t actionflag,struct coin_info *cp,struct NXT_asset *ap,uint64_t nxt64bits,char *issuerNXT,char **specialNXTaddrs)
{
    
    char coinaddr[1024],txidstr[1024],depositaddr[128],withdrawaddr[512],rsacct[64],depositstr[64],numstr[128],redeemstr[128],NXTaddr[64];
    struct multisig_addr **msigs,*msig;
    struct address_entry *entries;
    struct NXT_assettxid *tp;
    cJSON *autojson,*array,*item;
    int32_t i,j,n,nummsigs,lastbuyNXT,buyNXT = 0;
    uint64_t pendingtxid,value,deposittxid,withdrew,issuerbits,balance,assetoshis,pending_withdraws,pending_deposits;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( *jsonp == 0 )
        *jsonp = cJSON_CreateObject();
    depositaddr[0] = 0;
    lastbuyNXT = 0;
    array = cJSON_CreateArray();
    issuerbits = calc_nxt64bits(issuerNXT);
    pending_withdraws = pending_deposits = 0;
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        for (i=0; i<nummsigs; i++)
        {
            if ( (msig= msigs[i]) != 0 )
            {
                if ( strcmp(msig->NXTaddr,NXTaddr) == 0 )
                {
                    buyNXT += msig->buyNXT;
                    if ( msig->multisigaddr[0] != 0 )
                    {
                        lastbuyNXT = msig->buyNXT;
                        strcpy(depositaddr,msig->multisigaddr);
                    }
                    if ( (entries= get_address_entries(&n,cp->name,msig->multisigaddr)) != 0 )
                    {
                        printf("got %d entries for (%s)\n",n,msig->multisigaddr);
                        for (j=0; j<n; j++)
                        {
                            item = cJSON_CreateObject();
                            cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(entries[j].v));
                            cJSON_AddItemToObject(item,"height",cJSON_CreateNumber(entries[j].blocknum));
                            if ( (value= conv_address_entry(coinaddr,txidstr,0,cp,&entries[j])) != 0 )
                            {
                                cJSON_AddItemToObject(item,"txid",cJSON_CreateString(txidstr));
                                cJSON_AddItemToObject(item,"addr",cJSON_CreateString(coinaddr));
                                sprintf(numstr,"%.8f",dstr(value)), cJSON_AddItemToObject(item,"value",cJSON_CreateString(numstr));
                                if ( (deposittxid= get_deposittxid(ap,txidstr,entries[j].v)) != 0 )
                                {
                                    expand_nxt64bits(depositstr,deposittxid);
                                    cJSON_AddItemToObject(item,"depositid",cJSON_CreateString(depositstr));
                                    cJSON_AddItemToObject(item,"status",cJSON_CreateString("complete"));
                                }
                                else
                                {
                                    if ( value >= (cp->txfee + cp->NXTfee_equiv) * MIN_DEPOSIT_FACTOR )
                                    {
                                        pending_deposits += value;
                                        cJSON_AddItemToObject(item,"status",cJSON_CreateString("pending"));
                                    }
                                    else cJSON_AddItemToObject(item,"status",cJSON_CreateString("too small"));
                                }
                            }
                            cJSON_AddItemToArray(array,item);
                        }
                    }
                }
                free(msig);
            }
        }
        free(msigs);
        cJSON_AddItemToObject(*jsonp,"userdeposits",array);
    }
    if ( ap->num > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<ap->num; i++)
        {
            tp = ap->txids[i];
            if ( tp->redeemtxid != 0 && tp->senderbits == nxt64bits && tp->receiverbits == issuerbits && tp->assetbits == ap->assetbits )
            {
                withdrawaddr[0] = 0;
                if ( tp->comment != 0 && (tp->comment[0] == '{' || tp->comment[0] == '[') )
                {
                    item = cJSON_Parse(tp->comment);
                    copy_cJSON(withdrawaddr,cJSON_GetObjectItem(item,"withdrawaddr"));
                }
                else item = cJSON_CreateObject();
                expand_nxt64bits(redeemstr,tp->redeemtxid), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(redeemstr));
                value = tp->U.assetoshis;
                sprintf(numstr,"%.8f",dstr(value)), cJSON_AddItemToObject(item,"value",cJSON_CreateString(numstr));
                cJSON_AddItemToObject(item,"timestamp",cJSON_CreateNumber(tp->timestamp));
                if ( value < MIN_DEPOSIT_FACTOR*(cp->txfee + cp->NXTfee_equiv) )
                    cJSON_AddItemToObject(item,"status",cJSON_CreateString("too small"));
                else
                {
                    autojson = gen_autoconvert_json(tp);
                    if ( autojson != 0 )
                    {
                        cJSON_AddItemToObject(item,"autoconv",autojson);
                        if ( actionflag > 0 && (tp->convexpiration == 0 || time(NULL) < tp->convexpiration) )
                            if ( (pendingtxid= autoconvert(tp)) != 0 )
                                add_pendingxfer(0,pendingtxid);
                    }
                    if ( tp->AMtxidbits == 0 && is_limbo_redeem(cp,tp->AMtxidbits) == 0 )
                        cJSON_AddItemToObject(item,"status",cJSON_CreateString("queued"));
                    else if ( tp->AMtxidbits == 1 ) cJSON_AddItemToObject(item,"status",cJSON_CreateString("pending"));
                    else if ( is_limbo_redeem(cp,tp->AMtxidbits) != 0 ) cJSON_AddItemToObject(item,"status",cJSON_CreateString("completed"));
                    else if ( tp->AMtxidbits > 1 )
                    {
                        expand_nxt64bits(redeemstr,tp->AMtxidbits), cJSON_AddItemToObject(item,"sentAM",cJSON_CreateString(redeemstr));
                        if ( (withdrew= get_sentAM_cointxid(txidstr,cp,autojson,withdrawaddr,tp->redeemtxid,tp->AMtxidbits)) <= 0 )
                            cJSON_AddItemToObject(item,"status",cJSON_CreateString("error"));
                        else
                        {
                            cJSON_AddItemToObject(item,"cointxid",cJSON_CreateString(txidstr));
                            sprintf(numstr,"%.8f",dstr(withdrew)), cJSON_AddItemToObject(item,"withdrew",cJSON_CreateString(numstr));
                            cJSON_AddItemToObject(item,"status",cJSON_CreateString("completed"));
                        }
                    }
                    else cJSON_AddItemToObject(item,"status",cJSON_CreateString("unexpected"));
                }
                cJSON_AddItemToArray(array,item);
            }
        }
        cJSON_AddItemToObject(*jsonp,"userwithdraws",array);
    }
    assetoshis = get_accountassets(0,ap,NXTaddr);
    balance = assetoshis + pending_deposits + pending_withdraws;
    sprintf(numstr,"%.8f",dstr(assetoshis)), cJSON_AddItemToObject(*jsonp,"AEbalance",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(pending_deposits)), cJSON_AddItemToObject(*jsonp,"pending_userdeposits",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(pending_withdraws)), cJSON_AddItemToObject(*jsonp,"pending_userwithdraws",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(balance)), cJSON_AddItemToObject(*jsonp,"userbalance",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(*jsonp,"userNXT",cJSON_CreateString(NXTaddr));
    conv_rsacctstr(rsacct,nxt64bits);
    cJSON_AddItemToObject(*jsonp,"userRS",cJSON_CreateString(rsacct));
    if ( depositaddr[0] != 0 )
        cJSON_AddItemToObject(*jsonp,"depositaddr",cJSON_CreateString(depositaddr));
    if ( lastbuyNXT != 0 )
    {
        if ( buyNXT != lastbuyNXT )
            cJSON_AddItemToObject(*jsonp,"totalbuyNXT",cJSON_CreateNumber(buyNXT));
        else cJSON_AddItemToObject(*jsonp,"buyNXT",cJSON_CreateNumber(lastbuyNXT));
    }
}

char *check_MGW_cache(struct coin_info *cp,char *userNXTaddr)
{
    char *retstr = 0;
    FILE *fp;
    long fsize;
    cJSON *json;
    char fname[512],*buf;
    uint32_t coinheight,height,timestamp,cacheheight,cachetimestamp;
    coinheight = get_blockheight(cp);
    if ( cp->uptodate >= (coinheight - cp->minconfirms) )
    {
        height = get_NXTheight();
        timestamp = (uint32_t)time(NULL);
        set_MGW_statusfname(fname,userNXTaddr);
        if ( (fp= fopen(fname,"rb")) != 0 )
        {
            fseek(fp,0,SEEK_END);
            fsize = ftell(fp);
            rewind(fp);
            buf = calloc(1,fsize);
            fread(buf,1,fsize,fp);
            fclose(fp);
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                cacheheight = (int32_t)get_cJSON_int(json,"NXTheight");
                cachetimestamp = (int32_t)get_cJSON_int(json,"timestamp");
                printf("uptodate.%u vs %u | cache.(%d t%d) vs (%d t%d)\n",cp->uptodate,(coinheight - cp->minconfirms),cacheheight,cachetimestamp,height,timestamp);
                if ( cacheheight > 0 && cacheheight >= height ) // > can happen on blockchain rescans
                    retstr = buf, buf = 0;
                free_json(json);
            }
            if ( buf != 0 )
                free(buf);
        }
    }
    return(retstr);
}

char *MGW(char *issuerNXT,int32_t rescan,int32_t actionflag,char *coin,char *assetstr,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1,char *exclude2,char *refNXTaddr,char *depositors_pubkey)
{
    static portable_mutex_t mutex;
    static int32_t firsttimestamp,didinit;
    static char **specialNXTaddrs;
    char retbuf[4096],NXTaddr[64],rsacct[64],*ipaddrs[3],*retstr = 0;
    struct coin_info *cp = 0;
    uint64_t pendingtxid,nxt64bits = 0;
    double startmilli = milliseconds();
    int32_t i,createdflag;
    struct NXT_asset *ap = 0;
    cJSON *json = 0;
    retbuf[0] = 0;
    if ( didinit == 0 )
    {
        portable_mutex_init(&mutex);
        didinit = 1;
    }
    if ( MGW_initdone == 0 )
        sprintf(retbuf,"{\"error\":\"MGW not initialized yet\"");
    else
    {
        if ( refNXTaddr != 0 && refNXTaddr[0] == 0 )
        {
            if ( rescan != 0 )
                sprintf(retbuf,"{\"error\":\"need NXT address to rescan\"");
            refNXTaddr = 0;
            nxt64bits = 0;
        }
        else
        {
            nxt64bits = conv_rsacctstr(refNXTaddr,0);
            expand_nxt64bits(NXTaddr,nxt64bits);
        }
        if ( depositors_pubkey != 0 && depositors_pubkey[0] == 0 )
            depositors_pubkey = 0;
        if ( (cp= get_coin_info(coin)) == 0 )
        {
            ap = get_NXTasset(&createdflag,Global_mp,assetstr);
            cp = conv_assetid(assetstr);
            if ( cp == 0 || ap == 0 )
                sprintf(retbuf,"{\"error\":\"dont have coin_info for asset (%s) ap.%p\"",assetstr,ap);
        }
        else
        {
            if ( assetstr != 0 && strcmp(cp->assetid,assetstr) != 0 )
                sprintf(retbuf,"{\"error\":\"mismatched assetid\"");
            ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        }
        if ( firsttimestamp == 0 )
            get_NXTblock(&firsttimestamp);
    }
    if ( retbuf[0] != 0 )
    {
        if ( refNXTaddr != 0 && nxt64bits != 0 )
        {
            conv_rsacctstr(rsacct,nxt64bits);
            sprintf(retbuf+strlen(retbuf),",\"userNXT\":\"%llu\",\"RS\":\"%s\"",(long long)nxt64bits,rsacct);
        }
        sprintf(retbuf+strlen(retbuf),",\"gatewayid\":%d",Global_mp->gatewayid);
        if ( (cp= get_coin_info("BTCD")) != 0 )
            sprintf(retbuf+strlen(retbuf),",\"requestType\":\"MGWresponse\",\"NXT\":\"%s\"}",cp->srvNXTADDR);
        printf("MGWERROR.(%s)\n",retbuf);
        return(clonestr(retbuf));
    }
    if ( issuerNXT == 0 || issuerNXT[0] == 0 )
        issuerNXT = cp->MGWissuer;
    if ( NXT0 != 0 && NXT0[0] != 0 )
    {
        specialNXTaddrs = calloc(16,sizeof(*specialNXTaddrs));
        init_specialNXTaddrs(specialNXTaddrs,ipaddrs,issuerNXT,NXT0,NXT1,NXT2,ip0,ip1,ip2,exclude0,exclude1,exclude2);
    } else specialNXTaddrs = MGW_whitelist;
    if ( nxt64bits != 0 && rescan != 0 )
    {
        if ( (retstr= check_MGW_cache(cp,NXTaddr)) == 0 )
        {
            update_NXTblockchain_info(cp,specialNXTaddrs,NXTaddr);
            json = process_MGW(0,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
            MGW_useracct_str(&json,actionflag,cp,ap,nxt64bits,issuerNXT,specialNXTaddrs);
        }
    }
    else
    {
        //if ( actionflag != 0 || (retstr= check_MGW_cache(cp,0)) == 0 )
        {
            if ( (pendingtxid= update_NXTblockchain_info(cp,specialNXTaddrs,issuerNXT)) == 0 )
            {
                json = process_MGW(actionflag,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
                if (  actionflag == 0 && Global_mp->gatewayid == NUM_GATEWAYS-1 )
                {
                    portable_mutex_lock(&mutex);
                    if ( cp->withdrawinfos[0].rawtx.batchcrc == cp->withdrawinfos[1].rawtx.batchcrc && cp->withdrawinfos[0].rawtx.batchcrc == cp->withdrawinfos[2].rawtx.batchcrc )
                    {
                        printf(">>>>>>>>>>>>>> STARTING AUTO WITHDRAW %u <<<<<<<<<<<<<<<<<<<\n",cp->withdrawinfos[0].rawtx.batchcrc);
                        if ( json != 0 )
                            free_json(json);
                        json = process_MGW(-1,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
                    }
                    else if ( cmp_batch_depositinfo(&cp->withdrawinfos[0],&cp->withdrawinfos[1]) == 0 && cmp_batch_depositinfo(&cp->withdrawinfos[0],&cp->withdrawinfos[2]) == 0 )
                    {
                        printf(">>>>>>>>>>>>>> STARTING AUTO DEPOSIT %.8f <<<<<<<<<<<<<<<<<<<\n",dstr(cp->withdrawinfos[0].pendingdeposits));
                        if ( json != 0 )
                            free_json(json);
                        json = process_MGW(1,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
                    }
                    portable_mutex_unlock(&mutex);
                }
            }
            else retstr = wait_for_pendingtxid(cp,specialNXTaddrs,issuerNXT,pendingtxid);
        }
    }
    if ( json != 0 )
    {
        if ( NXTaddr == 0 || NXTaddr[0] == 0 )
            strcpy(NXTaddr,cp->name);
        if ( (cp= get_coin_info("BTCD")) != 0 )
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(cp->srvNXTADDR));
        cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("MGWresponse"));
        cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(Global_mp->gatewayid));
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
        cJSON_AddItemToObject(json,"NXTheight",cJSON_CreateNumber(get_NXTheight()));
        retstr = cJSON_Print(json);
        save_MGW_status(NXTaddr,retstr);
        //stripwhite_ns(retstr,strlen(retstr));
        free_json(json);
    }
    if ( specialNXTaddrs != MGW_whitelist )
    {
        for (i=0; specialNXTaddrs[i]!=0; i++)
            free(specialNXTaddrs[i]);
        free(specialNXTaddrs);
    }
    if ( retstr == 0 )
        retstr = clonestr("{}");
    printf("MGW.(%s)\n",retstr);
    return(retstr);
}
#endif

