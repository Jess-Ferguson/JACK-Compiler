/* 
 *	Filename:	huffman.c
 *	Author:		Jessica Turner (highentropystring@gmail.com)
 *	Date:		17/07/18
 *	Licence:	GNU GPL V3
 *
 *	Encodes and decodes a byte stream using huffman coding
 *
 *	Return/exit codes:
 *
 *		EXIT_SUCCESS	- No error
 *		MEM_ERROR		- Memory allocation error
 *		INPUT_ERROR		- No input given
 *
 *	User Functions:
 *
 *		- huffman_encode()		- Encodes a string using Huffman coding
 *		- huffman_decode()		- Decodes a Huffman encoded string 
 *
 *	Helper Functions:
 *
 *		Encoding:
 *
 *			- huff_tree_from_freq()		- Generate a Huffman tree from a frequency analysis
 *			- code_array_from_tree()	- Generate a "code array" from the huffman tree, used for fast encoding
 *			- node_compare()			- Calculate the difference in frequency between two nodes
 *			- node_compare()			- Modified version of node_compare() which prioritises character nodes over internal nodes when frequencies are equal
 *
 *		Decoding:
 *
 *			- huff_tree_from_codes()	- Generates a Huffman tree from a stored "code array"
 *			- is_char_node()			- Determine if a given byte is a character node in a Huffman tree
 *			- add_char_to_tree()		- Adds a character and its encoding byte to a Huffman tree
 *
 *		Universal:
 *
 *			- create_char_node()		- Generate a character node
 *			- create_internal_node()	- Generate an internal node
 *			- destroy_huff_tree()		- Traverses the tree and frees all memory associated with it
 *
 *	Data Structures:
 *
 *		Code Array:
 *
 *			- Fast way to encode data using the information generated from a Huffman tree and an easy way to store a representation of the tree
 *			- Represents each byte to be encoded and how it is encoded allowing for O(1) time to determine how a given byte is encoded
 *			- Position in the array (i.e. code_array[0-255]) represents the byte to be encoded
 *
 *		Huffman Tree:
 *
 *			- Binary tree that operates much like any other Huffman tree
 *			- Contains two types of nodes, internal nodes and character nodes
 *			- Every node contains either the frequency of the character it represents or the combined frequencies of its child nodes
 *
 *	Encoded Data Format:
 *
 *		Header:
 *
 *			- Compressed string length (uint32_t stored as 4 uint8_t's)
 *			- Decompressed string length (uint32_t stored as 4 uint8_t's)
 *			- Header size (uint16_t stored as 2 uint8_t's)
 *			- Huffman tree stored as a "code array" (3 bytes per character: encoded character, encoding byte, number of bits set)
 *		
 *		Encoded data:
 *
 *			- The input stored in its compressed huffman form
 *
 *	Todo:
 *
 *		- Fix decoding algorithm
 *
 *	The Future:
 *
 *		- Modify decoding algorithm to use a hash table lookup rather than tree recursion
 *		- Find way to reduce header size, possibly using the huffman algorithm twice to encode the header?
 *		- Look into using a terminator byte instead of storing length, might reduce total size
 *		- Combine with duplicate string removal and make full LZW compression
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "huffman.h"

int huff_tree_from_freq(size_t * freq, huffman_node_t ** head_node) {
	huffman_node_t * node_list[256] = { NULL };
	huffman_node_t * first_temp_node, * second_temp_node, * internal_node;
	huffman_node_t ** node_list_p;
	size_t node_count = 0;

	for(uint16_t i = 0; i < 256; i++)
		if(freq[i] && !(node_list[node_count++] = create_char_node(i - 128, freq[i])))
			return MEM_ERROR;

	node_list_p = node_list;

	while(node_count > 1) {
		qsort(node_list_p, node_count, sizeof(huffman_node_t *), node_compare);

		first_temp_node = node_list_p[0];
		second_temp_node = node_list_p[1];

		if(!(internal_node = create_internal_node(first_temp_node, second_temp_node)))
			return MEM_ERROR;

		node_list_p[0] = NULL;
		node_list_p[1] = internal_node;

		node_list_p++;
		node_count--;
	}

	*head_node = node_list_p[0];

	return EXIT_SUCCESS;
}

void destroy_huff_tree(huffman_node_t * node)
{
	if(node->child[0]) {
		destroy_huff_tree(node->child[0]);
		destroy_huff_tree(node->child[1]);
	}

	free(node);

	return;
}

int huffman_encode(char * input, uint8_t ** output)
{
	size_t freq[256]		= { 0 };
	uint16_t header_size	= HEADER_BASE_SIZE;
	uint32_t length			= strlen(input);

	for(size_t i = 0; i < length; i++)
		freq[input[i] + 128]++;

	for(uint16_t i = 0; i < 256; i++)
		if(freq[i])
			header_size += 3;

	/* Handle strings with either one unique byte or zero bytes */

	if(header_size == HEADER_BASE_SIZE) {
		return INPUT_ERROR;
	} else if(header_size == HEADER_BASE_SIZE + 3) {
		for(uint16_t i = 0; i < 256; i++)
			if(freq[i])
				++freq[i > 0 ? i - 1 : i + 1];
			
		header_size += 3;
	}

	huffman_node_t * head_node = NULL;

	if(huff_tree_from_freq(freq, &head_node) != EXIT_SUCCESS)
		return MEM_ERROR;

	huffman_encode_t codes[256] = {{ .code = 0, .length = 0 }};

	code_array_from_tree(head_node, codes, 0);
	destroy_huff_tree(head_node);

	size_t encoded_bit_len = 0;

	/* Use the generated code array to calculate the byte length of the output */

	for(size_t i = 0; i < length; i++)
		encoded_bit_len += codes[input[i] + 128].length;

	size_t encoded_byte_len = (encoded_bit_len >> 3) + !!(encoded_bit_len & 0x7); /* Calculate bit length / 8, add one if there's a remainder */

	uint8_t * str_out = NULL;

	if(!(str_out = calloc(encoded_byte_len + header_size + 1, sizeof(uint8_t))))
		return MEM_ERROR;

	/* Write header information */

	/* Bit level hack to store uint32_t's and uint16_t's in an array of uint8_t's */

	str_out[0] = (uint8_t)length;
	str_out[1] = (uint8_t)(length >> 0x8);
	str_out[2] = (uint8_t)(length >> 0x10);
	str_out[3] = (uint8_t)(length >> 0x18);

	str_out[4] = (uint8_t)encoded_byte_len;
	str_out[5] = (uint8_t)(encoded_byte_len >> 0x8);
	str_out[6] = (uint8_t)(encoded_byte_len >> 0x10);
	str_out[7] = (uint8_t)(encoded_byte_len >> 0x18);

	str_out[8] = (uint8_t)header_size;
	str_out[9] = (uint8_t)(header_size >> 0x8);

	size_t byte_pos = HEADER_BASE_SIZE;

	/* Store the encoding information */

	for(uint16_t i = 0; i < 256; i++) {
		if(codes[i].length) {
			str_out[byte_pos++] = i;
			str_out[byte_pos++] = codes[i].code;
			str_out[byte_pos++] = codes[i].length;

			printf("Encoding for character \"%c\" (%d bits):\n", i - 128, codes[i].length);

			for(int j = sizeof(uint8_t) << 3; j; j--)
				putchar('0' + (((codes[i].code) >> (j - 1)) & 1));

			putchar('\n');
		}
	}

	/* Encode output stream */

	for(size_t i = 0, bit_pos = 0; i < length; i++) {
		for(size_t j = 0; j < codes[input[i] + 128].length; j++) {
			str_out[byte_pos] |= ((codes[input[i] + 128].code >> j) & 0x1) << bit_pos;

			if(++bit_pos == 8) {
				bit_pos = 0;
				byte_pos++;
			}
		}
	}

	*output = str_out;

	return EXIT_SUCCESS;
}

