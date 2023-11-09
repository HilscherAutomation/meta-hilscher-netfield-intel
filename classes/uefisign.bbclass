inherit sign-wrapper

DEPENDS:append = " sbsigntool-native"

uefisign_files() {
    local SIGN_FILES="${1}"
    local signing_key=$(setup_sign_wrapper_env "${PLATFORM_KEYNAME}")

    sign_wrapper_copy_certificate "${B}/cert" "pem"

    for file_to_sign in ${SIGN_FILES}; do
        mv ${file_to_sign} ${file_to_sign}.unsigned

        case "${SIGN_WRAPPER_MODE}" in
        file)
            sbsign --key "$signing_key" --cert "${B}/cert" ${file_to_sign}.unsigned --output ${file_to_sign}
        ;;

        swtpm)
            sbsign --key "$signing_key" --cert "${B}/cert" --engine tpm2tss ${file_to_sign}.unsigned --output ${file_to_sign}
        ;;

        pkcs11)
            sbsign --key "$signing_key" --cert "${B}/cert" --engine pkcs11 ${file_to_sign}.unsigned --output ${file_to_sign}
        ;;
        esac

        rm -f ${file_to_sign}.unsigned
    done
}
