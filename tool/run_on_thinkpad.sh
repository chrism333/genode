#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Usage: run_on_toughpad.sh buiddir run_name"
fi

BUILDDIR=$1
RUN_NAME=$2

scp $BUILDDIR/var/run/$RUN_NAME/hypervisor root@thinkpad:/
scp $BUILDDIR/var/run/$RUN_NAME/genode/* root@thinkpad:/genode/

chmod +x $BUILDDIR/var/run/$RUN_NAME/40_custom
scp $BUILDDIR/var/run/$RUN_NAME/40_custom root@thinkpad:/etc/grub.d/40_custom

ssh root@thinkpad "update-grub && grub-reboot 2 && reboot || true" || true

