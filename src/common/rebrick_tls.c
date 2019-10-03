#include "rebrick_tls.h"



//struct rebrick_tls_checkitem_list_t *tls_after_io_checklist = NULL;
//struct rebrick_tls_checkitem_list_t *tls_before_io_checklist = NULL;
//multi init protector
static int32_t tls_init_finished = 0;

static void after_io(uv_check_t *check)
{
    char current_time_str[32] = {0};
    unused(current_time_str);
    if (check && check->data)
    {
        rebrick_tls_checkitem_list_t *checklist = cast(check->data, rebrick_tls_checkitem_list_t *);
        rebrick_tls_checkitem_t *tmp;
        DL_FOREACH(checklist->head, tmp)
        {

            tmp->func(tmp->socket);
        }
    }
}

int32_t rebrick_after_io_list_add(rebrick_tls_checkitem_func func,struct rebrick_tlssocket *socket)
{

    char current_time_str[32] = {0};
    unused(current_time_str);
    rebrick_tls_checkitem_t *tmp;

    int32_t founded = 0;

    //burası hash table yapılsa daha hızlı çalışır
    DL_FOREACH(tls_after_io_checklist->head, tmp)
    {
        if (tmp->socket == socket)
        {
            founded = 1;
            break;
        }
    }
    if (!founded)
    {
        rebrick_tls_checkitem_t *item = new (rebrick_tls_checkitem_t);
        constructor(item, rebrick_tls_checkitem_t);
        item->socket = socket;
        item->func=func;
        DL_APPEND(tls_after_io_checklist->head, item);
    }

    return REBRICK_SUCCESS;
}

int32_t rebrick_after_io_list_remove(struct rebrick_tlssocket *socket)
{

    char current_time_str[32] = {0};
    unused(current_time_str);
    rebrick_tls_checkitem_t *tmp, *el;
    if(tls_after_io_checklist)
    DL_FOREACH_SAFE(tls_after_io_checklist->head, el, tmp)
    {
        if (el->socket == socket){
            DL_DELETE(tls_after_io_checklist->head, el);
            free(el);
        }
    }

    return REBRICK_SUCCESS;
}

int32_t rebrick_before_io_list_add(rebrick_tls_checkitem_func func,struct rebrick_tlssocket *socket)
{
    char current_time_str[32] = {0};
    unused(current_time_str);
    rebrick_tls_checkitem_t *tmp;

    int32_t founded = 0;

    DL_FOREACH(tls_before_io_checklist->head, tmp)
    {
        if (tmp->socket == socket)
        {
            founded = 1;
            break;
        }
    }
    if (!founded)
    {
        rebrick_tls_checkitem_t *item = new (rebrick_tls_checkitem_t);
        constructor(item, rebrick_tls_checkitem_t);
        item->socket = socket;
        item->func=func;
        DL_APPEND(tls_before_io_checklist->head, item);
    }

    return REBRICK_SUCCESS;
}



int32_t rebrick_before_io_list_remove(struct rebrick_tlssocket *socket)
{

    char current_time_str[32] = {0};
    unused(current_time_str);
    rebrick_tls_checkitem_t *tmp, *el;
    if(tls_before_io_checklist)
    DL_FOREACH_SAFE(tls_before_io_checklist->head, el, tmp)
    {
        if (el->socket == socket){
            DL_DELETE(tls_before_io_checklist->head, el);
            free(el);
        }
    }

    return REBRICK_SUCCESS;
}

static uv_check_t check;

extern int32_t rebrick_tlssocket_change_context(struct rebrick_tlssocket *socket,const char *servername);

