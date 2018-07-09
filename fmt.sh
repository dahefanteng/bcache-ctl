#!/bin/bash
indent -kr -i 8 bcache-ctl.c
indent -kr -i 8 lib.c
indent -kr -i 8 lib.h
rm -rf bcache-ctl.c~
rm -rf lib.c~
rm -rf lib.h~
