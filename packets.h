//
//  packets.h
//  xcode
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef xcode_packets_h
#define xcode_packets_h

int32_t _encode_str(unsigned char *cipher,char *str,int32_t size,unsigned char *destpubkey,unsigned char *myprivkey)
{
    long len;
    unsigned char buf[4096],*nonce = cipher;
    randombytes(nonce,crypto_box_NONCEBYTES);
    cipher += crypto_box_NONCEBYTES;
    len = size;//strlen(str) + 1;
    //printf("len.%ld -> %ld %ld\n",len,len+crypto_box_ZEROBYTES,len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
    memset(cipher,0,len+crypto_box_ZEROBYTES);
    memset(buf,0,crypto_box_ZEROBYTES);
    memcpy(buf+crypto_box_ZEROBYTES,str,len);
    /*int i;
     for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
     printf("%02x",destpubkey[i]);
     printf(" destpubkey\n");
     for (i=0; i<crypto_box_SECRETKEYBYTES; i++)
     printf("%02x",myprivkey[i]);
     printf(" myprivkey\n");
     for (i=0; i<crypto_box_NONCEBYTES; i++)
     printf("%02x",nonce[i]);
     printf(" nonce\n");*/
    crypto_box(cipher,buf,len+crypto_box_ZEROBYTES,nonce,destpubkey,myprivkey);
    return((int32_t)len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
}

int32_t _decode_cipher(char *str,unsigned char *cipher,int32_t *lenp,unsigned char *srcpubkey,unsigned char *myprivkey)
{
    int i,err,len = *lenp;
    unsigned char *nonce = cipher;
    cipher += crypto_box_NONCEBYTES, len -= crypto_box_NONCEBYTES;
    /*int i;
     for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
     printf("%02x",srcpubkey[i]);
     printf(" destpubkey\n");
     for (i=0; i<crypto_box_SECRETKEYBYTES; i++)
     printf("%02x",myprivkey[i]);
     printf(" myprivkey\n");
     for (i=0; i<crypto_box_NONCEBYTES; i++)
     printf("%02x",nonce[i]);
     printf(" nonce\n");*/
    err = crypto_box_open((unsigned char *)str,cipher,len,nonce,srcpubkey,myprivkey);
    for (i=0; i<len-crypto_box_ZEROBYTES; i++)
        str[i] = str[i+crypto_box_ZEROBYTES];
    *lenp = len - crypto_box_ZEROBYTES;
    return(err);
}

int32_t deonionize(unsigned char *pubkey,unsigned char *decoded,unsigned char *encoded,int32_t len)
{
    //void *origencoded = encoded;
    int32_t err;
    uint64_t packetdest;
    struct coin_info *cp;
    uint16_t payload_len;
    cp = get_coin_info("BTCD");
    memcpy(&packetdest,encoded,sizeof(packetdest));
    if ( packetdest == 0 || ((packetdest == cp->srvpubnxtbits && strcmp(cp->privacyserver,"127.0.0.1") == 0) || packetdest == cp->pubnxtbits) )
    {
        encoded += sizeof(packetdest);
        memcpy(pubkey,encoded,crypto_box_PUBLICKEYBYTES);
        encoded += crypto_box_PUBLICKEYBYTES;
        memcpy(&payload_len,encoded,sizeof(payload_len));
        //printf("deonionize >>>>> pubkey.%llx vs mypubkey.%llx (%llx) -> %d %2x\n",*(long long *)pubkey,*(long long *)Global_mp->session_pubkey,*(long long *)Global_mp->loopback_pubkey,payload_len,payload_len);
        encoded += sizeof(payload_len);
        printf("packedest.%llu srvpub.%llu (%s) payload_len.%d\n",(long long)packetdest,(long long)cp->srvpubnxtbits,cp->privacyserver,payload_len);
        if ( (payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(packetdest)) <= len )
        {
            len = payload_len;
            if ( packetdest == 0 || packetdest == cp->srvpubnxtbits  )
            {
                err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->loopback_privkey);
                if ( err == 0 )
                {
                    //printf("srvpubnxtbits payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                    return(len);
                }
            }
            else
            {
                err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->session_privkey);
                if ( err == 0 )
                {
                    //printf("payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                    return(len);
                }
            }
        } else printf("mismatched len expected %ld got %d\n",(payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(packetdest)),len);
    }
    else printf("deonionize onion for NXT.%llu not this address.(%llu)\n",(long long)packetdest,(long long)cp->srvpubnxtbits);
    return(0);
}

