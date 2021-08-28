#include "cert.h"

#include <openssl/asn1.h>
#include <openssl/evp.h>
#include <openssl/ossl_typ.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <stdio.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

struct CBytes read_file_to_bytes(const char* filename) {
    struct CBytes c_bytes;

    unsigned char* buffer = 0;
    long length;
    FILE * f = fopen (filename, "rb");

    if (f)
    {
	fseek (f, 0, SEEK_END);
	length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length);
	if (buffer)
        {
	    fread (buffer, 1, length, f);
	}
	fclose (f);
    }

    c_bytes.bytes = buffer;
    c_bytes.len = length;

    return c_bytes;
}

// https://stackoverflow.com/a/15082282/12339402
struct CBytes get_certificate(/*const char* name, const char* email_id*/) {
    struct CBytes cert_bytes;
    cert_bytes.bytes = NULL;
    cert_bytes.len = 0;

    /* Before we create the certificate, we first need a private key...
     * EVP_PKEY is a structure for storing an algorithm-independent private key */
    EVP_PKEY *pkey = EVP_PKEY_new();

    /* Using RSA to create key, then we assign this RSA key to the EVP_PKEY
     * structure */
    RSA *rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL); // The last two are callbacks, in which 3rd is to track progress
    if(rsa == NULL) {
        printf("ERROR: COULDN'T GENERATE RSA KEY");
	return cert_bytes;
    }
    EVP_PKEY_assign_RSA(pkey, rsa);

    // NOTE: We don't need to free RSA key now, it will get freed when pkey is
    // freed

    /* Now, we have the private key... now for the certificate
     * Using X509 structure to store a X509 certificate*/
    X509* x509_cert = X509_new();

    /* Setting serial number of certificate... we set it to 1... some HTTP
     * servers refuse certs with serial number 0 */
    ASN1_INTEGER_set(X509_get_serialNumber(x509_cert), 1);

    /* Set expiration time (+0 sec from now till +1 year) */
    X509_gmtime_adj(X509_get_notBefore(x509_cert), 0);
    X509_gmtime_adj(X509_get_notAfter(x509_cert), 31536000L);

    /* Set public key for the certificate, we can just assign the PKEY structure
     * here */
    X509_set_pubkey(x509_cert, pkey);

    /* Now we first set some info for issuer, then set the issuer...
     * As for issuer's name, we are just using the subject name of the
     * certificate for that
     * Then, 'C' - Country Code, 'O' - Organisation, 'CN' - Common Name*/
    X509_NAME *name = X509_get_subject_name(x509_cert);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"IN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"Techy15 AdiG.in", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);

    X509_set_issuer_name(x509_cert, name);

    /* Sign the certificate with our key, and using SHA512 hash for the signature */
    X509_sign(x509_cert, pkey, EVP_sha512());

    /* Exporting - We export 2 files, .pem (private signature, generally password protected), .cert (the actual certificate) */
    FILE *pem, *cert_file;
    pem = fopen("key.pem", "wb");
    PEM_write_PrivateKey(
	    pem,
	    pkey,
	    EVP_des_ede3_cbc(),	// Cipher, default cypher for encryting the key on disk
	    NULL,
	    0,
	    NULL,	// callback for requesting a password
	    NULL	// data to pass to callback
	);

    cert_file = fopen("cert.pem", "wb");
    PEM_write_X509(
	    cert_file,
	    x509_cert);

    cert_bytes = read_file_to_bytes("cert.pem");

    X509_free(x509_cert);
    EVP_PKEY_free(pkey);
    return cert_bytes;
}
