#!/usr/bin/env bash
# Selective initialization of submodules
# Originally written by Marko Kosunen, marko.kosunen@aalto.fi, 2017,
# modified by Santeri Porrasmaa

DIR=`pwd`

SUBMODULES="\
    ./src/components/esp-wifi-logger/ \
    ./src/components/AIP31068L/ \
    ./src/components/TCA6408A/ \
"
git submodule sync
for mod in $SUBMODULES; do 
    git submodule update --init $mod
    cd ${mod}
    if [ -f ./init_submodules.sh ]; then
        ./init_submodules.sh
    fi
    cd ${DIR}

done
exit 0

