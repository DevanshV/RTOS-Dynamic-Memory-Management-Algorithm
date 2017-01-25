#include "half_fit.h"
#include <lpc17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "uart.h"

/***********************************************************
	Initializing Global Vars and Reference Tables
***********************************************************/
unsigned char array[MAX_BYTES]__attribute__((section(".ARM.__at_0x10000000"), zero_init));
Bucket_Node buckets [NUM_BUCKETS];
BOOL avail [NUM_BUCKETS];
Bit_Vector bins;
U16 main_mem = 1024;

U16 set_values [11] = {
	0x001, 0x002, 0x004, 0x008, 0x010, 0x020, 0x040, 0x080, 0x100, 0x200, 0x400
};
U16 reset_values [11] = {
	0x7FE, 0x7FD, 0x7FB, 0x7F7, 0x7EF, 0x7DF, 0x7BF, 0x77F, 0x6FF, 0x5FF, 0x3FF
};

U16 bucket_index [NUM_BUCKETS] = {
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024
};


void  half_init(void){
	
	/***********************************************************
		Initialize Bucket Data Structure
	***********************************************************/
	
	Block_Header * initial_block;
	Bucket_Node  * initial_bucket_node;
	U8 i;
	
	for (i = 0; i < NUM_BUCKETS; i++){
		buckets [i].next = NULL;
		buckets [i].prev = NULL;
	}
	
	/***********************************************************
		Create Initial MemHeader
	***********************************************************/
	
	initial_block = (Block_Header*)(START_INDEX);
	
	initial_block -> prev = NULL;
	initial_block -> next = NULL;
	initial_block -> size = MAX_BLOCKS;
	initial_block -> is_alloc = 0;
	initial_block -> next_null = 1;

	/***********************************************************
		Populate new bucket
	***********************************************************/
	
	buckets [NUM_BUCKETS - 1].next = (Bucket_Node*)((char*)(initial_block) + 4);
	initial_bucket_node = (Bucket_Node*)((char*)(initial_block) + 4);
	initial_bucket_node -> next = NULL;
	initial_bucket_node -> prev = &(buckets [NUM_BUCKETS - 1]);
	bins.avail = 0x400;	
}
	
void *half_alloc(unsigned int size){
	
	/***********************************************************
	Initializing Locals
	***********************************************************/
	
	U32 num_blocks = (U32)((size + 35)>>5); //35 = 4 + 32 - 1
	U32 num_frag_blocks;
	Block_Header* free_header;
	Block_Header* old_next_header;
	Bucket_Node * free_bucket_node;
	Block_Header* existing_header;
	Block_Header* list_root = NULL;
	Bucket_Node* existing_bucket_node = NULL;
	Bucket_Node* existing_bucket_left;
	Bucket_Node* existing_bucket_right;
	Bucket_Node* free_bucket_right;
	U8 index;
	U8 i;
	
	/***********************************************************
		Greater Than Max Value Case
	***********************************************************/
	
	if (num_blocks > 1024)
		return NULL;
	
	/***********************************************************
	Removing Free Block From Bucket List
	***********************************************************/
	
	for (i = LOG2_UP(num_blocks); i < NUM_BUCKETS && list_root == NULL; i++){
		if (bucket_index [i] >= num_blocks && CHECK_BIT(i)){ //avail[i]
			list_root = (Block_Header*)((char*)(buckets [i].next) - 4);
			existing_bucket_node = (Bucket_Node*)(buckets [i].next) ;
			existing_bucket_left = (Bucket_Node*)(existing_bucket_node -> prev); //
			existing_bucket_right = (Bucket_Node*)(existing_bucket_node -> next);
			
			existing_bucket_left -> next = existing_bucket_node -> next;
			
			if (existing_bucket_right == NULL)
				reset_vector_bit(i);
			else
				existing_bucket_right -> prev = existing_bucket_node -> prev;
			break;
		}
	}

	/***********************************************************
	Checking that Space is Available
	***********************************************************/
	
	if (list_root == NULL)
		return NULL;

	/***********************************************************
	Creating New Header if Fragment is Created
	***********************************************************/
	
	existing_header = list_root;
	free_header = (Block_Header*)((char*)(existing_header) + (num_blocks << 5));
	
	num_frag_blocks = replace0(existing_header -> size ) - num_blocks;
	
	if (num_frag_blocks != 0) {
		
		free_header -> prev = ((U32)existing_header >> 5);
		free_header -> size = num_frag_blocks;
		free_header -> is_alloc = 0;
		free_header -> next_null = list_root -> next_null; 
		free_header -> next = list_root -> next;
		
		if (existing_header -> next_null == 0){
			old_next_header = (Block_Header*)(START_INDEX | (U32)((existing_header -> next) << 5 ));
			old_next_header -> prev = ((U32)free_header >> 5);
		}
		
		
		existing_header -> next = ((U32)free_header >> 5);
		existing_header -> size = num_blocks;
		existing_header -> is_alloc = 1;
		existing_header -> next_null = 0; 
		
		free_bucket_node = (Bucket_Node*)((char*)(free_header) + 4);
		free_bucket_node -> next = NULL;
		
		index = LOG2_DOWN(num_frag_blocks);
		load_bucket_block(free_bucket_node, index);
		
	} else {
		existing_header -> size = num_blocks;
		existing_header -> is_alloc = 1;
		if (list_root -> next_null == 1)
			existing_header -> next_null = 1;
	}
	if (existing_header == NULL){
		return NULL;
	}
	else{		
		return (void*)(((char*)(existing_header) + 4));
	}
	
}

