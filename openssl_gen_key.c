#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "openssl/rsa.h"

// BN_sqr(a, b, ctx); needs a context ctx and calculates a = b^2

int main() 
{
	// generate rsa key
	printf("Generating RSA key...\n");
	RSA* rsa = RSA_generate_key(64, 3, NULL, NULL);
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
	RSA* rsa2 = RSA_new();
	// set the size of the key
	PEM_read_RSAPrivateKey(f, &rsa2, NULL, NULL);
	fclose(f);
	printf("Key loaded!\n");
	
	// encrypt
	printf("Encrypting...\n");
	unsigned char* input = "Ciphertext";
	unsigned char* output = malloc(RSA_size(rsa));
	int len = RSA_public_encrypt(strlen((char*)input), input, output, rsa2, RSA_PKCS1_PADDING);
	printf("Encrypted %d bytes\n", len);
	
	// decrypt
	printf("Decrypting...\n");
	unsigned char* output2 = malloc(RSA_size(rsa));
	int len2 = RSA_private_decrypt(len, output, output2, rsa2, RSA_PKCS1_PADDING);
	printf("Decrypted %d bytes\n", len2);
	
	// print
	printf("Original: %s\n", input);
	printf("Decrypted: %s\n", output2);
	
	// cleanup
	free(output);
	free(output2);
	RSA_free(rsa);
	RSA_free(rsa2);
	return 0;
}