void code_array_from_tree(huffman_node_t * node, huffman_encode_t huffman_array[256], uint8_t bits_set)
{
	static uint8_t byte = '\0';

	if(node->child[0]) {
		byte &= ~(0x1 << bits_set);
		code_array_from_tree(node->child[0], huffman_array, bits_set + 1);
		byte |= 0x1 << bits_set;
		code_array_from_tree(node->child[1], huffman_array, bits_set + 1);
	} else {
		huffman_array[node->c + 128].code = byte;
		huffman_array[node->c + 128].length = bits_set; // **FATAL** Some code lengths are incorrectly set
	}
}

/* <hellworld> */

int huffman_decode(uint8_t * input, char ** output)
{
	huffman_decode_t decoding_table[256] = {{ .symbol = '\0', .length = 0 }};
	size_t byte_pos				= 0;
	size_t bit_pos				= 0;
	uint32_t char_count			= 0;
	uint32_t length = * (uint32_t *) &input[0]; /* Get the length of the decoded string */
	uint16_t header_size = * (uint16_t *) &input[8];

	/* Build the decoding table from the stored Huffman Tree */

	for(byte_pos = HEADER_BASE_SIZE; byte_pos < header_size; byte_pos += 3) {
		uint32_t code = input[byte_pos + 1];
		int padlength = 8 - input[byte_pos + 2];
		uint32_t padmask = (1 << padlength) - 1;

		printf("Encodings for character \"%c\":\n", input[byte_pos] - 128);

		for (uint32_t padding = 0; padding <= padmask; padding++) {
			decoding_table[(code << padlength) | padding].symbol = input[byte_pos] - 128;
			decoding_table[(code << padlength) | padding].length = input[byte_pos + 2];

			for(int i = sizeof(uint8_t) << 3; i; i--)
				putchar('0' + ((((code << padlength) | padding) >> (i - 1)) & 1));
			putchar('\n');
		}
	}

	/* Build the output string from the encoded string and the decoding table */

	char * str_out = NULL;

	if(!(str_out = calloc(length + 1, sizeof(char))))
		return MEM_ERROR;

	while(char_count < length) {
		uint32_t symbol = peekKbits(input + byte_pos, bit_pos);
		str_out[char_count++] = decoding_table[symbol].symbol;
		bit_pos += decoding_table[symbol].length;
	}

	str_out[char_count] = '\0';
	*output = str_out;

	return EXIT_SUCCESS;
}

