# Maintainer: YuutaW <i@yuuta.moe>
pkgname=twtfwd-git
pkgver=0.0.0.0
pkgrel=1
pkgdesc="Personal Twitter to Telegram bot"
arch=("any")
url="https://github.com/Trumeet/twtfwd"
license=('GPL2')
depends=("jq"
		"coreutils"
		"bash")
source=('twtfwd'
		'twtfwd.conf'
		'twtfwd.timer'
		'twtfwd.service')
md5sums=('281e2b71474531287f8210060fad1b49'
         'c4d15aa857664308e3455b7597bc0b28'
         '8ee49826056dee908b52339e15e23889'
         '6fcdb0d2461af86190ddbe1ca308bd56')
backup=('etc/twtfwd.conf')

pkgver() {
	git rev-list --count master
}

package() {
    install -m755 -d "${pkgdir}/usr/bin/"
	install -m755 -d "${pkgdir}/usr/lib/systemd/system"
    install -m755 -d "${pkgdir}/var/lib/twtfwd/"
    install -m755 -d "${pkgdir}/etc"
    install -m755 "${srcdir}/twtfwd" "${pkgdir}/usr/bin/"
	install -m600 "${srcdir}/twtfwd.conf" "${pkgdir}/etc/"
    install -m644 "${srcdir}/twtfwd.service" "${pkgdir}/usr/lib/systemd/system/"
    install -m644 "${srcdir}/twtfwd.timer" "${pkgdir}/usr/lib/systemd/system/"
}
