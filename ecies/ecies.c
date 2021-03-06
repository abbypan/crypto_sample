#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_snprintf        snprintf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C) && \
    defined(MBEDTLS_FS_IO)
#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pem.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/bignum.h"
#include "mbedtls/pk.h"
#include "mbedtls/gcm.h"
/*#include "mbedtls/pkparse.h"*/

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 2048

#endif

void hexdump(unsigned char *info, unsigned char *buf, const int num)
{
    int i;
    printf("\n%s, %d\n", info, num);

    for(i = 0; i < num; i++)
    {
        printf("%02x", buf[i]);
    }
    printf("\n");

    for(i = 0; i < num; i++)
    {
        printf("%02x ", buf[i]);
        if ((i+1)%8 == 0)
            printf("\n");
    }
    printf("\n");
    return;
}

char* get_random(int len){

    int i, k, ret = 1;

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    unsigned char *buf = mbedtls_calloc(len, sizeof(unsigned char));

    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_entropy_init( &entropy );
    ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "RANDOM_GEN", 10 );

    mbedtls_ctr_drbg_set_prediction_resistance( &ctr_drbg, MBEDTLS_CTR_DRBG_PR_OFF );

    ret = mbedtls_ctr_drbg_random( &ctr_drbg, buf, len );

    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    return buf;
}

int hkdf_hmac_sha256(char* key, size_t key_len, char* salt, size_t salt_len, char* label, size_t label_len, unsigned char **out, size_t L){
    /*int key_len = sizeof(key);*/
    hexdump("hkdf key", key, key_len);

    /*int salt_len = sizeof(salt);*/
    hexdump("hkdf salt", salt, salt_len);

    /*int label_len = strlen(label);*/
    /*printf("\nhkdf label: %s\n", label);*/

    printf("\nhkdf L: %d\n", L);
    *out = mbedtls_calloc(L, sizeof(unsigned char));
    mbedtls_hkdf( mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 ), salt, salt_len, key, key_len, label, label_len, *out, L);
    hexdump("hkdf okm", *out, L);

    return 0;
}

const mbedtls_pk_context * read_pk_from_cert(const char *path){
    printf("cert_file: %s\n", path);

    mbedtls_x509_crt *crt;
    crt = mbedtls_calloc( 1, sizeof( mbedtls_x509_crt ) );
    mbedtls_x509_crt_init(crt);

    mbedtls_x509_crt_parse_file( crt, path );

    /*size_t n;*/
    /*unsigned char *buf;*/
    /*mbedtls_pk_load_file( path, &buf, &n );*/
    /*printf("\n%s\n", buf);*/
    /*mbedtls_x509_crt_parse( crt, buf, n );*/

    /*const mbedtls_x509_time *time = &crt->valid_to;*/
    /*printf("\nvalid to: %d %d %d\n\n", time->year, time->mon, time->day);*/

    const mbedtls_pk_context *pk = &crt->pk;

    /*mbedtls_pk_type_t pk_type = mbedtls_pk_get_type( pk );*/
    /*printf("\npk type: %d\n\n", pk_type);*/

    return pk;
}



int sender_ecdh_z(const mbedtls_pk_context *receiver_pk, unsigned char **temp_pub, size_t *temp_pub_len, unsigned char **z, size_t *zlen ){
    size_t olen = 0; 

    mbedtls_ecdh_context ctx;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    //init ecdh
    mbedtls_ecdh_init( &ctx );

    //random generate temp key pair
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "RANDOM_GEN", 10 );

    mbedtls_ecp_keypair *receiver_ecp = mbedtls_pk_ec( *receiver_pk );
    const mbedtls_ecp_group_id gid = receiver_ecp->grp.id;
    mbedtls_ecp_group_load( &ctx.grp, gid); 


    mbedtls_ecdh_gen_public( &ctx.grp,         
            &ctx.d,              // temp priv key
            &ctx.Q,              // temp public key
            mbedtls_ctr_drbg_random, 
            &ctr_drbg );            

    size_t pbuf_len = mbedtls_mpi_size(&ctx.d);
    unsigned char *pbuf = mbedtls_calloc(pbuf_len, sizeof(unsigned char));
    mbedtls_mpi_write_binary( &ctx.d, pbuf, pbuf_len ); 
    hexdump("temp priv key",pbuf, pbuf_len);

    /*unsigned char buf[BUFLEN];*/
    /*mbedtls_ecp_point_write_binary( &ctx.grp, &ctx.Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, buf, buflen);*/
    /*temp_pub = mbedtls_calloc(olen, sizeof(unsigned char));*/
    /**temp_pub_len = olen;*/
    /*memcpy( temp_pub, buf, olen );*/
    /*hexdump("temp public key", temp_pub, *temp_pub_len);*/


    const mbedtls_pk_type_t pk_type = mbedtls_pk_get_type(receiver_pk);
    const mbedtls_pk_info_t *pk_info_t = mbedtls_pk_info_from_type( pk_type );

    mbedtls_ecp_keypair *temp_ecp =  mbedtls_calloc( 1, sizeof( mbedtls_ecp_keypair ) );
    mbedtls_ecp_copy(&temp_ecp->Q, &ctx.Q); 
    mbedtls_mpi_copy(&temp_ecp->d, &ctx.d); 
    mbedtls_ecp_group_copy( &temp_ecp->grp, &ctx.grp );

    mbedtls_pk_context *temp_pk_ctx = mbedtls_calloc( 1, sizeof( mbedtls_pk_context ) );
    temp_pk_ctx->pk_info = pk_info_t;
    temp_pk_ctx->pk_ctx = temp_ecp;
    unsigned char temp_pub_pem[BUFLEN];
    mbedtls_pk_write_pubkey_pem(temp_pk_ctx, temp_pub_pem, BUFLEN);

    *temp_pub_len = strlen(temp_pub_pem);
    *temp_pub = mbedtls_calloc(*temp_pub_len, sizeof(unsigned char));
    strcpy( *temp_pub, temp_pub_pem);
    printf("temp pub pem:\n%s\n", *temp_pub);


    //set receiver public key
    mbedtls_ecp_copy(&ctx.Qp, &receiver_ecp->Q); //receiver public key

    unsigned char tbuf[BUFLEN];
    mbedtls_ecp_point_write_binary( &ctx.grp, &receiver_ecp->Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, tbuf, BUFLEN);
    hexdump("receiver public key", tbuf, olen);

    //calc Z
    mbedtls_ecdh_compute_shared( &ctx.grp,           
            &ctx.z,           
            &ctx.Qp,           
            &ctx.d,             
            mbedtls_ctr_drbg_random,
            &ctr_drbg );          

    /*int zlen = (&ctx.z)->n;*/
    *zlen = mbedtls_mpi_size(&ctx.z);
    *z = mbedtls_calloc(*zlen, sizeof(unsigned char));
    mbedtls_mpi_write_binary( &ctx.z, *z, *zlen ); 
    hexdump("sender ecdh z",*z, *zlen);

    return 0;
}