int32_t direct_onionize(uint64_t nxt64bits,unsigned char *destpubkey,unsigned char *maxbuf,unsigned char *encoded,unsigned char **payloadp,int32_t len)
{
    unsigned char *origencoded,onetime_pubkey[crypto_box_PUBLICKEYBYTES],onetime_privkey[crypto_box_SECRETKEYBYTES],*payload = (*payloadp);
    uint16_t *payload_lenp,*max_lenp,slen,slen2;
    int32_t padlen,onlymax = 0;
    long hdrlen;
    memset(maxbuf,0,MAX_UDPLEN);
    origencoded = encoded;
    padlen = MAX_UDPLEN - 56 - (len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES) - (sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits)) - sizeof(uint32_t);
    if ( padlen < 0 )
        padlen = 0;
    if ( encoded == 0 )
    {
        encoded = maxbuf;
        onlymax = 1;
    }
    (*payloadp) = encoded;
    crypto_box_keypair(onetime_pubkey,onetime_privkey);
    memcpy(encoded,&nxt64bits,sizeof(nxt64bits));
    encoded += sizeof(nxt64bits);
    memcpy(encoded,onetime_pubkey,sizeof(onetime_pubkey));
    encoded += sizeof(onetime_pubkey);
    payload_lenp = (uint16_t *)encoded;
    if ( onlymax == 0 )
    {
        hdrlen = (long)(encoded - origencoded);
        memcpy(maxbuf,origencoded,hdrlen);
        maxbuf += hdrlen;
        max_lenp = (uint16_t *)maxbuf;
        maxbuf += sizeof(*payload_lenp);
    } else max_lenp = payload_lenp;
    encoded += sizeof(*payload_lenp);
    if ( 0 )
    {
        char hexstr[1024];
        init_hexbytes(hexstr,destpubkey,crypto_box_PUBLICKEYBYTES);
        hexstr[16] = 0;
        printf("DIRECT ONIONIZE: pubkey.%s encode len.%d padlen.%d -> dest.%llu ",hexstr,len,padlen,(long long)nxt64bits);
    }
    if ( onlymax != 0 )
    {
        len = _encode_str(encoded,(char *)payload,len+padlen,destpubkey,onetime_privkey);
        slen2 = slen = len;
        memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
    }
    else
    {
        len = _encode_str(encoded,(char *)payload,len,destpubkey,onetime_privkey);
        slen = len;
        memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
        if ( padlen > 0 )
        {
            //slen2 = len + padlen;
            slen2 = _encode_str(maxbuf,(char *)payload,len + padlen,destpubkey,onetime_privkey);
            memcpy(max_lenp,&slen2,sizeof(*max_lenp));
        }
        else memcpy(maxbuf,encoded,len);
    }
    printf("new len.%d + %ld = %ld (%d %d)\n",len,sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits),sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits)+len,*payload_lenp,*max_lenp);
    return(len + sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits));
}

int32_t onionize(char *hopNXTaddr,unsigned char *maxbuf,unsigned char *encoded,char *destNXTaddr,unsigned char **payloadp,int32_t len)
{
    uint64_t nxt64bits;
    int32_t createdflag;
    struct NXT_acct *np;
    strcpy(hopNXTaddr,destNXTaddr);
    nxt64bits = calc_nxt64bits(destNXTaddr);
    np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    return(direct_onionize(nxt64bits,np->mypeerinfo.srv.pubkey,maxbuf,encoded,payloadp,len));
}

