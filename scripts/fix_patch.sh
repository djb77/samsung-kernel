#! /usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [[ -z $1 ]]
then
  COMMIT="HEAD"
else
  COMMIT="$1"
fi

FLIST=$(git diff-tree --no-commit-id --name-only -r "$COMMIT")
for f in $FLIST
do
  "$DIR/cleanfile" -width 120 "$f"
  "$DIR/checkpatch.pl" --max-line-length=120 --show-types --ignore  GERRIT_CHANGE_ID,UNDOCUMENTED_DT_STRING,FILE_PATH_CHANGES,NOT_UNIFIED_DIFF,MODIFIED_INCLUDE_ASM --fix-inplace -f "$f"
  if [[ ( $f == *.c ) || ( $f == *.h ) ]]
  then
    chmod 644 "$f"
  fi
done
