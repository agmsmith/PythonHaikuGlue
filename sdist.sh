#!/bin/sh

VERSION=0.1.0
DIST=BeOSmodule-${VERSION}-src

rm -rf dist $DIST

mkdir $DIST
cp -r setup.py ?dist.sh docs ext src $DIST
zip -9ry ${DIST}.zip $DIST -x *.svn* *.pyc *.pyo
rm -rf $DIST
