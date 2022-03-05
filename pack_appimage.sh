#!/bin/sh

mkdir -p AppDir/usr/bin/
cp build/bin/gah AppDir/usr/bin/
wget -O linuxdeployqt https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt
./linuxdeployqt AppDir/usr/share/applications/gah.desktop -appimage -verbose=2
