#!/bin/sh
mkdir -p uefi

cd uefi
for keytype in pk kek db; do
    openssl req -new -x509 -newkey rsa:4096 -keyout $keytype.key -out $keytype.crt -days 10950 -nodes -sha256
    openssl x509 -outform der -in $keytype.crt -out $keytype.crt.der
    cert-to-efi-sig-list $keytype.crt $keytype.esl
done

sign-efi-sig-list -k pk.key -c pk.crt PK pk.esl pk.auth
sign-efi-sig-list -k pk.key -c pk.crt KEK kek.esl kek.auth
sign-efi-sig-list -k kek.key -c kek.crt $(uuidgen) db.esl db.auth

rm *.esl

cd ..

