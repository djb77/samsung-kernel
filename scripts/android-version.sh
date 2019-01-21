#!/bin/sh

MAJOR=$(echo $1 | cut -d '.' -f 1)
MINOR=$(echo $1 | cut -d '.' -f 2)
PATCH=$(echo $1 | cut -d '.' -f 3)
if echo $MAJOR | egrep -q '^[0-9]+$'; then
  if [ "x$PATCH" != "x" ] ; then
    printf "%d%02d%02d\\n" $MAJOR $MINOR $PATCH
  else
    printf "%d%02d00\\n" $MAJOR $MINOR
  fi
else
  MAJOR=$(echo "$(($(printf '%d\n' "'$MAJOR")-71))")
  MINOR=0
  printf "%d%02d00\\n" $MAJOR $MINOR
fi
