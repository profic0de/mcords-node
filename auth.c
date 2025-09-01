#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include "networking/buffer.h"

#define TOKENS_FILE "tokens.json"
#define URL "https://login.live.com/oauth20_desktop.srf?code=M.C538_BL2.2.U.086de2b4-c8ba-2707-0e1f-84ac88eb037c&lc=1033"

int main() {
    int len, start;
    sscanf(URL, "https://login.live.com/oauth20_desktop.srf?code=%n%*[^&]%n", &start, &len);
    len -= start;
    char *code[len + 1];
    sscanf(URL, "https://login.live.com/oauth20_desktop.srf?code=%[^&]", code);
    printf("Got code from the URL: %s",code);

/*
    def exchange_code_for_token(code):
        data = {
            "client_id": "00000000402b5328",
            "redirect_uri": "https://login.live.com/oauth20_desktop.srf",
            "grant_type": "authorization_code",
            "code": code
        }
        resp = requests.post("https://login.live.com/oauth20_token.srf", data=data)
        resp.raise_for_status()
        tokens = resp.json()
        save_tokens(tokens)
        return tokens["access_token"], tokens["refresh_token"]
*/

    return 0;
}