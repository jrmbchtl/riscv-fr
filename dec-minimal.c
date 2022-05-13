#include "polarssl/config.h"
#include "polarssl/platform.h"

#include "polarssl/error.h"
#include "polarssl/pk.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"

#include <stdio.h>
#include <string.h>

#define KEYFILE "keyfile.key"

int main()
{
    FILE *f;
    int ret, c;
    size_t i, olen = 0;
    pk_context pk;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    unsigned char result[1024];
    unsigned char buf[512];
    const char *pers = "pk_decrypt";

    memset(result, 0, sizeof( result ) );
    ret = 1;

    polarssl_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! ctr_drbg_init returned %d\n", ret );
        goto exit;
    }

    polarssl_printf( "\n  . Reading private key from '%s'", KEYFILE );
    fflush( stdout );

    pk_init( &pk );

    if( ( ret = pk_parse_keyfile( &pk, KEYFILE, "" ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! pk_parse_keyfile returned -0x%04x\n", -ret );
        goto exit;
    }

    /*
     * Extract the RSA encrypted value from the text file
     */
    ret = 1;

    if( ( f = fopen( "result-enc.txt", "rb" ) ) == NULL )
    {
        polarssl_printf( "\n  ! Could not open %s\n\n", "result-enc.txt" );
        goto exit;
    }

    i = 0;

    while( fscanf( f, "%02X", &c ) > 0 &&
           i < (int) sizeof( buf ) )
        buf[i++] = (unsigned char) c;

    fclose( f );

    /*
     * Decrypt the encrypted RSA data and print the result.
     */
    polarssl_printf( "\n  . Decrypting the encrypted data" );
    fflush( stdout );

    if( ( ret = pk_decrypt( &pk, buf, i, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! pk_decrypt returned -0x%04x\n", -ret );
        goto exit;
    }

    polarssl_printf( "\n  . OK\n\n" );

    polarssl_printf( "The decrypted result is: '%s'\n\n", result );

    ret = 0;

exit:
    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );

    return( ret );
}