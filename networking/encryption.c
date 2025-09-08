#include "encryption.h"

// Handle OpenSSL errors
static void handleErrors(void) {
    ERR_print_errors_fp(stderr);
    abort();
}

// Initialize OpenSSL and context
RSA_CryptoContext* RSA_init(void) {
    RSA_CryptoContext *ctx = malloc(sizeof(RSA_CryptoContext));
    if (!ctx) {
        printf("Memory allocation failed\n");
        abort();
    }
    ctx->rsa_key = NULL;
    ctx->enc_ctx = NULL;
    ctx->dec_ctx = NULL;
    ctx->shared_secret = init_buffer();

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    if (RAND_poll() != 1) {
        free_buffer(ctx->shared_secret);
        free(ctx);
        handleErrors();
    }
    return ctx;
}

// Generate 16-byte shared secret and setup RSA keys
// Generate RSA keypair (server if pub_der == NULL) or load public key (client),
// and also generate the shared secret + RSA contexts.
int RSA_generate(RSA_CryptoContext *ctx, const Buffer *pub_der, const Buffer *secret) {
    if (!pub_der) {
        // --- Server mode: Generate 1024-bit RSA keypair ---
        BIGNUM *e_bn = BN_new();
        if (!e_bn || BN_set_word(e_bn, RSA_F4) != 1) {
            if (e_bn) BN_free(e_bn);
            handleErrors();
        }
        ctx->rsa_key = RSA_new();
        if (!ctx->rsa_key || RSA_generate_key_ex(ctx->rsa_key, 1024, e_bn, NULL) != 1) {
            BN_free(e_bn);
            handleErrors();
        }
        BN_free(e_bn);

        // --- Generate or use provided secret ---
        unsigned char temp[16];
        if (secret && secret->length >= 16) {
            memcpy(temp, secret->buffer, 16);
        } else {
            if (RAND_bytes(temp, sizeof(temp)) != 1) {
                handleErrors();
            }
        }
        append_to_buffer(ctx->shared_secret, (const char *)temp, sizeof(temp));

        // --- RSA init ---
        ctx->enc_ctx = EVP_CIPHER_CTX_new();
        ctx->dec_ctx = EVP_CIPHER_CTX_new();
        if (!ctx->enc_ctx || !ctx->dec_ctx) {
            handleErrors();
        }
        if (EVP_EncryptInit_ex(ctx->enc_ctx, EVP_aes_128_cfb8(), NULL, temp, temp) != 1 ||
            EVP_DecryptInit_ex(ctx->dec_ctx, EVP_aes_128_cfb8(), NULL, temp, temp) != 1) {
            handleErrors();
        }
    } else {
        // --- Client mode: Load server public key ---
        if (pub_der->length <= 0) {
            printf("Invalid public key data length: %i\n", pub_der->length);
            return 0;
        }
        const unsigned char *pub_ptr = (const unsigned char *)pub_der->buffer;
        ctx->rsa_key = d2i_RSA_PUBKEY(NULL, &pub_ptr, pub_der->length);
        if (!ctx->rsa_key) {
            handleErrors();
        }

        // --- If secret provided, init RSA now ---
        if (secret && secret->length >= 16) {
            unsigned char temp[16];
            memcpy(temp, secret->buffer, 16);
            append_to_buffer(ctx->shared_secret, (const char *)temp, sizeof(temp));

            ctx->enc_ctx = EVP_CIPHER_CTX_new();
            ctx->dec_ctx = EVP_CIPHER_CTX_new();
            if (!ctx->enc_ctx || !ctx->dec_ctx) {
                handleErrors();
            }
            if (EVP_EncryptInit_ex(ctx->enc_ctx, EVP_aes_128_cfb8(), NULL, temp, temp) != 1 ||
                EVP_DecryptInit_ex(ctx->dec_ctx, EVP_aes_128_cfb8(), NULL, temp, temp) != 1) {
                handleErrors();
            }
        }
        // Else: client must encrypt its secret with ctx->rsa_key and call again later
    }

    return 1;
}

