#!/bin/bash

pkgname="md2roff"
pkgver="1.8.0"
pkgdesc="convert markdown files into to roff format."
arch=("x86_64")
license=("GPLv3")
source=("${pkgname}::git+https://github.com/pnmlinux/${pkgname}")
makedepends=("gcc" "make")

build() {
    cd "${srcdir}/${pkgname}"
    make
}

package() {
    cd "${srcdir}"
    make install
}