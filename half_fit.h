#ifndef HALF_FIT_H_
#define HALF_FIT_H_
#include "type.h"

#define smlst_blk                       5 
#define smlst_blk_sz  ( 1 << smlst_blk ) 
#define lrgst_blk                       15 
#define lrgst_blk_sz    ( 1 << lrgst_blk ) 
#define MAX_BYTES 32768
#define MAX_BLOCKS 1024
#define NUM_BUCKETS 11
#define replace0(size) ((size) ? size : 1024)
#define LOG2_DOWN(x) (32U - __clz(x) - 1)
#define LOG2_UP(x) (((x) - (1 << LOG2_DOWN(x))) ? LOG2_DOWN(x) + 1 : LOG2_DOWN(x))
#define START_INDEX 0x10000000
#define CHECK_BIT(i) ((bins.avail >> i) & 1)
#define GET_HEADER_BLOCK(header) ((Block_Header*)((char*)(bucket) - 4))
#define GET_BUCKET_NODE(header)((Bucket_Node*)((char*)(header) + 4))

void  half_init( void );
void *half_alloc( unsigned int );
void  half_free( void * );
void  set_vector_bit(U8);
void  reset_vector_bit(U8);

typedef struct Block_Header{
	U32 prev:10;
	U32 next:10;
	U32 size:10;
	U32 is_alloc:1;
	U32 next_null:1;
}Block_Header;

typedef struct Bit_Vector{
		U16 avail:11;
}Bit_Vector;

typedef struct Bucket_Node{
	void * next;
	void * prev;
}Bucket_Node;

void  load_bucket_block(Bucket_Node*, U8);

#endif

