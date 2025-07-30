import requests
import urllib.parse
import json
import time
import os

# 📌 Replace this with your redirect URL after login (only needed the first time)
full_url = "<secret>"

TOKEN_FILE = "microsoft_tokens.json"
CLIENT_ID = "00000000402b5328"
REDIRECT_URI = "https://login.live.com/oauth20_desktop.srf"

def extract_code(redirect_url):
    parsed = urllib.parse.urlparse(redirect_url)
    query = urllib.parse.parse_qs(parsed.query)
    return query.get("code", [None])[0]


def exchange_code_for_token(auth_code):
    data = {
        "client_id": CLIENT_ID,
        "redirect_uri": REDIRECT_URI,
        "grant_type": "authorization_code",
        "code": auth_code
    }
    resp = requests.post("https://login.live.com/oauth20_token.srf", data=data)
    resp.raise_for_status()
    tokens = resp.json()
    save_tokens(tokens)
    return tokens["access_token"], tokens["refresh_token"]


def refresh_access_token(refresh_token):
    data = {
        "client_id": CLIENT_ID,
        "redirect_uri": REDIRECT_URI,
        "grant_type": "refresh_token",
        "refresh_token": refresh_token
    }
    resp = requests.post("https://login.live.com/oauth20_token.srf", data=data)
    resp.raise_for_status()
    tokens = resp.json()
    save_tokens(tokens)
    return tokens["access_token"], tokens["refresh_token"]


def xbox_authenticate(ms_token):
    json_data = {
        "Properties": {
            "AuthMethod": "RPS",
            "SiteName": "user.auth.xboxlive.com",
            "RpsTicket": f"d={ms_token}"
        },
        "RelyingParty": "http://auth.xboxlive.com",
        "TokenType": "JWT"
    }
    resp = requests.post("https://user.auth.xboxlive.com/user/authenticate", json=json_data)
    resp.raise_for_status()
    data = resp.json()
    return data["Token"], data["DisplayClaims"]["xui"][0]["uhs"]


def get_xsts_token(xbl_token):
    json_data = {
        "Properties": {
            "SandboxId": "RETAIL",
            "UserTokens": [xbl_token]
        },
        "RelyingParty": "rp://api.minecraftservices.com/",
        "TokenType": "JWT"
    }
    resp = requests.post("https://xsts.auth.xboxlive.com/xsts/authorize", json=json_data)
    resp.raise_for_status()
    data = resp.json()
    return data["Token"], data["DisplayClaims"]["xui"][0]["uhs"]


def get_minecraft_token(user_hash, xsts_token):
    identity_token = f"XBL3.0 x={user_hash};{xsts_token}"
    resp = requests.post(
        "https://api.minecraftservices.com/authentication/login_with_xbox",
        json={"identityToken": identity_token}
    )
    resp.raise_for_status()
    return resp.json()["access_token"]


def get_minecraft_profile(mc_token):
    headers = {"Authorization": f"Bearer {mc_token}"}
    resp = requests.get("https://api.minecraftservices.com/minecraft/profile", headers=headers)
    resp.raise_for_status()
    return resp.json()


def save_tokens(tokens):
    data = {
        "access_token": tokens["access_token"],
        "refresh_token": tokens.get("refresh_token"),
        "expires_at": time.time() + int(tokens.get("expires_in", 3600))
    }
    with open(TOKEN_FILE, "w") as f:
        json.dump(data, f)


def load_tokens():
    if not os.path.exists(TOKEN_FILE):
        return None
    with open(TOKEN_FILE, "r") as f:
        return json.load(f)


def main():
    tokens = load_tokens()
    if tokens and tokens.get("expires_at", 0) > time.time():
        print("✅ Using saved access token.")
        ms_token = tokens["access_token"]
    elif tokens and "refresh_token" in tokens:
        print("🔁 Refreshing access token...")
        try:
            ms_token, refresh_token = refresh_access_token(tokens["refresh_token"])
        except Exception as e:
            print("❌ Refresh failed, falling back to full login.")
            tokens = None
            ms_token = None
    else:
        print("🔑 Getting token from login URL...")
        code = extract_code(full_url)
        if not code:
            print("❌ Couldn't extract 'code' from URL.")
            return
        ms_token, _ = exchange_code_for_token(code)

    print("🎮 Logging into Xbox Live...")
    xbl_token, user_hash = xbox_authenticate(ms_token)

    print("🔐 Getting XSTS token...")
    xsts_token, user_hash2 = get_xsts_token(xbl_token)

    print("🌍 Logging into Minecraft services...")
    mc_token = get_minecraft_token(user_hash2, xsts_token)

    print("👤 Fetching Minecraft profile...")
    profile = get_minecraft_profile(mc_token)

    print("\n✅ Minecraft Profile:")
    print("Username:", profile["name"])
    print("UUID:    ", profile["id"])


if __name__ == "__main__":
    main()
