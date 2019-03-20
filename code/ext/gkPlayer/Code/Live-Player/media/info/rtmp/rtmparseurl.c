#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <ctype.h>

#include "rtmp_sys.h"
#include "TTLog.h"
#include "rtmphashtable.h"

int RTMP_ParseURL(const char *url, int *protocol, AVal *host, unsigned int *port,
	AVal *playpath, AVal *app)
{
	char *p, *end, *col, *ques, *slash;

	LOGI("Parsing...");

	*protocol = RTMP_PROTOCOL_RTMP;
	*port = 0;
	playpath->av_len = 0;
	playpath->av_val = NULL;
	app->av_len = 0;
	app->av_val = NULL;

	/* Old School Parsing */

	/* look for usual :// pattern */
	p = strstr(url, "://");
	if(!p) {
		LOGE("RTMP URL: No :// in url!");
		return FALSE;
	}
	{
	int len = (int)(p-url);

	if(len == 4 && strncasecmp(url, "rtmp", 4)==0)
		*protocol = RTMP_PROTOCOL_RTMP;
	else if(len == 5 && strncasecmp(url, "rtmpt", 5)==0)
		*protocol = RTMP_PROTOCOL_RTMPT;
	else if(len == 5 && strncasecmp(url, "rtmps", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMPS;
	else if(len == 5 && strncasecmp(url, "rtmpe", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMPE;
	else if(len == 5 && strncasecmp(url, "rtmfp", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMFP;
	else if(len == 6 && strncasecmp(url, "rtmpte", 6)==0)
	        *protocol = RTMP_PROTOCOL_RTMPTE;
	else if(len == 6 && strncasecmp(url, "rtmpts", 6)==0)
	        *protocol = RTMP_PROTOCOL_RTMPTS;
	else {
		LOGI( "Unknown protocol!\n");
		goto parsehost;
	}
	}

	LOGI("Parsed protocol: %d", *protocol);

parsehost:
	/* let's get the hostname */
	p+=3;

	/* check for sudden death */
	if(*p==0) {
		LOGI("No hostname in URL!");
		return FALSE;
	}

	end   = p + strlen(p);
	col   = strchr(p, ':');
	ques  = strchr(p, '?');
	slash = strchr(p, '/');

	{
	int hostlen;
	if(slash)
		hostlen = slash - p;
	else
		hostlen = end - p;
	if(col && col -p < hostlen)
		hostlen = col - p;

	if(hostlen < 256) {
		host->av_val = p;
		host->av_len = hostlen;
		LOGI("Parsed host    : %.*s", hostlen, host->av_val);
	} else {
		LOGI("Hostname exceeds 255 characters!");
	}

	p+=hostlen;
	}

	/* get the port number if available */
	if(*p == ':') {
		unsigned int p2;
		p++;
		p2 = atoi(p);
		if(p2 > 65535) {
			LOGI("Invalid port number!");
		} else {
			*port = p2;
		}
	}

	if(!slash) {
		LOGI("No application or playpath in URL!");
		return TRUE;
	}
	p = slash+1;

	{
	/* parse application
	 *
	 * rtmp://host[:port]/app[/appinstance][/...]
	 * application = app[/appinstance]
	 */

	char *slash2, *slash3 = NULL;
	int applen, appnamelen;

	slash2 = strchr(p, '/');
	if(slash2)
		slash3 = strchr(slash2+1, '/');

	applen = end-p; /* ondemand, pass all parameters as app */
	appnamelen = applen; /* ondemand length */

	if(ques && strstr(p, "slist=")) { /* whatever it is, the '?' and slist= means we need to use everything as app and parse plapath from slist= */
		appnamelen = ques-p;
	}
	else if(strncmp(p, "ondemand/", 9)==0) {
                /* app = ondemand/foobar, only pass app=ondemand */
                applen = 8;
                appnamelen = 8;
        }
	else { /* app!=ondemand, so app is app[/appinstance] */
		if(slash3)
			appnamelen = slash3-p;
		else if(slash2)
			appnamelen = slash2-p;

		applen = appnamelen;
	}

	app->av_val = p;
	app->av_len = applen;
	LOGI("Parsed app     : %.*s", applen, p);

	p += appnamelen;
	}

	if (*p == '/')
		p++;

	if (end-p) {
		AVal av = {p, end-p};
		RTMP_ParsePlaypath(&av, playpath);
	}

	return TRUE;
}

/*
 * Extracts playpath from RTMP URL. playpath is the file part of the
 * URL, i.e. the part that comes after rtmp://host:port/app/
 *
 * Returns the stream name in a format understood by FMS. The name is
 * the playpath part of the URL with formatting depending on the stream
 * type:
 *
 * mp4 streams: prepend "mp4:", remove extension
 * mp3 streams: prepend "mp3:", remove extension
 * flv streams: remove extension
 */
void RTMP_ParsePlaypath(AVal *in, AVal *out) {
	int addMP4 = 0;
	int addMP3 = 0;
	int subExt = 0;
	const char *playpath = in->av_val;
	const char *temp, *q, *ext = NULL;
	const char *ppstart = playpath;
	char *streamname, *destptr, *p;

	int pplen = in->av_len;

	out->av_val = NULL;
	out->av_len = 0;

	if ((*ppstart == '?') &&
	    (temp=strstr(ppstart, "slist=")) != 0) {
		ppstart = temp+6;
		pplen = strlen(ppstart);

		temp = strchr(ppstart, '&');
		if (temp) {
			pplen = temp-ppstart;
		}
	}

	q = strchr(ppstart, '?');
	if (pplen >= 4) {
		if (q)
			ext = q-4;
		else
			ext = &ppstart[pplen-4];
		if ((strncmp(ext, ".f4v", 4) == 0) ||
		    (strncmp(ext, ".mp4", 4) == 0)) {
			addMP4 = 1;
			subExt = 1;
		/* Only remove .flv from rtmp URL, not slist params */
		} else if ((ppstart == playpath) &&
		    (strncmp(ext, ".flv", 4) == 0)) {
			subExt = 1;
		} else if (strncmp(ext, ".mp3", 4) == 0) {
			addMP3 = 1;
			subExt = 1;
		}
	}

	streamname = (char *)malloc((pplen+4+1)*sizeof(char));
	if (!streamname)
		return;

	destptr = streamname;
	if (addMP4) {
		if (strncmp(ppstart, "mp4:", 4)) {
			strcpy(destptr, "mp4:");
			destptr += 4;
		} else {
			subExt = 0;
		}
	} else if (addMP3) {
		if (strncmp(ppstart, "mp3:", 4)) {
			strcpy(destptr, "mp3:");
			destptr += 4;
		} else {
			subExt = 0;
		}
	}

 	for (p=(char *)ppstart; pplen >0;) {
		/* skip extension */
		if (subExt && p == ext) {
			p += 4;
			pplen -= 4;
			continue;
		}
		if (*p == '%') {
			unsigned int c;
			sscanf(p+1, "%02x", &c);
			*destptr++ = c;
			pplen -= 3;
			p += 3;
		} else {
			*destptr++ = *p++;
			pplen--;
		}
	}
	*destptr = '\0';

	out->av_val = streamname;
	out->av_len = destptr - streamname;
}

//rtmp hash table
void  init_hlist_node(struct hlist_node *node)
{
    node->next  = NULL;
    node->pprev = NULL;
    node->key = 0;
    node->value = NULL;
}

void hlist_add_head(struct hlist_node *n,struct hlist_head *h)
{
    struct hlist_node *first = h->first;
    n->next= first;
    
    if (first)
        first->pprev= &n->next;
    
    h->first = n;
    n->pprev = &h->first;
}

void __hlist_del(struct hlist_node *n)
{
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;
    *pprev = next;
    if (next)
        next->pprev = pprev;
}

void hlist_del(struct hlist_node *n)
{
    __hlist_del(n);
    free(n);
}

int CalcIndexValue(int key)
{
    int index= ((key & 0x0000FFFF)^(key>>16))% HASH_HEAD_SIZE;
    if (index < 0)
        index = -index;
    return index;
}

void* getValueByKey(hash_head_t* hashtable, int key)
{
    int index = CalcIndexValue(key);
    struct hlist_node* pNode = hashtable->head[index].first;
    hashtable->findStatus = 0;
    if(!pNode)
        return NULL;
    
    while(pNode)
    {
        if(pNode->key == key){
            hashtable->findStatus = 1;
            return pNode->value;
        }
        else
            pNode = pNode->next;
    }
    return NULL;
}

void putValueByKey(hash_head_t* hashtable, int key, void* value)
{
    int index = CalcIndexValue(key);
    struct hlist_node* pnode = (struct hlist_node*)malloc(sizeof(struct hlist_node));
    init_hlist_node(pnode);
    pnode->key = key;
    pnode->value = value;
    hlist_add_head(pnode, &(hashtable->head[index]));
}

void deleteValueNodeByKey(hash_head_t* hashtable, int key)
{
    int index = CalcIndexValue(key);
    struct hlist_node* pNode = hashtable->head[index].first;
    if (!pNode)
        return;
    
    while(pNode)
    {
        if(pNode->key == key)
            break;
        else
            pNode = pNode->next;
    }
    
    if(pNode)
        hlist_del(pNode);
}

void destroyHashTable(hash_head_t* hashtable)
{
	int i;
    struct hlist_node* pTemp = NULL;
    struct hlist_node* pNode = NULL;
    for(i = 0;i <HASH_HEAD_SIZE; i++)
    {
        pNode = hashtable->head[i].first;
        while (pNode)
        {
            pTemp = pNode;
            pNode = pNode->next;
            if (hashtable->valueType == HASH_VALUETYPE_POINTER) {
                if (pTemp->value)
                    free(pTemp->value);
            }
            hlist_del(pTemp);
        }
        hashtable->head[i].first = NULL;
    }
}

void setHashValueType(hash_head_t* hashtable, int type)
{
    hashtable->valueType = type;
}

void  updateValueByKey(hash_head_t* hashtable, int key, void* value)
{
    int index = CalcIndexValue(key);
    struct hlist_node* pNode = hashtable->head[index].first;
    if (!pNode)
        return;
    
    while(pNode)
    {
        if(pNode->key == key)
            break;
        else
            pNode = pNode->next;
    }
    if(pNode)
        pNode->value = value;
}