void  half_free(void * address){
	
	/***********************************************************
	Declaring Locals
	***********************************************************/
	
	Block_Header* header;
	Block_Header* left_header;
	Block_Header* right_header;
	Block_Header* right_header_right;
	
	Bucket_Node* left_bucket_node;
	Bucket_Node* left_bucket_left;
	Bucket_Node* left_bucket_right;
	
	Bucket_Node* right_bucket_node;
	Bucket_Node* right_bucket_left;
	Bucket_Node* right_bucket_right;
	
  Bucket_Node* free_bucket_right;
	Bucket_Node * free_bucket_node;
	U16 merge_size;
	U32 index;

	/***********************************************************
		Checking Valid Address
	***********************************************************/

	if (address == NULL)
		return;
	
	header = (Block_Header*)((char*)(address) - 4);
		
	if (header -> is_alloc == 0 || header -> is_alloc == NULL || address == NULL)
			return;
	
	left_header = (Block_Header*)(START_INDEX | (U32)((header -> prev) << 5 ));
	
	if (header -> next == 0 && header -> next_null == 1)
		right_header = NULL;
	else
		right_header = (Block_Header*)(START_INDEX | (U32)((header -> next) << 5 )); // null and 1000  evaluate to the same thing

	header -> is_alloc = 0;

	/***********************************************************
	Merging Header With Free Block To The Left
	***********************************************************/
	
	if (!left_header -> is_alloc && header != (Block_Header*)(START_INDEX)){
		merge_size = left_header -> size;
		
		left_header -> next = header -> next;
		right_header -> prev = header -> prev;
		left_header -> size = left_header -> size + header -> size;
		left_header -> next_null = header -> next_null; //
		
		left_bucket_node = GET_BUCKET_NODE(left_header);
		left_bucket_left = (Bucket_Node*)(left_bucket_node -> prev);
		left_bucket_right = (Bucket_Node*)(left_bucket_node -> next);
		
		if (left_bucket_left != NULL) {
			left_bucket_left -> next = (Bucket_Node*)(left_bucket_right);
		}
		
		if (left_bucket_right != NULL) {
			 left_bucket_right -> prev = (Bucket_Node*)(left_bucket_left);
		}
		
		header = left_header;
		 
		index = LOG2_DOWN(merge_size);
		if (buckets[index].next == NULL)
			reset_vector_bit(index);
		
	}
	
	/***********************************************************
	Merging Header With Free Block To The Right
	***********************************************************/
	
	if(!right_header -> is_alloc && header -> next_null == 0 && right_header != NULL){
		merge_size = right_header -> size;
		
		if (right_header -> next_null == 0){
			right_header_right = (Block_Header*)(START_INDEX | (U32)((right_header -> next) << 5 ));
			header -> next = ((U32)right_header_right >> 5);
			right_header_right -> prev = right_header -> prev;
		} else {
			header -> next = NULL;
		}
		
		header -> next_null = right_header -> next_null;
		
		header -> size = header -> size + right_header -> size;
		
		right_bucket_node = GET_BUCKET_NODE(right_header);
		right_bucket_left = (Bucket_Node*)(right_bucket_node -> prev);
		right_bucket_right = (Bucket_Node*)(right_bucket_node -> next);
		

		if (right_bucket_left != NULL) {
			right_bucket_left -> next = (Bucket_Node*)(right_bucket_right);
		}
		
		if (right_bucket_right != NULL) {
			right_bucket_right -> prev = (Bucket_Node*)(right_bucket_left);
		}
	
		index = LOG2_DOWN(merge_size);
		if (buckets[index].next == NULL)
			reset_vector_bit(index);
	}
	
	
		
	/***********************************************************
		Adding Bucket To Linked List
	***********************************************************/
	
	free_bucket_node = GET_BUCKET_NODE(header);
	free_bucket_node -> next = NULL;
	
	index = LOG2_DOWN(replace0(header -> size));
	
	load_bucket_block(free_bucket_node, index);
	
	
}

/***********************************************************
	Set Bit Vector Bit
***********************************************************/

void set_vector_bit(U8 i){
	bins.avail = bins.avail | set_values [i];
}

/***********************************************************
	Resetting Bit Vector Bit
***********************************************************/

void reset_vector_bit (U8 i){
	bins.avail = bins.avail & reset_values [i];
}

/***********************************************************
	Routine to Insert a block into it's bucket
***********************************************************/

void load_bucket_block(Bucket_Node* free_bucket_node, U8 index){
	Bucket_Node* free_bucket_right;
	if (CHECK_BIT(index)){
		free_bucket_right = (Bucket_Node*)(buckets[index].next);
		free_bucket_node -> next = free_bucket_right;
		free_bucket_right -> prev = free_bucket_node;
	}
	
	buckets[index].next = free_bucket_node;
	free_bucket_node -> prev = &(buckets[index]);
	set_vector_bit(index);
}

/***********************************************************
	See Macros for other routines
***********************************************************/