int32_t pserver_canhop(struct pserver_info *pserver,char *hopNXTaddr)
{
    int32_t createdflag,i = -1;
    uint32_t *hasips;
    struct NXT_acct *np;
    struct peerinfo *peer;
    np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
    peer = &np->mypeerinfo;
    if ( is_privacyServer(peer) != 0 && pserver != 0 && pserver->numips > 0 && (hasips= pserver->hasips) != 0 )
    {
        for (i=0; i<pserver->numips; i++)
            if ( hasips[i] == peer->srv.ipbits )
            {
                char ipaddr[16];
                expand_ipbits(ipaddr,hasips[i]);
                printf(">>>>>>>>>>> HASIP.%s in slot %d of %d\n",ipaddr,i,pserver->numips);
                return(i);
            }
    }
    return(i);
}

int32_t add_random_onionlayers(char *hopNXTaddr,int32_t numlayers,uint8_t *maxbuf,uint8_t *final,uint8_t **srcp,int32_t len)
{
    char destNXTaddr[64],ipaddr[64];
    uint8_t dest[4096],srcbuf[4096],*src = srcbuf;
    struct peerinfo *peer;
    struct pserver_info *pserver;
    struct NXT_acct *np;
    if ( numlayers > 1 )
        numlayers = ((rand() >> 8) % numlayers);
    if ( numlayers > 0 )
    {
        printf("add_random_onionlayers %d of %d\n",numlayers,Global_mp->Lfactor);
        memset(dest,0,sizeof(dest));
        memcpy(srcbuf,*srcp,len);
        while ( numlayers > 0 )
        {
            peer = get_random_pserver();
            if ( peer == 0 )
            {
                printf("FATAL: cant get random peer!\n");
                return(-1);
            }
            expand_ipbits(ipaddr,peer->srv.ipbits);
            if ( (pserver= get_pserver(0,ipaddr,0,0)) == 0 || pserver_canhop(pserver,hopNXTaddr) < 0 )
                continue;
            np = search_addresses(peer->pubBTCD);
            if ( np == 0 && peer->pubnxtbits != 0 )
            {
                expand_nxt64bits(destNXTaddr,peer->pubnxtbits);
                np = search_addresses(destNXTaddr);
            }
            if ( np == 0 )
                np = search_addresses(peer->pubBTC);
            if ( np == 0 )
            {
                printf("FATAL: %s %s %s is unknown account\n",peer->pubBTCD,destNXTaddr,peer->pubBTC);
                return(-1);
            }
            if ( strcmp(hopNXTaddr,np->H.U.NXTaddr) != 0 )
            {
                //printf("add layer %d: NXT.%s\n",numlayers,np->H.U.NXTaddr);
                len = onionize(hopNXTaddr,maxbuf,final,np->H.U.NXTaddr,&src,len);
                memcpy(srcbuf,final,len);
                src = srcbuf;
                *srcp = final;
                if ( len > 4096 )
                {
                    printf("FATAL: onion layers too big.%d\n",len);
                    return(-1);
                }
            }
            numlayers--;
        }
        src = dest;
    }
    return(len);
}

