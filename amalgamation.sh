#!/bin/bash -ex

AMALGAMATION_FILE_NAME=amalgamated/redisclient
HEADERS="src/redisclient/version.h \
    src/redisclient/redisbuffer.h \
    src/redisclient/redisvalue.h \
    src/redisclient/redisparser.h \
    src/redisclient/impl/redisclientimpl.h \
    src/redisclient/redisasyncclient.h \
    src/redisclient/redissyncclient.h"


echo > "$AMALGAMATION_FILE_NAME.h"
echo > "$AMALGAMATION_FILE_NAME.cpp"

for file in $HEADERS
do
    cat "$file" >> "$AMALGAMATION_FILE_NAME.h"
done

for file in `find src -name '*.cpp'`
do
    cat "$file" >> "$AMALGAMATION_FILE_NAME.cpp"
done

sed -i 's|# *ifn\?def .*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|# *endif.*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|#include ".*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|#include ".*$||' "$AMALGAMATION_FILE_NAME.cpp"
sed -i 's|REDIS_CLIENT_DECL||' "$AMALGAMATION_FILE_NAME.h"
sed -i "1s/^/#include \"$AMALGAMATION_FILE_NAME.h\"/" "$AMALGAMATION_FILE_NAME.cpp"