static int write_file( const char *path, unsigned char *buf, size_t n )
{
    FILE *f;

    if( ( f = fopen( path, "wb" ) ) == NULL )
        return( -1 );

    if( fwrite( buf, 1, n, f ) != n )
    {
        fclose( f );
        return( -1 );
    }

    fclose( f );
    return( 0 );
}

int receiver_ecdh_z( mbedtls_pk_context *receiver_priv, unsigned char *temp_pub, size_t temp_pub_len, unsigned char **z, size_t *zlen ){

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "RANDOM_GEN", 10 );

    //init ecdh
    mbedtls_ecdh_context ctx;
    mbedtls_ecdh_init( &ctx );

    mbedtls_ecp_keypair *receiver_ecp = mbedtls_pk_ec( *receiver_priv );
    const mbedtls_ecp_group_id gid = receiver_ecp->grp.id;
    /*printf("gid: %d\n", gid);*/

    mbedtls_ecp_group_load( &ctx.grp, gid); 

    mbedtls_mpi_copy(&ctx.d, &receiver_ecp->d);
    size_t pbuf_len = mbedtls_mpi_size(&ctx.d);
    unsigned char *pbuf = mbedtls_calloc(pbuf_len, sizeof(unsigned char));
    mbedtls_mpi_write_binary( &ctx.d, pbuf, pbuf_len ); 
    hexdump("receiver priv key",pbuf, pbuf_len);

    mbedtls_pk_context *temp_pub_ctx = mbedtls_calloc( 1, sizeof( mbedtls_pk_context ) );

    printf("receive temp pub pem %d\n%s\n", temp_pub_len, temp_pub);
    write_file("temp_pub.pem", temp_pub, temp_pub_len);

    /*int ret = mbedtls_pk_parse_public_keyfile(temp_pub_ctx, "temp_pub.pem");*/
    int ret = mbedtls_pk_parse_public_key( temp_pub_ctx, temp_pub, temp_pub_len*8);
    printf("ret: %x\n", ret);

    mbedtls_ecp_keypair *temp_pub_ecp ;
    temp_pub_ecp = mbedtls_pk_ec( *temp_pub_ctx );
    mbedtls_ecp_copy(&ctx.Qp, &temp_pub_ecp->Q);

    unsigned char buf[BUFLEN];
    size_t olen;
    mbedtls_ecp_point_write_binary( &ctx.grp, &ctx.Qp, MBEDTLS_ECP_PF_COMPRESSED, &olen, buf, BUFLEN);
    hexdump("temp public key", buf, olen);

    //calc Z
    mbedtls_ecdh_compute_shared( &ctx.grp,           
            &ctx.z,           
            &ctx.Qp,           
            &ctx.d,             
            mbedtls_ctr_drbg_random,
            &ctr_drbg );          

    *zlen = mbedtls_mpi_size(&ctx.z);
    *z = mbedtls_calloc(*zlen, sizeof(unsigned char));
    mbedtls_mpi_write_binary( &ctx.z, *z, *zlen ); 
    hexdump("ecdh z",*z, *zlen);

    return 0;
}