struct NXT_acct *process_packet(char *retjsonstr,unsigned char *recvbuf,int32_t recvlen,uv_udp_t *udp,struct sockaddr *prevaddr,char *sender,uint16_t port)
{
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t destbits = 0;
    struct NXT_acct *tokenized_np = 0;
    int32_t valid,len=0,len2,createdflag,datalen,parmslen,encrypted = 1;
    cJSON *argjson,*tmpjson,*valueobj;
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES],pubkey2[crypto_box_PUBLICKEYBYTES],decoded[4096],decoded2[4096],tmpbuf[4096],maxbuf[4096],*outbuf;
    char senderNXTaddr[64],datastr[4096],hopNXTaddr[64],checkstr[MAX_JSON_FIELD],datalenstr[MAX_JSON_FIELD];
    char *parmstxt=0,*jsonstr;
    memset(decoded,0,sizeof(decoded));
    memset(tmpbuf,0,sizeof(tmpbuf));
    retjsonstr[0] = 0;
    //sprintf(retjsonstr,"{\"error\":\"unknown error processing %d bytes from %s/%d\"}",recvlen,sender,port);
    if ( is_encrypted_packet(recvbuf,recvlen) != 0 )
    {
        recvbuf += sizeof(uint32_t);
        recvlen -= sizeof(uint32_t);
        if ( (len= deonionize(pubkey,decoded,recvbuf,recvlen)) > 0 )
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            if ( cp != 0 && (destbits == 0 || destbits == cp->pubnxtbits || destbits == cp->srvpubnxtbits) )
            {
                memset(decoded2,0,sizeof(decoded2));
                if ( (len2= deonionize(pubkey2,decoded2,decoded,len)) > 0 )
                {
                    memcpy(&destbits,decoded2,sizeof(destbits));
                    printf("decrypted2 len2.%d dest2.(%llu)\n",len2,(long long)destbits);
                    len = len2;
                    memcpy(decoded,decoded2,len);
                    memcpy(pubkey,pubkey2,sizeof(pubkey));
                } else printf("couldnt decrypt2 packet len.%d\n",len);
            }
        }
        else
        {
            printf("couldnt decrypt packet len.%d\n",recvlen);
            return(0);
        }
    }
    else
    {
        printf("process_packet got nonencrypted len.%d %s/%d (%s)\n",recvlen,sender,port,recvbuf);
        len = recvlen;
        memcpy(decoded,recvbuf,recvlen);
        encrypted = 0;
        //return(0);
    }
    if ( len > 0 )
    {
        //decoded[len] = 0;
        parmslen = (int32_t)strlen((char *)decoded) + 1;
        if ( len > parmslen )
            datalen = (len - parmslen);
        else datalen = 0;
        printf("len.%d parmslen.%d datalen.%d\n",len,parmslen,datalen);
        parmstxt = clonestr((char *)decoded);
        argjson = cJSON_Parse(parmstxt);
        //printf("[%s] argjson.%p udp.%p\n",parmstxt,argjson,udp);
        free(parmstxt), parmstxt = 0;
        if ( argjson != 0 ) // if it parses, we must have been the ultimate destination
        {
            senderNXTaddr[0] = 0;
            memset(pubkey,0,sizeof(pubkey));
            parmstxt = verify_tokenized_json(pubkey,senderNXTaddr,&valid,argjson);
            if ( valid > 0 && parmstxt != 0 && parmstxt[0] != 0 )
            {
                tmpjson = cJSON_Parse(parmstxt);
                if ( tmpjson != 0 )
                {
                    if ( encrypted == 0 )
                        copy_cJSON(checkstr,cJSON_GetObjectItem(tmpjson,"requestType"));
                    else
                    {
                        valueobj = cJSON_GetObjectItem(tmpjson,"data");
                        if ( is_cJSON_Number(valueobj) != 0 )
                        {
                            copy_cJSON(datalenstr,valueobj);
                            if ( datalen > 0 && datalen == atoi(datalenstr) )
                            {
                                init_hexbytes(datastr,decoded + parmslen,datalen);
                                cJSON_ReplaceItemInObject(argjson,"data",cJSON_CreateString(datastr));
                            }
                            else printf("datalen.%d mismatch.(%s) -> %d\n",datalen,datalenstr,atoi(datalenstr));
                        }
                        strcpy(checkstr,"ping");
                    }
                    free_json(tmpjson);
                }
                if ( strcmp(checkstr,"ping") == 0 )
                {
                    char *pNXT_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *argjson,char *sender,int32_t valid,char *origargstr);
                    tokenized_np = get_NXTacct(&createdflag,Global_mp,senderNXTaddr);
                    update_routing_probs(tokenized_np->H.U.NXTaddr,1,udp == 0,&tokenized_np->mypeerinfo,sender,port,pubkey);
                    //printf("GOT.(%s)\n",parmstxt);
                    jsonstr = pNXT_json_commands(Global_mp,prevaddr,argjson,tokenized_np->H.U.NXTaddr,valid,(char *)decoded);
                    if ( jsonstr != 0 )
                    {
                        //printf("should send tokenized.(%s) to %s\n",jsonstr,tokenized_np->H.U.NXTaddr);
                        /*if ( (retstr= send_tokenized_cmd(hopNXTaddr,Global_mp->Lfactor,srvNXTaddr,cp->srvNXTACCTSECRET,retjsonstr,tokenized_np->H.U.NXTaddr)) != 0 )
                         {
                         printf("sent back via UDP.(%s)\n",retstr);
                         free(retstr);
                         }*/
                        free(jsonstr);
                    }
                }
                else printf("encrypted.%d: checkstr.(%s) for (%s)\n",encrypted,checkstr,parmstxt);
            }
            else printf("valid.%d | unexpected non-tokenized message.(%s)\n",valid,decoded);
            free_json(argjson);
            if ( parmstxt != 0 )
                free(parmstxt);
            return(tokenized_np);
        }
        else
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            if ( destbits != 0 ) // route packet
            {
                expand_nxt64bits(hopNXTaddr,destbits);
                printf("Route to {%s}\n",hopNXTaddr);
                outbuf = decoded;
                len = onionize(hopNXTaddr,maxbuf,tmpbuf,hopNXTaddr,&outbuf,len);
                //route_packet(1,0,hopNXTaddr,outbuf,len);//decoded,len);
                route_packet(1,0,hopNXTaddr,maxbuf,MAX_UDPLEN-sizeof(uint32_t));
                return(0);
            }
        }
    }
    else printf("process_packet got unexpected recvlen.%d %s/%d\n",recvlen,sender,port);
    return(0);
}

