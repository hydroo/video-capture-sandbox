#! /bin/bash

SCRIPT_DIRECTORY=$(echo "$0" | sed 's/\/[^\/]*$//g')
SOURCES=$(ls $SCRIPT_DIRECTORY | grep -e "\.cpp$" -e "\.c$")

TARGET_DIRECTORY=$1
if [ "$TARGET_DIRECTORY" == "" ]; then
    TARGET_DIRECTORY="."
fi


for SOURCE in $SOURCES;
do
    SOURCE_WITHOUT_EXTENSION=$(echo "$SOURCE" | sed -e 's/\.cpp//g' -e 's/\.c//g')
    SOURCE_WITH_PATH=$SCRIPT_DIRECTORY/$SOURCE
    TARGET_WITH_PATH="$TARGET_DIRECTORY/lib$SOURCE_WITHOUT_EXTENSION.so"


    SOURCE_MODIFICATION_TIME="$(stat -c "%Y" $SOURCE_WITH_PATH)"
    #trick: that 0 makes not finding the file work
    TARGET_MODIFICATION_TIME="0$(stat -c "%Y" $TARGET_WITH_PATH 2>/dev/null)"

    if [ "$SOURCE_MODIFICATION_TIME" -ge "$TARGET_MODIFICATION_TIME" ]; then

        EXTENSION=$(echo "$SOURCE" | sed 's/.*\.cpp/cpp/g' | sed 's/.*\.c/c/g')

        if [ "$EXTENSION" == "cpp" ]; then CC="g++"
        elif [ "$EXTENSION" == "c" ]; then CC="gcc"
        else exit -1
        fi

        COMMAND="$CC -shared -fPIC $SOURCE_WITH_PATH -o $TARGET_WITH_PATH"

        echo "$COMMAND"
        eval "$COMMAND"

    fi
done

