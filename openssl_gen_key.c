#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "openssl/rsa.h"

// BN_sqr(a, b, ctx); needs a context ctx and calculates a = b^2

int main() 
{
	// generate rsa key
	printf("Generating RSA key...\n");
	RSA* rsa = RSA_generate_key(512, 65537, NULL, NULL);
	printf("RSA key generated!\n");
	
	// dump key to file
	FILE* f = fopen("key.pem", "w");
	if (f == NULL) {
		printf("Error opening file for writing\n");
		return 1;
	}
	PEM_write_RSAPrivateKey(f, rsa, NULL, NULL, 0, NULL, NULL);
	fclose(f);
	printf("Key written to file\n");

	// load key from file
	f = fopen("key.pem", "r");
	if (f == NULL) {
		printf("Error opening file for reading\n");
		return 1;
	}
	printf("Loading key from file...\n");
	PEM_read_RSAPrivateKey(f, rsa, NULL, NULL);
	printf("Key loaded!\n");

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
	return 0;
}
