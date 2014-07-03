#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Usage: run_on_toughpad.sh buiddir run_name"
fi

BUILDDIR=$1
RUN_NAME=$2

scp $BUILDDIR/var/run/$RUN_NAME/hypervisor root@fzm1k:/
scp $BUILDDIR/var/run/$RUN_NAME/genode/* root@fzm1k:/genode/

chmod +x $BUILDDIR/var/run/$RUN_NAME/40_custom
scp $BUILDDIR/var/run/$RUN_NAME/40_custom root@fzm1k:/etc/grub.d/40_custom

ssh root@fzm1k "update-grub && grub-reboot 2 && reboot || true" || true

