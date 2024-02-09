inherit uefisign sign-wrapper

DEPENDS:append = " dtc-native openssl-native"

SRC_URI:append = " \
    file://env/bin/init \
    file://env/boot/default \
    file://scripts/boot-recovery.scr \
"

SRC_URI:append = " \
    file://0001-Add-command-to-verify-an-RSA-signature-for-files.patch \
    file://0002-EFI-disable-debug-prints.patch                         \
    file://0003-EFI-Disable-default-environment.patch                  \
    file://0004-Add-command-to-load-variables-from-file-boot.cfg.patch \
"

do_configure:append() {
    rm -rf ${B}/myenv
    mkdir -p ${B}/myenv
    cp -r ${WORKDIR}/env/* ${B}/myenv
    sed -i -e 's;@KERNEL_CMDLINE@;${KERNEL_CMDLINE};g' ${B}/myenv/bin/init

    if [ "${@bb.utils.contains('IMAGE_FEATURES', 'debug-tweaks', 'true', 'false',d)}" = "true" ]; then
        cat <<EOF>${B}/myenv/bin/debug
#!/bin/sh

mkdir -p /tmp/menu/shell
echo -o /tmp/menu/shell/title "Enter shell"
echo -o /tmp/menu/shell/action "#!/bin/sh"
echo -a /tmp/menu/shell/action "sh"
let "numentries" "numentries++"
EOF

    fi

    dtc -O dtb -o ${B}/myenv/pubkey.dtb ${B}/pubkey.dts

    # Patch environment
    sed -i -e 's;.*CONFIG_DEFAULT_ENVIRONMENT_PATH=.*;CONFIG_DEFAULT_ENVIRONMENT_PATH="${B}/myenv";g' ${B}/.config
}

do_compile:append() {
    uefisign_files ${B}/barebox.efi
}

python do_generate_verification_keys() {
  import subprocess
  import sys
  import re

  # http://www.algorithmist.com/index.php/Modular_inverse
  def recursive_egcd(a, b):
      """Returns a triple (g, x, y), such that ax + by = g = gcd(a,b).
         Assumes a, b >= 0, and that at least one of them is > 0.
         Bounds on output values: |x|, |y| <= max(a, b)."""
      if a == 0:
          return (b, 0, 1)
      else:
          g, y, x = recursive_egcd(b % a, a)
          return (g, x - (b // a) * y, y)

  def modinv(a, m):
      g, x, y = recursive_egcd(a, m)
      if g != 1:
          return None
      else:
          return x % m

  key_file = os.path.join(d.getVar("SIGN_WRAPPER_KEY_DST", True), 
                          d.getVar("PLATFORM_KEYNAME", True),
                          d.getVar("PLATFORM_KEYNAME", True) + '.pub')

  # Extract modulus and N0inv
  modulus = subprocess.check_output(["openssl", "rsa", "-pubin", "-in", key_file, "-modulus", "-noout"])
  modulus = re.sub(b'Modulus=', b'', modulus).rstrip()
  N       = int(modulus, 16)
  keylen  = N.bit_length()

  B = 0x100000000
  N0inv = B - modinv(N, B)

  # Extract public exponent (defaults to 0x100001)
  exponent = subprocess.check_output(["openssl", "rsa", "-pubin", "-in", key_file, "-text", "-noout"])
  exponent = int(re.search(b'Exponent: ([0-9]+)', exponent).group(1))

  str_modulus=modulus.decode("utf-8").lower()
  dtc_modulus=["0x" + str_modulus[i:i+8] for i in range(0, len(str_modulus), 8)]
  dtc_modulus=" ".join(dtc_modulus)

  R = pow(2, keylen)
  RR = pow(R, 2, int(modulus, 16))
  str_RR=hex(RR)[2:]
  rr=["0x" + str_RR[i:i+8] for i in range(0, len(str_RR), 8)]
  rr=" ".join(rr)

  signature_block="""
/dts-v1/;
/ {
    signature {
        key-sig {
            required = "conf";
            rsa,modulus = <%s>;
            rsa,exponent = <0x00000000 0x%08x>;
            rsa,n0-inverse = <0x%08x>;
            rsa,num-bits = <0x%08x>;
            rsa,r-squared = <%s>;
        };
    };
};
""" % ( dtc_modulus, int(exponent),
        N0inv, keylen, rr)

  with open(os.path.join(d.getVar("B", True), "pubkey.dts"), 'w+') as sig_check:
    sig_check.write(signature_block)
}

python() {
    bb.build.addtask('do_generate_verification_keys', 'do_configure', 'do_unpack do_prepare_recipe_sysroot', d)
}

do_deploy:append() {
    install -d ${DEPLOYDIR}/boot-files
    install -m 0644 ${B}/barebox.efi ${DEPLOYDIR}/boot-files/bootx64.efi

    local signing_key=$(setup_sign_wrapper_env "${PLATFORM_KEYNAME}")

    install ${WORKDIR}/scripts/boot-recovery.scr ${DEPLOYDIR}/boot-files/
    openssl_sign_wrapper "${signing_key}" "sha512" "${DEPLOYDIR}/boot-files/boot-recovery.scr"
}

inherit hilscher-deploy

hd_path = "${HDEPLOY_PATH_EXTRAS}/boot-scripts"

do_hilscher_deploy() {
    cp -r ${DEPLOYDIR}/boot-files/* "${hd_path}/"
}
do_hilscher_deploy[cleandirs] = "${hd_path}/"
addtask hilscher_deploy before do_build after do_deploy
