#!/bin/sh

set -e

export VENDOR=lge
export DEVICE=f300
./../../$VENDOR/g2-common/extract-files.sh $@