static int ssl_servername_cb(SSL *s, int *ad, void *arg)
{
    unused(s);
    unused(ad);
    unused(arg);
    char current_time_str[32] = {0};
    unused(current_time_str);
    int32_t result;
    rebrick_tls_context_t *context = cast(arg,rebrick_tls_context_t*);
    if(!context){
        rebrick_log_fatal("sni cannot find, context is null\n");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    rebrick_tls_ssl_t *el,*tmp;
    DL_FOREACH_SAFE(context->sni_pending_list,el,tmp){
        if(el->ssl==s){

            const char *servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);

            if(servername || strlen(servername)){

                struct rebrick_tlssocket *tlssocket=cast(el->ref,struct rebrick_tlssocket*);
                result=rebrick_tlssocket_change_context(tlssocket,servername);
                if(result<0)
                return  SSL_TLSEXT_ERR_ALERT_FATAL;


            }

            DL_DELETE(context->sni_pending_list,el);
            break;




        }

    }


   /*  if (servername != NULL && p->biodebug != NULL) {
        const char *cp = servername;
        unsigned char uc;

        BIO_printf(p->biodebug, "Hostname in TLS extension: \"");
        while ((uc = *cp++) != 0)
            BIO_printf(p->biodebug,
                       isascii(uc) && isprint(uc) ? "%c" : "\\x%02x", uc);
        BIO_printf(p->biodebug, "\"\n");
    }

    if (p->servername == NULL)
        return SSL_TLSEXT_ERR_NOACK;

    if (servername != NULL) {
        if (strcasecmp(servername, p->servername))
            return p->extension_error;
        if (ctx2 != NULL) {
            BIO_printf(p->biodebug, "Switching server context.\n");
            SSL_set_SSL_CTX(s, ctx2);
        }
    } */
    return SSL_TLSEXT_ERR_OK;
}

int32_t rebrick_tls_init()
{
    if (!tls_init_finished)
    {
        char current_time_str[32] = {0};
        int32_t result;
        unused(current_time_str);
        OPENSSL_init_ssl(0,NULL);
        OpenSSL_add_all_digests();
        SSL_load_error_strings();
        ERR_load_crypto_strings();




        //create a new context for SNI(server name indication)
        rebrick_tls_context_t *context_sni;
        result=rebrick_tls_context_new(&context_sni,REBRICK_TLS_CONTEXT_SNI,SSL_VERIFY_NONE, SSL_SESS_CACHE_BOTH, SSL_OP_ALL,0, REBRICK_TLS_SNI_FAKE_CERT_PRV_FILE, REBRICK_TLS_SNI_FAKE_CERT_PRV_FILE);
        if(result<0){
        return result;
        }

        SSL_CTX_set_tlsext_servername_callback(context_sni->tls_ctx, ssl_servername_cb);
        SSL_CTX_set_tlsext_servername_arg(context_sni->tls_ctx,context_sni);

        //after io part
        tls_after_io_checklist=new(rebrick_tls_checkitem_list_t);
        constructor(tls_after_io_checklist,rebrick_tls_checkitem_list_t);

         tls_before_io_checklist=new(rebrick_tls_checkitem_list_t);
        constructor(tls_before_io_checklist,rebrick_tls_checkitem_list_t);

        uv_check_init(uv_default_loop(), &check);
        check.data = tls_after_io_checklist;
        uv_check_start(&check, after_io);



        tls_init_finished = 1;
    }

    return REBRICK_SUCCESS;
}



int32_t rebrick_tls_cleanup(){
    if(tls_init_finished){
        rebrick_tls_context_t *context_sni=NULL;
        rebrick_tls_context_get(REBRICK_TLS_CONTEXT_SNI,&context_sni);
        if(context_sni)
        rebrick_tls_context_destroy(context_sni);

        //OPENSSL_cleanup();

        EVP_cleanup();
        ENGINE_cleanup();
        CONF_modules_unload(1);
EVP_cleanup();
CRYPTO_cleanup_all_ex_data();
//ERR_remove_state(uv_os_getpid());
ERR_free_strings();

        if(tls_after_io_checklist)
        free(tls_after_io_checklist);
        if(tls_before_io_checklist)
        free(tls_before_io_checklist);

        tls_after_io_checklist=NULL;
        tls_before_io_checklist=NULL;

        uv_check_stop(&check);

        uv_close(cast(&check,uv_handle_t*),NULL);

        tls_init_finished=0;
    }
    return REBRICK_SUCCESS;
}

