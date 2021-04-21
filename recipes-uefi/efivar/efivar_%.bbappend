do_install_append_class-native () {
    oe_runmake install DESTDIR=${D}
}
