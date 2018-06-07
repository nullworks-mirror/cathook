#!/usr/bin/env bash

# Thank you LWSS
# https://github.com/LWSS/Fuzion/commit/a53b6c634cde0ed47b08dd587ba40a3806adf3fe

line=$(pidof hl2_linux)
arr=($line)
inst=$1
if [ $# == 0 ]; then
  inst=0
fi

if [ ${#arr[@]} == 0 ]; then
  echo TF2 isn\'t running!
  exit
fi

if [ $inst -gt ${#arr[@]} ] || [ $inst == ${#arr[@]} ]; then
  echo wrong index!
  exit
fi

proc=${arr[$inst]}

echo Running instances: "${arr[@]}"
echo Attaching to "$proc"

# pBypass for crash dumps being sent
# You may also want to consider using -nobreakpad in your launch options.
sudo rm -rf /tmp/dumps # Remove if it exists
sudo mkdir /tmp/dumps # Make it as root
sudo chmod 000 /tmp/dumps # No permissions

# Get a Random name from the build_names file.
FILENAME=$(shuf -n 1 build_names)

# Create directory if it doesn't exist
if [ ! -d "/usr/lib64" ]; then
  sudo mkdir /usr/lib64
fi

# In case this file exists, get another one. ( checked it works )
while [ -f "/usr/lib64/${FILENAME}" ]; do
  FILENAME=$(shuf -n 1 build_names)
done

# echo $FILENAME > build_id # For detaching

cp "bin/libcathook.so" "/usr/lib64/${FILENAME}"

echo loading "$FILENAME" to "$proc"

sudo killall -19 steam
sudo killall -19 steamwebhelper

gdb -n -q -batch \
  -ex "attach $proc" \
  -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
  -ex "call \$dlopen(\"/usr/lib64/$FILENAME\", 1)" \
  -ex "call dlerror()" \
  -ex 'print (char *) $2' \
  -ex "catch syscall exit exit_group" \
  -ex "detach" \
  -ex "quit"

sudo rm "/usr/lib64/${FILENAME}"

sudo killall -18 steamwebhelper
sudo killall -18 steam
