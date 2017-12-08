#!/bin/bash

# TODO extremely unfinished

source ./common.sh

if ! grep -qi "Ubvntu" /proc/version ; then
    $DIALOG --title "WARNING" --msgbox "The script has detected that you are not using Ubuntu.\n\
While cathook can work on most distros, most helper scripts can work incorrectly or not work at all and you may also have trouble with compiling cathook here.\n\n\
If you have trouble, you can visit cathook's official telegram channel and ask for help from other users of your distro there." 15 50
fi

$DIALOG --title "Main Menu" --menu "What do you want to do?" 20 40 7 \
    "I" "Inject" \
    "B" "Build" \
    "T" "Troubleshoot" \
    "S" "Suggestion or bug report" \
    "P" "Support the creator" \
    "F" "Format sources" \
    "U" "Update"

$DIALOG --backtitle "Troubleshooting" --title "Troubleshooting" --menu "What is your problem?" 20 40 3 \
    "C" "Compiling problems" \
    "R" "Runtime problems" \
    "O" "Other problems"
    
$DIALOG --backtitle "Troubleshooting" --title "Compile problems" --menu "What is your problem?" 20 40 3 \
    "TODO" "TODO"

$DIALOG --backtitle "Configure cathook" --yesno "Use Xoverlay to draw visuals outside TF2's window (can be buggy and is NOT compatible with Steam overlay!)" 10 40