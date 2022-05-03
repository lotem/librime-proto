#!/bin/bash

if [[ "$OSTYPE" =~ 'darwin' ]]; then
    git submodule update --init
    make -f deps.mk
fi
