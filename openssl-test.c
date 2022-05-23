#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "openssl/rsa.h"

int main() 
{
	// load key from file
	FILE* f = fopen("key.pem", "r");
	if (f == NULL) {
		printf("Error opening file for reading\n");
		return 1;
	}
	printf("Loading key from file...\n");
	RSA* rsa = RSA_new();
	// set the size of the key
	PEM_read_RSAPrivateKey(f, &rsa, NULL, NULL);
	fclose(f);
	printf("Key loaded!\n");
	
	// encrypt
	printf("Encrypting...\n");
	unsigned char* input = "Ciphertext";
	unsigned char* output = malloc(RSA_size(rsa));
	int len = RSA_public_encrypt(strlen((char*)input), input, output, rsa, RSA_PKCS1_PADDING);
	printf("Encrypted %d bytes\n", len);
	
	// decrypt
	printf("Decrypting...\n");
	unsigned char* output2 = malloc(RSA_size(rsa));
    printf("RSA_size(rsa): %d\n", RSA_size(rsa));
	int len2 = RSA_private_decrypt(len, output, output2, rsa, RSA_PKCS1_PADDING);
	printf("Decrypted %d bytes\n", len2);
	
	// print
	printf("Original: %s\n", input);
	printf("Decrypted: %s\n", output2);
	
	// cleanup
	free(output);
	free(output2);
	RSA_free(rsa);
	return 0;
}