struct rebrick_tls_context_hashitem
{
    base_object();
    char key[REBRICK_TLS_KEY_LEN];
    rebrick_tls_context_t *ctx;
    UT_hash_handle hh;
};




struct rebrick_tls_context_hashitem *ctx_map = NULL;

int32_t rebrick_tls_context_new(rebrick_tls_context_t **context, const char *key, int32_t ssl_verify, int32_t session_mode, int32_t options,int32_t clearoptions,  const char *certificate_file, const char *private_file)
{
    char current_time_str[32] = {0};
    unused(current_time_str);

    struct rebrick_tls_context_hashitem *out;
    //find in hash map
    HASH_FIND_STR(ctx_map, key, out);

    if (out)
    {
        *context = out->ctx;
        return REBRICK_SUCCESS;
    }

    rebrick_tls_context_t *ctx = new (rebrick_tls_context_t);
    constructor(ctx, rebrick_tls_context_t);

    ctx->tls_ctx = SSL_CTX_new(TLS_method());

    if (!ctx->tls_ctx)
    {

        rebrick_log_fatal("ssl init failed\n");
        free(ctx);
        return REBRICK_ERR_TLS_INIT;
    }


    //only load not fake files
    //because every server context must have a cert_file path
    //we will copy file name but we will not load cert files
    //also private keys
    if (certificate_file && strcmp(certificate_file,REBRICK_TLS_SNI_FAKE_CERT_PRV_FILE) && SSL_CTX_use_certificate_file(ctx->tls_ctx, certificate_file, SSL_FILETYPE_PEM) <= 0)
    {
        rebrick_log_fatal("ssl cerfiticate file %s loading failed\n", certificate_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx->tls_ctx);
        free(ctx);
        return REBRICK_ERR_TLS_INIT;
    }
    if(certificate_file)
    snprintf(ctx->cert_file,REBRICK_TLS_FILE_MAX_LEN,"%s",certificate_file);

    if (private_file && strcmp(private_file,REBRICK_TLS_SNI_FAKE_CERT_PRV_FILE)  && SSL_CTX_use_PrivateKey_file(ctx->tls_ctx, private_file, SSL_FILETYPE_PEM) <= 0)
    {
        rebrick_log_fatal("ssl private file %s loading failed\n", private_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx->tls_ctx);
        free(ctx);
        return REBRICK_ERR_TLS_INIT;
    }

    if(private_file && strcmp(private_file,REBRICK_TLS_SNI_FAKE_CERT_PRV_FILE) &&   !SSL_CTX_check_private_key(ctx->tls_ctx)){
         rebrick_log_fatal("ssl private file %s loading failed\n", private_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx->tls_ctx);
        free(ctx);
        return REBRICK_ERR_TLS_INIT;
    }
    if(private_file)
    snprintf(ctx->prv_file,REBRICK_TLS_FILE_MAX_LEN,"%s",private_file);

    strncpy(ctx->key, key, REBRICK_TLS_KEY_LEN - 1);
    SSL_CTX_set_verify(ctx->tls_ctx, ssl_verify, NULL);
    SSL_CTX_set_options(ctx->tls_ctx, options);
    SSL_CTX_set_session_cache_mode(ctx->tls_ctx, session_mode);



    if(clearoptions)
    SSL_CTX_clear_options(ctx->tls_ctx, clearoptions);




    struct rebrick_tls_context_hashitem *hash;
    hash = new (struct rebrick_tls_context_hashitem);
    constructor(hash, struct rebrick_tls_context_hashitem);
    hash->ctx = ctx;
    strncpy(hash->key, ctx->key, REBRICK_TLS_KEY_LEN - 1);

    HASH_ADD_STR(ctx_map, key, hash);
    rebrick_log_debug("%s ssl context created\n", key);

    *context = ctx;
    return REBRICK_SUCCESS;
}