// Encrypt data (shared secret or verify token) with RSA public key
int RSA_encrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output) {
    if (!ctx->rsa_key || !input || !output) {
        printf("Invalid parameters for RSA_encrypt\n");
        return 0;
    }
    int rsa_size = RSA_size(ctx->rsa_key);
    unsigned char *temp_out = malloc(rsa_size);
    if (!temp_out) {
        printf("Memory allocation failed\n");
        return 0;
    }
    int len = RSA_public_encrypt(input->length, (unsigned char *)input->buffer, temp_out, ctx->rsa_key, RSA_PKCS1_PADDING);
    if (len == -1) {
        free(temp_out);
        handleErrors();
    }

    if (input == output) {free_buffer(output);output = init_buffer();}

    append_to_buffer(output, (const char *)temp_out, len);
    free(temp_out);
    return 1;
}

// Decrypt data (shared secret or verify token) with RSA private key
int RSA_decrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output) {
    if (!ctx->rsa_key || !input || !output) {
        printf("Invalid parameters for RSA_decrypt\n");
        return 0;
    }
    int rsa_size = RSA_size(ctx->rsa_key);
    unsigned char *temp_out = malloc(rsa_size);
    if (!temp_out) {
        printf("Memory allocation failed\n");
        return 0;
    }
    int len = RSA_private_decrypt(input->length, (unsigned char *)input->buffer, temp_out, ctx->rsa_key, RSA_PKCS1_PADDING);
    if (len == -1) {
        free(temp_out);
        handleErrors();
    }

    if (input == output) {free_buffer(output);output = init_buffer();}

    append_to_buffer(output, (const char *)temp_out, len);
    free(temp_out);
    return 1;
}

// Cleanup context and OpenSSL resources
void RSA_cleanup(RSA_CryptoContext *ctx) {
    if (ctx) {
        if (ctx->rsa_key) RSA_free(ctx->rsa_key);
        if (ctx->enc_ctx) EVP_CIPHER_CTX_free(ctx->enc_ctx);
        if (ctx->dec_ctx) EVP_CIPHER_CTX_free(ctx->dec_ctx);
        if (ctx->shared_secret) free_buffer(ctx->shared_secret);
        free(ctx);
    }
    EVP_cleanup();
    ERR_free_strings();
}

// Encrypt a buffer using RSA public key
// Encrypt a buffer using AES-128-CFB8 (Minecraft style)
int AES_encrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output) {
    if (!ctx || !ctx->enc_ctx || !input || !output) return 0;

    int out_len = input->length;
    unsigned char *out_buf = malloc(out_len);
    if (!out_buf) return 0;

    int processed = 0;
    if (EVP_EncryptUpdate(ctx->enc_ctx, out_buf, &processed, (unsigned char *)input->buffer, input->length) != 1) {
        free(out_buf);
        handleErrors();
    }

    if (input == output) {free_buffer(output);output = init_buffer();}

    append_to_buffer(output, (const char *)out_buf, processed);
    free(out_buf);
    return 1;
}

// Decrypt a buffer using AES-128-CFB8 (Minecraft style)
int AES_decrypt(RSA_CryptoContext *ctx, const Buffer *input, Buffer *output) {
    if (!ctx || !ctx->dec_ctx || !input || !output) return 0;

    int out_len = input->length;
    unsigned char *out_buf = malloc(out_len);
    if (!out_buf) return 0;

    int processed = 0;
    if (EVP_DecryptUpdate(ctx->dec_ctx, out_buf, &processed, (unsigned char *)input->buffer, input->length) != 1) {
        free(out_buf);
        handleErrors();
    }

    if (input == output) {free_buffer(output);output = init_buffer();}

    append_to_buffer(output, (const char *)out_buf, processed);
    free(out_buf);
    return 1;
}

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

// 3. Generate a random shared secret and initialize RSA
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

// 2. Store the shared secret in server_ctx and initialize RSA
append_to_buffer(server_ctx->shared_secret, dec_shared->buffer, dec_shared->length);
RSA_setup_aes(server_ctx);  // derive RSA contexts from shared_secret

// ==========================================================
// --- ENCRYPTION/DECRYPTION READY ---
// ==========================================================

// Both sides now have:
//   - server_ctx->enc_ctx / server_ctx->dec_ctx
//   - client_ctx->enc_ctx / client_ctx->dec_ctx
// These can be used for RSA-128-CFB8 packet encryption/decryption.
*/
