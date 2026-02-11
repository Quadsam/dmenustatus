# Maintainer: Quadsam <sam@quadsam.com>

pkgname='dmenustatus-git'
pkgver='0.10.1.beta.r0.8166a44'
pkgrel='1'
pkgdesc="A statusbar for dmenu written in C."
arch=('x86_64')
url="https://github.com/Quadsam/dmenu-statusbar"
license=('AGPL-3.0-or-later')
depends=('libx11')
makedepends=('git')
provides=("${pkgname%-git}")
conflicts=("${pkgname%-git}")
#optdepends=('')
source=("${pkgname%-git}::git+${url}")
sha256sums=('SKIP')

pkgver()
{
	cd "$srcdir/${pkgname%-git}"
	printf "%s" "$(git describe --long | sed 's/\([^-]*-\)g/r\1/;s/-/./g')"
}

prepare()
{
	cd "$srcdir/${pkgname%-git}"
}

build()
{
	cd "$srcdir/${pkgname%-git}"
	make
}

package()
{
	cd "$srcdir/${pkgname%-git}"
	make PREFIX="/usr" DESTDIR="$pkgdir/" install
}