int32_t rebrick_tls_context_destroy(rebrick_tls_context_t *context)
{
    if (context)
    {
        //clear the sni pending list
         rebrick_tls_ssl_t *el,*tmp;
        DL_FOREACH_SAFE(context->sni_pending_list,el,tmp){
            DL_DELETE(context->sni_pending_list,el);
        }

        if (context->tls_ctx)
        {
            //remove from hash map
            struct rebrick_tls_context_hashitem *out;
            HASH_FIND_STR(ctx_map, context->key, out);
            if (out)
            {
                HASH_DEL(ctx_map, out);
                free(out);
            }
            //then dispose ctx
            SSL_CTX_free(context->tls_ctx);
        }
        free(context);
    }
    return REBRICK_SUCCESS;
}

int32_t rebrick_tls_context_get(const char *key, rebrick_tls_context_t **context)
{
    char current_time_str[32] = {0};
    unused(current_time_str);

    struct rebrick_tls_context_hashitem *out;
    //find in hash map
    HASH_FIND_STR(ctx_map, key, out);

    if (out)
    {
        rebrick_log_debug("%s ssl context found\n", key);
        *context = out->ctx;
    }
    else
    {
        rebrick_log_debug("%s ssl context not found\n", key);
        *context = NULL;
    }
    return REBRICK_SUCCESS;
}

int32_t rebrick_tls_ssl_new(rebrick_tls_ssl_t **ssl, const rebrick_tls_context_t *context)
{
    char current_time_str[32] = {0};
    unused(current_time_str);
    if (!context || !context->tls_ctx)
    {
        return REBRICK_ERR_BAD_ARGUMENT;
    }

    SSL *tmp = SSL_new(context->tls_ctx);
    if (!ssl)
    {

        rebrick_log_fatal("new ssl with key %s failed\n", context->key);
        ERR_print_errors_fp(stderr);

        return REBRICK_ERR_TLS_NEW;
    }
    if (rebrick_tls_context_is_server(context))
        SSL_set_accept_state(tmp);
    else
        SSL_set_connect_state(tmp);
    BIO *read = BIO_new(BIO_s_mem());
    if (!read)
    {

        rebrick_log_fatal("new bio read with key %s failed\n", context->key);

        SSL_free(tmp);
        return REBRICK_ERR_TLS_ERR;
    }

    BIO *write = BIO_new(BIO_s_mem());
    if (!write)
    {
        rebrick_log_fatal("new bio write with key %s failed\n", context->key);
        BIO_free(read);
        SSL_free(tmp);
        return REBRICK_ERR_TLS_ERR;
    }

    BIO_set_nbio(read, 1);
    BIO_set_nbio(write, 1);

    rebrick_tls_ssl_t *state = new (rebrick_tls_ssl_t);
    constructor(state, rebrick_tls_ssl_t);
    state->ssl = tmp;
    state->read = read;
    state->write = write;
    SSL_set_bio(tmp, read, write);
    *ssl = state;
    return REBRICK_SUCCESS;
}


int32_t rebrick_tls_ssl_new2(rebrick_tls_ssl_t **ssl,const char *server_indication_name){
    char current_time_str[32] = {0};
    unused(current_time_str);
    unused(server_indication_name);
    rebrick_tls_context_t *context;

    //get context for SNI
    int32_t result=rebrick_tls_context_get(REBRICK_TLS_CONTEXT_SNI,&context);
    if(result<0){
        rebrick_log_fatal("sni context not found\n");
        return REBRICK_ERR_BAD_ARGUMENT;
    }
    rebrick_tls_ssl_t *state;
    result=rebrick_tls_ssl_new(&state,context);
    if(result<0)
    return result;

    //copy sni name for finding real tls context
    //snprintf(state->sni_pattern,REBRICK_TLS_SNI_MAX_LEN-1,"%s",server_indication_name);
    //add to SNI_CONTEXT pending list
    //this is import
    DL_APPEND(context->sni_pending_list,state);

    *ssl = state;
    return REBRICK_SUCCESS;
}

int32_t rebrick_tls_ssl_destroy(rebrick_tls_ssl_t *tls)
{
    if (tls)
    {
        if (tls->ssl)
        {

            SSL_clear(tls->ssl);
            SSL_free(tls->ssl);

        }
        free(tls);
    }
    return REBRICK_SUCCESS;
}