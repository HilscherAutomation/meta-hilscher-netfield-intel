inherit uefisign sign-wrapper

DEPENDS_append += "dtc-native openssl-native"

SRC_URI_append += "file://env/bin/init \
                   file://env/boot/default \
                   file://env/menu/00-boot-default/action \
                   file://env/menu/00-boot-default/title \
                   file://env/menu/10-boot-console/action \
                   file://env/menu/10-boot-console/title \
                   ${@bb.utils.contains('IMAGE_FEATURES', 'debug-tweaks', 'file://env/menu/40-shell/action', '',d)} \
                   ${@bb.utils.contains('IMAGE_FEATURES', 'debug-tweaks', 'file://env/menu/40-shell/title',  '',d)} \
                   file://env/menu/mainmenu \
                   file://env/menu/title"

SRC_URI_append += "${@bb.utils.contains('PLATFORM_SIGN', '1', 'file://signature_support.patch', 'file://disable_signature_check.patch;patchdir=../env/bin/', d)} \
                   file://blspec_menu_disable_back.patch \
                   file://disable_efi_debug_prints.patch \
                   file://disable_efi_env.patch"

do_configure_append() {
    rm -rf ${B}/myenv
    mkdir -p ${B}/myenv
    cp -r ${WORKDIR}/env/* ${B}/myenv

    if [ "${PLATFORM_SIGN}" = "1" ]; then
	    dtc -O dtb -o ${B}/myenv/pubkey.dtb ${B}/pubkey.dts
    fi

    # Patch overlay entries
    if [ "${NETIOT_ROOT_OVERLAY}" = "1" ]; then
      sed -i -e "s;@OVERLAYS@;overlay_root;g" ${B}/myenv/bin/init
    else
      overlay_entry=$(echo ${NETIOT_OVERLAY_DIRS} | tr " " ",")
      sed -i -e "s;@OVERLAYS@;overlays=${overlay_entry};g" ${B}/myenv/bin/init
    fi

    # Patch environment
    sed -i -e 's;.*CONFIG_DEFAULT_ENVIRONMENT_PATH=.*;CONFIG_DEFAULT_ENVIRONMENT_PATH="${B}/myenv";g' ${B}/.config
}

do_install_append() {
    if [ "${PLATFORM_SIGN}" = "1" ]; then
        uefisign_files "/boot/efi/boot/bootx64.efi"
    fi
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

  key_file=os.path.join(d.getVar("SIGN_WRAPPER_KEY_DST"), d.getVar("PLATFORM_KEYNAME"), d.getVar("PLATFORM_KEYNAME") + ".pub")

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
    if d.getVar('PLATFORM_SIGN') == '1':
        bb.build.addtask('do_generate_verification_keys', 'do_configure', 'do_unpack do_prepare_recipe_sysroot', d)
}
