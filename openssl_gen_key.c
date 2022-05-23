#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "openssl/rsa.h"

// BN_sqr(a, b, ctx); needs a context ctx and calculates a = b^2

int main() 
{
	// generate rsa key
	printf("Generating RSA key...\n");
	RSA* rsa = RSA_generate_key(128, 3, NULL, NULL);
	printf("RSA key generated!\n");
	// print primes

	char* input = "Hello World!";
	int input_len = strlen(input);
	int output_len = RSA_size(rsa);
	unsigned char* output = malloc(output_len);
	printf("Encrypting...\n");
	int ret = RSA_public_encrypt(input_len, (unsigned char*)input, output, rsa, RSA_PKCS1_PADDING);
	if (ret == -1) {
		printf("encrypt error\n");
		return -1;
	}
	printf("Encryption success!\n");

	unsigned char* decrypted = malloc(input_len);
	printf("Decrypting...\n");
	ret = RSA_private_decrypt(output_len, output, decrypted, rsa, RSA_PKCS1_PADDING);
	if (ret == -1) {
		printf("decrypt error\n");
		return -1;
	}
	printf("Decryption success!\n");
	printf("Decrypted: %s\n", decrypted);

	RSA_free(rsa);

	BIGNUM* a = BN_new();
	BIGNUM* b = BN_new();
	// set all values to 2
	BN_set_word(b, 2);
	// create BN_ctx
	BN_CTX* ctx = BN_CTX_new();
	BN_sqr(a, b, ctx);

	// print a
	printf("a: ");
	BN_print_fp(stdout, a);
	printf("\n");
	
	return 0;
}
