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
  find libapetag "$@"
  find expat "$@"
  find utility "$@"
  find database "$@"
  find httpd "$@"
  find ui "$@"
  find main "$@"
}

rm -Rf $PACKAGE_DIR
mkdir $PACKAGE_DIR
cp -R \
  README CODING_STYLE COPYING \
  *.ts \
  expatconfig.h \
  configure current-version \
  scons-*.tar.gz \
  $PACKAGE_DIR

cp_keep_layout $PACKAGE_DIR `findsource -name "*.h"` 
cp_keep_layout $PACKAGE_DIR `findsource -name "*.c"` 
cp_keep_layout $PACKAGE_DIR `findsource -name "*.cpp"` 
cp_keep_layout $PACKAGE_DIR `find -name "*.ui"` 
cp_keep_layout $PACKAGE_DIR `find -name "*.png"` 
cp_keep_layout $PACKAGE_DIR `find plugins -type f -name "[a-z]*" -o -name README` 
cp_keep_layout $PACKAGE_DIR `find -name SConscript -o -name SConstruct` 
cp httpd/make_webdata "$PACKAGE_DIR/httpd"
cp designer/make_imagedata "$PACKAGE_DIR/designer"
mkdir "$PACKAGE_DIR/httpd/webdata"
cp httpd/webdata/*.* "$PACKAGE_DIR/httpd/webdata"
chmod u=rwX,go=rX -R "$PACKAGE_DIR"
tar cvfz "$PACKAGE_DIR.tar.gz" "$PACKAGE_DIR"
rm -Rf $PACKAGE_DIR
