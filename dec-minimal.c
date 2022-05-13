#include "polarssl/config.h"
#include "polarssl/platform.h"

#include "polarssl/error.h"
#include "polarssl/pk.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"

#include <stdio.h>
#include <string.h>

#define KEYFILE "keyfile.key"
#define CIPHERFILE "result-enc.txt"

int main()
{
    FILE *f;
    int c;
    size_t i, olen = 0;
    pk_context pk;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    unsigned char result[1024];
    unsigned char buf[512] = {
        0x6F, 0x47, 0x3A, 0xFD, 0x23, 0x66, 0xAB, 0x6F, 0xB1, 0x83, 0xA2, 0x43, 0x53, 0x21, 0xC7, 0x6A,
        0x0D, 0xD8, 0x6B, 0x35, 0x65, 0xB4, 0x9A, 0x6C, 0x9D, 0x86, 0x8E, 0x5D, 0xA2, 0x76, 0x14, 0x1B,
        0x66, 0x67, 0x7C, 0x90, 0x5F, 0xB8, 0x0F, 0x3F, 0xA7, 0x5F, 0x9F, 0x95, 0x82, 0x2D, 0xEA, 0x45,
        0x25, 0xFD, 0x1A, 0x97, 0x65, 0xEA, 0x61, 0x33, 0xD1, 0x87, 0xC6, 0xD1, 0x96, 0x3F, 0xA9, 0x97,
        0x7C, 0xD3, 0x24, 0x60, 0x4C, 0x6A, 0x57, 0x9A, 0xF8, 0x7A, 0x50, 0x9F, 0xAE, 0x53, 0xA5, 0x38,
        0xD1, 0xF3, 0x55, 0x39, 0x9C, 0xD3, 0x5A, 0xFD, 0x05, 0x90, 0x1C, 0xE0, 0x30, 0x3E, 0x77, 0x0E,
        0x0B, 0x14, 0x0A, 0x7A, 0x15, 0x7F, 0x62, 0xBE, 0xFB, 0x49, 0x7B, 0x95, 0xE9, 0x01, 0xB0, 0x6C,
        0xC7, 0x46, 0xD5, 0x98, 0x2C, 0xC6, 0x9F, 0x4D, 0x7C, 0x72, 0xA1, 0x75, 0xD9, 0xB0, 0x8D, 0x18
    };
    const char *pers = "pk_decrypt";

    memset(result, 0, sizeof( result ) );

    entropy_init( &entropy );
    ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) );

    pk_init( &pk );
    pk_parse_keyfile( &pk, KEYFILE, "" );

    /*
     * Extract the RSA encrypted value from the text file
     */

    f = fopen( CIPHERFILE, "rb" );
    i = 0;
    while( fscanf( f, "%02X", &c ) > 0 &&
           i < (int) sizeof( buf ) )
        buf[i++] = (unsigned char) c;
    fclose( f );

    /*
     * Decrypt the encrypted RSA data and print the result.
     */

    pk_decrypt( &pk, buf, i, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg );
    printf( "The decrypted result is: '%s'\n\n", result );

    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );

    return 0;
}