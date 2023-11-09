SRC_URI:append = " file://bluetooth_more_csr_quirks.patch"

# ATTENTION: When updating recipe, make sure to recheck if whitelisted CVEs still apply
# CVE-2019-18814 Code path is not included in 4.14
CVE_CHECK_IGNORE:append = " CVE-2019-18814"
# Following are already patched upstream
CVE_CHECK_IGNORE:append = " CVE-2019-18805 CVE-2019-17133 CVE-2019-16746 CVE-2019-15926 CVE-2019-15505 \
                               CVE-2019-15504 CVE-2019-15292 CVE-2019-10126 CVE-2018-20784"