int aes_gcm_encrypt(unsigned char* key, size_t key_len, unsigned char* iv, size_t iv_len, unsigned char* input, size_t input_len, unsigned char* aad, unsigned char **output, size_t* output_len, unsigned char** tag, size_t tag_len){
    hexdump("aes encrypt: plaintext", input, input_len);
    hexdump("aes encrypt key", key, key_len);
    hexdump("aes encrypt iv", iv, iv_len);
    hexdump("aes encrypt aad", aad, strlen(aad));

    *output_len = input_len;
    *output = mbedtls_calloc(*output_len, sizeof(unsigned char));

    mbedtls_gcm_context aes;
    mbedtls_gcm_init( &aes );
    mbedtls_gcm_setkey( &aes,MBEDTLS_CIPHER_ID_AES , (const unsigned char*) key, key_len*8);
    mbedtls_gcm_starts(&aes, MBEDTLS_GCM_ENCRYPT, (const unsigned char*)iv, iv_len, (const unsigned char*) aad, strlen(aad));
    mbedtls_gcm_update(&aes,input_len,(const unsigned char*)input, *output);
    mbedtls_gcm_finish(&aes, *tag, tag_len);
    mbedtls_gcm_free( &aes );

    hexdump("aes encrypt: tag", *tag, tag_len);
    hexdump("aes encrypt: cipher", *output, *output_len);

    return 0;
}

int aes_gcm_decrypt(unsigned char* key, size_t key_len, unsigned char* iv, size_t iv_len, unsigned char* input, size_t input_len, unsigned char* aad, unsigned char** output, size_t* output_len, unsigned char* tag, size_t tag_len){
    hexdump("aes decrypt: cipher", input, input_len);
    hexdump("aes decrypt key", key, key_len);
    hexdump("aes decrypt iv", iv, iv_len);
    hexdump("aes decrypt aad", aad, strlen(aad));
    hexdump("aes decrypt: tag", tag, tag_len);

    *output_len = input_len;

    mbedtls_gcm_context aes;
    mbedtls_gcm_init( &aes );
    mbedtls_gcm_setkey( &aes,MBEDTLS_CIPHER_ID_AES , (const unsigned char*) key, key_len * 8);
    mbedtls_gcm_starts(&aes, MBEDTLS_GCM_DECRYPT, (const unsigned char*)iv, iv_len, (const unsigned char*) aad, strlen(aad));
    mbedtls_gcm_update(&aes,input_len,(const unsigned char*)input, *output);
    mbedtls_gcm_finish(&aes, tag, tag_len);
    mbedtls_gcm_free( &aes );

    hexdump("aes decrypt: plaintext", *output, *output_len);

    return 0;
}


void main( int argc, char *argv[] )
{
    //sender
    printf("sender:\n\n");
    char cert_f[BUFLEN] = "receiver_ee_cert.pem";

    const mbedtls_pk_context *pk = read_pk_from_cert(cert_f);
    unsigned char *temp_pub;
    size_t temp_pub_len;
    unsigned char *sender_z;
    size_t sender_zlen;
    sender_ecdh_z(pk, &temp_pub, &temp_pub_len, &sender_z, &sender_zlen);


    size_t salt_len = 32;
    unsigned char *salt = get_random(salt_len);

    unsigned char label[BUFLEN] = "someapp.somebusiness.somelabel";

    size_t L = 32;
    unsigned char *okm; 
    hkdf_hmac_sha256(sender_z, sender_zlen, salt, salt_len, label, strlen(label), &okm, L);

    //encrypt data
    size_t iv_len = 12;
    unsigned 	char *iv = get_random(iv_len);
    unsigned 	char data[] = "fujian quanzhou 66666666666666666666666666666666666";
    unsigned     char aad[] = "somedevice.context";

    size_t cipher_info_len;
    unsigned char *cipher_info;
    size_t tag_len = 16;
    unsigned     char *tag = mbedtls_calloc(tag_len, sizeof(unsigned char));
    aes_gcm_encrypt(okm, L, iv, iv_len, data, strlen(data), aad, &cipher_info, &cipher_info_len, &tag, tag_len);

    //receiver
    printf("\nreceiver:\n\n");
    char *receiver_priv_pwd = NULL;
    mbedtls_pk_context *receiver_priv = mbedtls_calloc( 1, sizeof( mbedtls_pk_context ) );
    mbedtls_pk_parse_keyfile(receiver_priv, "receiver_ee_priv.pem", receiver_priv_pwd);

    unsigned char *receiver_z;
    size_t receiver_zlen;
    receiver_ecdh_z( receiver_priv, temp_pub, temp_pub_len, &receiver_z, &receiver_zlen);
    unsigned char *r_okm;
    hkdf_hmac_sha256(receiver_z, receiver_zlen, salt, salt_len, label, strlen(label), &r_okm,  L);

    unsigned char *plaintext =  mbedtls_calloc(cipher_info_len, sizeof(unsigned char));
    size_t plaintext_len;
    aes_gcm_decrypt(r_okm, L, iv, iv_len, cipher_info, cipher_info_len, aad, &plaintext, &plaintext_len, tag, tag_len);
}
