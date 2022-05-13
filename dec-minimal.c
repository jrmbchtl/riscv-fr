#include "polarssl/pk.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"

#include <stdio.h>
#include <string.h>

#define KEYFILE "kyefile.key"
#define ENCRYPTION "result-enc.txt"

int main() {
    FILE *f;
    int c;
    size_t i, olen = 0;
    pk_context pk;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    unsigned char result[1024];
    unsigned char buf[512];
    const char *pers = "pk_decrypt";

    memset(result, 0, sizeof( result ) );

    printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) );

    printf( "\n  . Reading private key from '%s'", KEYFILE );
    fflush( stdout );

    pk_init( &pk );

    pk_parse_keyfile( &pk, KEYFILE, "" );

    /*
     * Extract the RSA encrypted value from the text file
     */
    f = fopen( "result-enc.txt", "rb" );
    i = 0;

    while( fscanf( f, "%02X", &c ) > 0 &&
           i < (int) sizeof( buf ) )
        buf[i++] = (unsigned char) c;

    fclose( f );

    printf( "\n  . Decrypting the encrypted data" );
    fflush( stdout );

    pk_decrypt( &pk, buf, i, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg );

                            printf( "\n  . OK\n\n" );

    printf( "The decrypted result is: '%s'\n\n", result );

    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );

    return 0;
}