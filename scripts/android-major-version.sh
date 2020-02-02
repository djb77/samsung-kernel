#!/bin/sh

MAJOR=$(echo $1 | cut -d '.' -f 1)
if echo $MAJOR | egrep -q '^[0-9]+$'; then
  let MAJOR=MAJOR+103
  printf "%b" "$(printf '\%03o' $MAJOR)"
else
  echo $MAJOR | tr '[A-Z]' '[a-z]'
fi