int32_t has_privacyServer(struct NXT_acct *np)
{
    if ( np->mypeerinfo.srv.ipbits != 0 && np->mypeerinfo.pubnxtbits != np->mypeerinfo.srvnxtbits )
        return(1);
    else return(0);
}

char *sendmessage(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr)
{
    uint64_t txid;
    char buf[4096],destsrvNXTaddr[64],srvNXTaddr[64];
    unsigned char maxbuf[4096],encodedsrvD[4096],encodedD[4096],encodedL[4096],encodedP[4096],*outbuf;
    int32_t len,createdflag;//,maxlen;
    struct NXT_acct *np,*destnp;
    np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
    expand_nxt64bits(srvNXTaddr,np->mypeerinfo.srvnxtbits);
    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    if ( np == 0 || destnp == 0 || Global_mp->udp == 0 || destnp->mypeerinfo.srvnxtbits == 0 )
    {
        sprintf(buf,"\"error\":\"no np.%p or global udp.%p for sendmessage || %s destnp->mypeerinfo.srvnxtbits %llu == 0\"}",np,Global_mp->udp,destNXTaddr,(long long)destnp->mypeerinfo.srvnxtbits);
        return(clonestr(buf));
    }
    expand_nxt64bits(destsrvNXTaddr,destnp->mypeerinfo.srvnxtbits);
    memset(maxbuf,0,sizeof(maxbuf)); // always the same size
    memset(encodedD,0,sizeof(encodedD)); // encoded to dest
    memset(encodedsrvD,0,sizeof(encodedsrvD)); // encoded to privacyServer of dest
    memset(encodedL,0,sizeof(encodedL)); // encoded to max L onion layers
    memset(encodedP,0,sizeof(encodedP)); // encoded to privacyserver
    outbuf = (unsigned char *)msg;
    len = onionize(hopNXTaddr,maxbuf,encodedD,destNXTaddr,&outbuf,msglen);
    printf("\nsendmessage (%s) len.%d to %s crc.%x\n",msg,msglen,destNXTaddr,_crc32(0,outbuf,len));
    if ( len > sizeof(encodedP)-1024 )
    {
        printf("sendmessage, payload too big %d\n",len);
        sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s too long.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    }
    else if ( len > 0 )
    {
        //printf("np.%p NXT.%s | destnp.%p\n",np,np!=0?np->H.U.NXTaddr:"no np",destnp);
        if ( strcmp(destsrvNXTaddr,destNXTaddr) != 0 && has_privacyServer(destnp) != 0 ) // build onion in reverse order, privacyServer for dest is 2nd
            len = onionize(hopNXTaddr,maxbuf,encodedsrvD,destsrvNXTaddr,&outbuf,len);
        if ( L > 0 )
            len = add_random_onionlayers(hopNXTaddr,L,maxbuf,encodedL,&outbuf,len);
        if ( strcmp(srvNXTaddr,hopNXTaddr) != 0 && has_privacyServer(np) != 0 ) // send via privacy server to protect our IP
            len = onionize(hopNXTaddr,maxbuf,0,srvNXTaddr,&outbuf,len);
        txid = route_packet(1,0,hopNXTaddr,outbuf,len);
        //txid = route_packet(1,0,hopNXTaddr,maxbuf,MAX_UDPLEN - sizeof(uint32_t));
        if ( txid == 0 )
        {
            sprintf(buf,"{\"error\":\"%s cant send via p2p sendmessage.(%s) to %s\"}",verifiedNXTaddr,msg,destNXTaddr);
        }
        else
        {
            sprintf(buf,"{\"status\":\"%s sends via p2p encrypted sendmessage to %s pending\"}",verifiedNXTaddr,destNXTaddr);
        }
    }
    else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s illegal len\"}",verifiedNXTaddr,msg,destNXTaddr);
    return(clonestr(buf));
}

