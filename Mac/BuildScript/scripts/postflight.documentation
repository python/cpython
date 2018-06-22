#!/bin/sh

PYVER="@PYVER@"
FWK="/Library/Frameworks/Python.framework/Versions/${PYVER}"
FWK_DOCDIR_SUBPATH="Resources/English.lproj/Documentation"
FWK_DOCDIR="${FWK}/${FWK_DOCDIR_SUBPATH}"
APPDIR="/Applications/Python ${PYVER}"
SHARE_DIR="${FWK}/share"
SHARE_DOCDIR="${SHARE_DIR}/doc/python${PYVER}"
SHARE_DOCDIR_TO_FWK="../../.."

# make link in /Applications/Python m.n/ for Finder users
if [ -d "${APPDIR}" ]; then
    ln -fhs "${FWK_DOCDIR}/index.html" "${APPDIR}/Python Documentation.html"
    open "${APPDIR}" || true  # open the applications folder
fi

# make share/doc link in framework for command line users
if [ -d "${SHARE_DIR}" ]; then
    mkdir -m 775 -p "${SHARE_DOCDIR}"
    # make relative link to html doc directory
    ln -fhs "${SHARE_DOCDIR_TO_FWK}/${FWK_DOCDIR_SUBPATH}" "${SHARE_DOCDIR}/html"
fi

