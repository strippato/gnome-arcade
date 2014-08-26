#!/bin/sh

LOCALPATH=$PWD

echo
echo This script will download free roms from http://mamedev.org/roms/
echo
echo Thanks to the generosity of some of the original creators of the classic games that MAME® can emulate,
echo several games have been released for free, non-commercial use.
echo It is our hope that in the future, we will be able to add more games to this list.
echo
echo Note: The ROMs on these pages have been approved for free distribution on this site only.
echo Just because they are available here for download does not entitle you to put them on your own site,
echo include them with your own distributions of MAME, or bundle them with your software, cabinet, or other item.
echo To do that, you must obtain permission from the original owners.
echo
echo

ROMS=" \
roms/alienar/alienar.zip \
roms/carpolo/carpolo.zip \
roms/circus/circus.zip \
roms/crash/crash.zip \
roms/fax2/fax2.zip \
roms/fax/fax.zip \
roms/fireone/fireone.zip \
roms/gridlee/gridlee.zip \
roms/hardhat/hardhat.zip \
roms/looping/looping.zip \
roms/ripcord/ripcord.zip \
roms/robby/robby.zip \
roms/robotbwl/robotbwl.zip \
roms/sidetrac/sidetrac.zip \
roms/spectar/spectar.zip \
roms/starfir2/starfir2.zip \
roms/starfire/starfire.zip \
roms/supertnk/supertnk.zip \
roms/targ/targ.zip \
roms/teetert/teetert.zip \
roms/topgunnr/topgunnr.zip \
roms/victorba/victorba.zip \
roms/victory/victory.zip \
roms/wrally/wrally.zip \
"

SERVER=" \
mamedev.org    \
"

CURL_OPTIONS="-L -s --speed-limit 3500 --speed-time 15"

if [ ! -f /usr/bin/id ]; then
 echo "Running in non-chrooted (install into directory) mode... Exit safely."
 exit 0
fi

if [ "`id -u`" != "0" ]; then
 ROM_DEST=$LOCALPATH/"../data/rom/"
else
 ROM_DEST="/usr/share/gnome-arcade/data/rom/"
fi
echo "your rom will be installed in $ROM_DEST"

if [ -f /etc/sysconfig/proxy ] ; then
   . /etc/sysconfig/proxy
fi

if test "x$PROXY_ENABLED" != "xno"; then
  if test -n "$HTTP_PROXY" ; then
    export http_proxy="$HTTP_PROXY"
  fi
fi

if [ -z $http_proxy ]; then
  echo
  echo "note: No proxy is used. Please set the environment variable \"http_proxy\""
  echo "note: to your favorite proxy, if you want to use a proxy for the download."
  echo "note:"
  echo "note:   bash: export http_proxy=\"http://proxy.example.com:3128/\""
  echo "note:   tcsh: setenv http_proxy \"http://proxy.example.com:3128/\""
  echo
fi

tmpname=`basename $0`
tmpdir=`mktemp -d /tmp/$tmpname.XXXXXX`
trap "rm -rf $tmpdir" EXIT
if [ $? -ne 0 ]; then
  echo "$0: Can't create temp dir, exiting..."
  exit 4
fi

pushd $tmpdir &> /dev/null

echo

for rom in $ROMS; do
 for i in $SERVER; do
  url=http://$i/$rom
  file=`echo $url|awk -F "/" '{print $NF}'`
  rm -f $file
  echo -n "Fetching $file from $url ... "
  curl $CURL_OPTIONS -o $file $url
  if [ $? -ne 0 ]; then
     rm -f $file
     echo "failed ... deleted!"
     continue
  fi
  echo done
 done
done

for i in *.zip; do
  lower=`echo $i|tr [:upper:] [:lower:]`
  test "$i" != "$lower" && mv $i $lower
done

chmod 644 *.zip


mv -f *.zip $ROM_DEST
echo
echo "*** roms installed in $ROM_DEST ***"

popd &> /dev/null
