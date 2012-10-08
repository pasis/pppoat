# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

EGIT_REPO_URI="git://github.com/pasis/pppoat.git"

inherit git-2

DESCRIPTION="PPP over Any Transport"
HOMEPAGE="https://github.com/pasis/pppoat"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS=""
IUSE="+xmpp"

RDEPEND="xmpp? ( dev-libs/libstrophe )"
DEPEND="${RDEPEND}"

S="${WORKDIR}/${P/-/_}"