char *send_tokenized_cmd(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr)
{
    char _tokbuf[4096];
    int32_t n;
    if ( strcmp("{\"result\":null}",cmdstr) == 0 )
    {
        printf("no need to send null JSON to %s\n",destNXTaddr);
        return(0);
    }
    n = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    {
        char sender[64],*parmstxt;
        int32_t valid;
        cJSON *json = cJSON_Parse(_tokbuf);
        if ( json != 0 )
        {
            parmstxt = verify_tokenized_json(0,sender,&valid,json);
            if ( valid <= 0 )
                printf("_tokbuf.%s valid.%d sender.(%s)\n",_tokbuf,valid,sender);
            if ( parmstxt != 0 )
                free(parmstxt);
            free_json(json);
        }
    }
    return(sendmessage(hopNXTaddr,L,verifiedNXTaddr,_tokbuf,(int32_t)n+1,destNXTaddr));
}

int32_t sendandfree_jsoncmd(int32_t L,char *sender,char *NXTACCTSECRET,cJSON *json,char *destNXTaddr)
{
    int32_t err = -1;
    cJSON *retjson;
    struct NXT_acct *np;
    char *msg,*retstr,errstr[512],hopNXTaddr[64],verifiedNXTaddr[64];
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    msg = cJSON_Print(json);
    stripwhite_ns(msg,strlen(msg));
    retstr = send_tokenized_cmd(hopNXTaddr,L,verifiedNXTaddr,NXTACCTSECRET,msg,destNXTaddr);
    if ( retstr != 0 )
    {
        // printf("sendandfree_jsoncmd.(%s)\n",retstr);
        retjson = cJSON_Parse(retstr);
        if ( retjson != 0 )
        {
            if ( extract_cJSON_str(errstr,sizeof(errstr),retjson,"error") > 0 )
            {
                printf("error.(%s) sending (%s)\n",errstr,msg);
                err = -2;
            } else err = 0;
            free_json(retjson);
        } else printf("sendandfree_jsoncmd: cant parse (%s) from (%s)\n",retstr,msg);
        free(retstr);
    } else printf("sendandfree_jsoncmd: no retstr from (%s)\n",msg);
    free_json(json);
    free(msg);
    return(err);
}

char *onion_sendfile(int32_t L,struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *dest,FILE *fp)
{
    return(0);
}
#endif
