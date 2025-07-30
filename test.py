import uuid
import hashlib

def offline_player_uuid(username: str) -> uuid.UUID:
    """
    Generates the same UUID as the Minecraft client does for offline players.
    This is a UUID v3 generated from 'OfflinePlayer:<username>'.
    """
    name = "OfflinePlayer:" + username
    hash = hashlib.md5(name.encode("utf-8")).digest()
    # Set UUID version and variant manually
    hash = bytearray(hash)
    hash[6] = (hash[6] & 0x0F) | 0x30  # Version 3
    hash[8] = (hash[8] & 0x3F) | 0x80  # Variant RFC 4122
    return uuid.UUID(bytes=bytes(hash))

print(offline_player_uuid("Proficode"))
# Output: 6ceeb44d-5f71-3ebf-8412-9d320fc0b0cf
#         6ceeb44d-5f71-3ebf-8412-9d320fc0b0cf
