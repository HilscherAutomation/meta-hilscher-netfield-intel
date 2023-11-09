do_install:append:class-native () {
    oe_runmake install DESTDIR=${D}
}
