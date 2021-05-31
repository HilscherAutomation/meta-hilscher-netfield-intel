DEPENDS_append += "pesign-native nss-native"

pesign_files() {
    PESIGN_FILES="${1}"

    rm -rf ${B}/etc/pki/pesign
    mkdir -p ${B}/etc/pki/pesign

    for file_to_sign in ${PESIGN_FILES}; do
        mv ${D}${file_to_sign} ${D}${file_to_sign}.unsigned

        openssl pkcs12 -export -out ${B}/etc/pki/uefi_sign.p12 \
                               -inkey ${KEYS_IMAGE_SIGN_PRIV} \
                               -in ${KEYS_IMAGE_SIGN_CERT} \
                               -nodes -passout pass:
        key_nick=$(pk12util -i ${B}/etc/pki/uefi_sign.p12 -d ${B}/etc/pki/pesign \
                  -W "" | grep "using nickname:" | sed "s/.*using nickname:[ ]*\(.*\)/\1/")
        pesign -i ${D}${file_to_sign}.unsigned -o ${D}${file_to_sign} \
               -c "${key_nick}" \
               -n ${B}/etc/pki/pesign -f -s -v

        rm -f ${D}${file_to_sign}.unsigned
        chown 0:0 ${D}${file_to_sign}
    done
}
