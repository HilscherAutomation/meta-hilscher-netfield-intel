require barebox.inc

SRCREV = "b258642b7db16b6a2a47f958357db597251304c9"

DEPENDS += "lzop-native python3-native"

# ATTENTION: When updating recipe, make sure to recheck if whitelisted CVEs still apply

# Following known CVEs concern NFS, which is not enabled/used on this platform
CVE_CHECK_WHITELIST = "CVE-2019-15937 CVE-2019-15938 CVE-2020-13910"
