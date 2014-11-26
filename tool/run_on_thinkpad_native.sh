#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Usage: run_on_toughpad.sh buiddir run_name"
fi

BUILDDIR=$1
RUN_NAME=$2

sudo cp $BUILDDIR/var/run/$RUN_NAME/hypervisor /
sudo cp $BUILDDIR/var/run/$RUN_NAME/genode/* /genode/

chmod +x $BUILDDIR/var/run/$RUN_NAME/40_custom
sudo cp $BUILDDIR/var/run/$RUN_NAME/40_custom /etc/grub.d/40_custom

sudo update-grub