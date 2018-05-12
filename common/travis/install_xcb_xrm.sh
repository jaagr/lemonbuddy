#!/bin/bash

# Fail on error
set -e

# If the Makefile exists, we have already cached xrm
if [ ! -e "${DEPS_DIR}/xcb-util-xrm/Makefile" ]; then
  git clone --recursive https://github.com/Airblader/xcb-util-xrm
fi

cd xcb-util-xrm

# Install xrm on the system
# If that doesn't work for some reason (not yet compiled, corrupt cache)
# we compile xrm and try to install it again
sudo make install || {
  ./autogen.sh --prefix=/usr --libdir=/usr/lib
  make
  sudo make install
}
