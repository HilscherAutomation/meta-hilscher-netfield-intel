inherit sign-wrapper

DEPENDS_append += "sbsigntool-native"

uefisign_files() {
    local SIGN_FILES="${1}"
    local signing_key=$(setup_sign_wrapper_env "${PLATFORM_KEYNAME}")

    sign_wrapper_copy_certificate "${B}/cert" "pem"

    for file_to_sign in ${SIGN_FILES}; do
        mv ${D}${file_to_sign} ${D}${file_to_sign}.unsigned

        case "${SIGN_WRAPPER_MODE}" in
        file)
            sbsign --key "$signing_key" --cert "${B}/cert" ${D}${file_to_sign}.unsigned --output ${D}${file_to_sign}
        ;;

        swtpm)
            sbsign --key "$signing_key" --cert "${B}/cert" --engine tpm2tss ${D}${file_to_sign}.unsigned --output ${D}${file_to_sign}
        ;;

        pkcs11)
            sbsign --key "$signing_key" --cert "${B}/cert" --engine pkcs11 ${D}${file_to_sign}.unsigned --output ${D}${file_to_sign}
        ;;
        esac

        rm -f ${D}${file_to_sign}.unsigned
        chown 0:0 ${D}${file_to_sign}
    done
}
