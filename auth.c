#include "networking/requests.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    http_init();

    char post_fields[512];
    const char *redirect_uri = "https%3A%2F%2Flogin.live.com%2Foauth20_desktop.srf"; 
    const char *code = "M.C538_BL2.2.U.dba59003-3e1b-00d9-64de-c2f880bc161c"; // hardcoded

    snprintf(post_fields, sizeof(post_fields), "client_id=00000000402b5328&redirect_uri=%s&grant_type=authorization_code&code=%s",redirect_uri, code);

    HttpResponse *res = http_post(
        "https://login.live.com/oauth20_token.srf",
        post_fields,
        "application/x-www-form-urlencoded"
    );

    while (!res->done) {
        http_perform();
        usleep(100 * 1000);
    }

    printf("Response: %s\n", res->buf->buffer);

    http_cleanup();
    return 0;
}
