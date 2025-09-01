#ifndef NETWORKING_ENCRYPTION_H
#define NETWORKING_ENCRYPTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include "networking/buffer.h"

typedef struct {
    RSA *rsa_key;              // RSA keypair (server private or client public)
    EVP_CIPHER_CTX *enc_ctx;   // AES/CFB8 encryption context
    EVP_CIPHER_CTX *dec_ctx;   // AES/CFB8 decryption context
    Buffer *shared_secret;     // Shared secret
} RSA_CryptoContext;

RSA_CryptoContext* RSA_init(void);

// Generate 16-byte shared secret and setup AES keys
// Generate RSA keypair (server if pub_der == NULL) or load public key (client),
// and also generate the shared secret + AES contexts.
int RSA_generate(RSA_CryptoContext *ctx, const Buffer *pub_der);

// Encrypt data (shared secret or verify token) with RSA public key
int RSA_encrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output);

// Decrypt data (shared secret or verify token) with RSA private key
int RSA_decrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output);

// Cleanup context and OpenSSL resources
void RSA_cleanup(RSA_CryptoContext *ctx);

/*
USAGE EXAMPLE:

// ==========================================================
// --- SERVER SIDE ---
// ==========================================================

// 1. Create and initialize server crypto context
RSA_CryptoContext *server_ctx = RSA_init();

// 2. Generate a 1024-bit RSA keypair
RSA_generate(server_ctx, NULL);

// 3. Export server's public key (DER format) to send to client
Buffer *pub_der = init_buffer();
unsigned char *tmp = NULL;
int pub_len = i2d_RSA_PUBKEY(server_ctx->rsa_key, &tmp);
append_to_buffer(pub_der, (const char*)tmp, pub_len);
OPENSSL_free(tmp);

// --- send pub_der->buffer/pub_der->length to client ---


// ==========================================================
// --- CLIENT SIDE ---
// ==========================================================

// 1. Create and initialize client crypto context
RSA_CryptoContext *client_ctx = RSA_init();

// 2. Load server public key from DER
RSA_generate(client_ctx, pub_der);

// 3. Generate a random shared secret and initialize AES
//    (stored in client_ctx->shared_secret, used to setup enc/dec ctxs)
RSA_generate_secret(client_ctx);

// 4. Encrypt the shared secret with server's public key
Buffer *enc_shared = init_buffer();
RSA_encrypt(client_ctx, client_ctx->shared_secret, enc_shared);

// --- send enc_shared->buffer/enc_shared->length to server ---


// ==========================================================
// --- SERVER SIDE (after receiving enc_shared) ---
// ==========================================================

// 1. Decrypt the shared secret sent by client
Buffer *dec_shared = init_buffer();
RSA_decrypt(server_ctx, enc_shared, dec_shared);

// 2. Store the shared secret in server_ctx and initialize AES
append_to_buffer(server_ctx->shared_secret, dec_shared->buffer, dec_shared->length);
RSA_setup_aes(server_ctx);  // derive AES contexts from shared_secret

// ==========================================================
// --- ENCRYPTION/DECRYPTION READY ---
// ==========================================================

// Both sides now have:
//   - server_ctx->enc_ctx / server_ctx->dec_ctx
//   - client_ctx->enc_ctx / client_ctx->dec_ctx
// These can be used for AES-128-CFB8 packet encryption/decryption.
*/
#endif // NETWORKING_ENCRYPTION_H