uint32_t peekKbits(uint8_t * input, int bit_pos) {
	int byte_pos = bit_pos >> 3;
	int bit_offset = 14 - (bit_pos & 7);
	uint32_t concat = (input[byte_pos] << 8) | input[byte_pos + 1];
	return (concat >> bit_offset) & 3;
}

/* </hellworld> */

huffman_node_t * create_char_node(char c, size_t freq) {
	huffman_node_t * node;

	if(!(node = malloc(sizeof(huffman_node_t))))
		return NULL;

	node->freq = freq;
	node->child[0] = NULL;
	node->child[1] = NULL;
	node->c = c;

	return node;
}

huffman_node_t * create_internal_node(huffman_node_t * first_child, huffman_node_t * second_child) {
	huffman_node_t * node;

	if(!(node = malloc(sizeof(huffman_node_t))))
		return NULL;

	node->freq = first_child->freq + second_child->freq;
	node->child[0] = first_child;
	node->child[1] = second_child;

	return node;
}

int node_compare(const void * first_node, const void * second_node) {
	if(first_node == NULL)
		return -1;
	else if(second_node == NULL)
		return 0;

	const huffman_node_t * first_node_internal = *(huffman_node_t **) first_node;
	const huffman_node_t * second_node_internal = *(huffman_node_t **) second_node;

	if(!(first_node_internal->freq - second_node_internal->freq))
		return second_node_internal->child[0] ? 1 : -1;

	return first_node_internal->freq - second_node_internal->freq;

}
