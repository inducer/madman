#! /bin/bash

set -e

VERSION=`cat current-version`
PACKAGE_DIR="madman-$VERSION"

function cp_keep_layout()
{
  DEST="$1"
  shift
  for i in "$@"; do
    DESTSUBDIR="`dirname "$i"`"
    mkdir -p "$DEST/$DESTSUBDIR"
    cp "$i" "$DEST/$DESTSUBDIR"
  done
}

function findsource()
{
  find expat "$@"
  find utility "$@"
  find database "$@"
  find httpd "$@"
  find ui "$@"
  find main "$@"
}

rm -Rf $PACKAGE_DIR
mkdir $PACKAGE_DIR
cp_keep_layout $PACKAGE_DIR `tla inventory --source`
chmod u=rwX,go=rX -R "$PACKAGE_DIR"
tar cvfz "$PACKAGE_DIR.tar.gz" "$PACKAGE_DIR"
rm -Rf $PACKAGE_DIR
