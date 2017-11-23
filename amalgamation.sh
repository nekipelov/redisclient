#!/bin/bash -ex

AMALGAMATION_FILE_NAME=amalgamated/redisclient
HEADERS="src/redisclient/version.h \
    src/redisclient/redisbuffer.h \
    src/redisclient/redisvalue.h \
    src/redisclient/redisparser.h \
    src/redisclient/impl/redisclientimpl.h \
    src/redisclient/impl/throwerror.h \
    src/redisclient/redisasyncclient.h \
    src/redisclient/redissyncclient.h \
    src/redisclient/pipeline.h"


echo > "$AMALGAMATION_FILE_NAME.h"
echo > "$AMALGAMATION_FILE_NAME.cpp"

for file in $HEADERS
do
    cat "$file" >> "$AMALGAMATION_FILE_NAME.h"
done

find src -name '*.cpp' -print0 | sort -z | xargs -r0 cat >> "$AMALGAMATION_FILE_NAME.cpp"

sed -i 's|# *ifn\?def .*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|# *endif.*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|#include ".*$||' "$AMALGAMATION_FILE_NAME.h"
sed -i 's|#include ".*$||' "$AMALGAMATION_FILE_NAME.cpp"
sed -i 's|REDIS_CLIENT_DECL||' "$AMALGAMATION_FILE_NAME.h"
sed -i '1s|^|#include "redisclient.h"|' "$AMALGAMATION_FILE_NAME.cpp"

