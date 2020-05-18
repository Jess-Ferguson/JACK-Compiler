#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Header files */

#include <stdbool.h>
#include <stdint.h>

/* Return values */

#define EXIT_SUCCESS 0
#define MEM_ERROR 1
#define INPUT_ERROR 2

/* Node identifiers, might change to enumeration */

#define INTERNAL_NODE 0
#define CHARACTER_NODE 1

/* Size of the header with no characters stored */

#define HEADER_BASE_SIZE 10

/* Huffman Tree node */

typedef struct huffman_node_t {
	size_t freq;
	struct huffman_node_t * child[2];
	char c;
} huffman_node_t;

/* Lookup table used for encoding */

typedef struct huffman_encode_t {
	uint8_t code;
	uint8_t length;
} huffman_encode_t;

/* Lookup table used for decoding */

typedef struct huffman_decode_t {
	uint8_t symbol;
	uint8_t length;
} huffman_decode_t;

/* Interface Functions */

int huffman_decode(uint8_t * input, char ** output);
int huffman_encode(char * input, uint8_t ** output);

/* Helper Functions */

/* Decoding */

uint32_t peekKbits(uint8_t * input, int bit_pos);

/* Encoding */

int huff_tree_from_freq(size_t * freq, huffman_node_t ** head_node);
int node_compare(const void * first_node, const void * second_node);
void code_array_from_tree(huffman_node_t * node, huffman_encode_t huffman_array[256], uint8_t bits_set);

/* Universal */

huffman_node_t * create_char_node(char c, size_t freq);
huffman_node_t * create_internal_node(huffman_node_t * first_child, huffman_node_t * second_child);
void destroy_huff_tree(huffman_node_t * node);

#endif
