#!/bin/sh

VERSION=0.1.0
DIST=BeOSmodule-${VERSION}-Zeta-R1.1

rm -rf dist $DIST

python setup.py bdist

rst2html.py docs/beospackage.txt dist/beospackage.html
cp -r docs/art dist

mv dist $DIST
zip -9ry ${DIST}.zip $DIST -x *.svn* *.pyc *.pyo

rm -rf $DIST dist build
