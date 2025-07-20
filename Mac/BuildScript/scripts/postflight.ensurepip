#!/bin/sh
#
# Install/upgrade pip.
#

PYVER="@PYVER@"
PYMAJOR="@PYMAJOR@"
FWK="/Library/Frameworks/Python.framework/Versions/${PYVER}"
RELFWKBIN="../../..${FWK}/bin"

umask 022

"${FWK}/bin/python${PYVER}" -E -s -m ensurepip --upgrade

# bpo-33290: An earlier "pip3 install --upgrade pip" may have installed
#     a "pip" in the fw bin directory.  For a py3 install, remove it.

rm -f "${FWK}/bin/pip"

"${FWK}/bin/python${PYVER}" -E -s -Wi \
    "${FWK}/lib/python${PYVER}/compileall.py" -q -j0 \
    -f -x badsyntax \
    "${FWK}/lib/python${PYVER}/site-packages"

"${FWK}/bin/python${PYVER}" -E -s -Wi -O \
    "${FWK}/lib/python${PYVER}/compileall.py" -q -j0 \
    -f -x badsyntax \
    "${FWK}/lib/python${PYVER}/site-packages"

chgrp -R admin "${FWK}/lib/python${PYVER}/site-packages" "${FWK}/bin"
chmod -R g+w "${FWK}/lib/python${PYVER}/site-packages" "${FWK}/bin"

# We do not know if the user selected the Python command-line tools
# package that installs symlinks to /usr/local/bin.  So we assume
# that the command-line tools package has already completed or was
# not selected and we will only install /usr/local/bin symlinks for
# pip et al if there are /usr/local/bin/python* symlinks to our
# framework bin directory.

if [ -d /usr/local/bin ] ; then
    (
        install_links_if_our_fw() {
            if [ "$(readlink -n ./$1)" = "${RELFWKBIN}/$1" ] ; then
                shift
                for fn ;
                do
                    if [ -e "${RELFWKBIN}/${fn}" ] ; then
                        rm -f ./${fn}
                        ln -s "${RELFWKBIN}/${fn}" "./${fn}"
                        chgrp -h admin "./${fn}"
                        chmod -h g+w "./${fn}"
                    fi
                done
            fi
        }

        cd /usr/local/bin

        # Create pipx.y links if /usr/local/bin/pythonx.y
        #   is linked to this framework version
        install_links_if_our_fw "python${PYVER}" \
                                    "pip${PYVER}"

        # Create pipx link if /usr/local/bin/pythonx is linked to this version
        install_links_if_our_fw "python${PYMAJOR}" \
                                    "pip${PYMAJOR}"

        # Create pip link if /usr/local/bin/python
        #   is linked to this version
        install_links_if_our_fw "python" \
                                    "pip"
    )
fi
exit 0
