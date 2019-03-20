
#ifndef __RTMP_HASHTABLE_H__
#define __RTMP_HASHTABLE_H__

#define HASH_HEAD_SIZE 100
#define HASH_VALUETYPE_POINTER 0
#define HASH_VALUETYPE_INT     1

#ifdef __cplusplus
extern "C" {
#endif

    struct hlist_node
    {
        struct hlist_node  *next;
        struct hlist_node  **pprev;
        int	   key;
        void*  value;
    };
    
    struct hlist_head
    {
        struct hlist_node *first;
    };
    
    typedef struct _hash_head
    {
        struct hlist_head head[HASH_HEAD_SIZE];
        int    valueType;
        int    findStatus; //1: ok, 0:fail
    }hash_head_t;

    void init_hlist_node(struct hlist_node *node);
    void hlist_add_head(struct hlist_node *n,struct hlist_head *h) ;
    void _hlist_del(struct hlist_node *n);
    void hlist_del(struct hlist_node *n);
    int  CalcIndexValue(int key);
    void* getValueByKey(hash_head_t* hashtable, int key);
    void  putValueByKey(hash_head_t* hashtable, int key, void* value);
    void  updateValueByKey(hash_head_t* hashtable, int key, void* value);
    void deleteValueNodeByKey(hash_head_t* hashtable, int key);
    void destroyHashTable(hash_head_t* hashtable);
    void setHashValueType(hash_head_t* hashtable, int type);

#ifdef __cplusplus
}
#endif

#endif
