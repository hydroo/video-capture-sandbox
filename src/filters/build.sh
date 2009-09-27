#! /bin/bash

SCRIPT_DIRECTORY=`echo $0 | sed 's/\/[^\/]*$//g'`
SOURCES=`ls ${SCRIPT_DIRECTORY} | grep -e "\.cpp$" -e "\.c$"`

TARGET=$1
#compile only one file if a target has been specified
if [ "$TARGET" != "" ]; then
    SOURCES=${TARGET}
fi

TARGET_DIRECTORY=$2
if [ "$TARGET_DIRECTORY" == "" ]; then
    TARGET_DIRECTORY="."
fi


for SOURCE in ${SOURCES};
do
    EXTENSION=`echo "$SOURCE" | sed 's/.*\.cpp/cpp/g' | sed 's/.*\.c/c/g'`
    SOURCE_WITHOUT_EXTENSION=`echo "${SOURCE}" | sed -e 's/\.cpp//g' -e 's/\.c//g'`
    TARGET_WITH_PATH="${TARGET_DIRECTORY}/lib${SOURCE_WITHOUT_EXTENSION}.so"
    SOURCE_WITH_PATH=${SCRIPT_DIRECTORY}/${SOURCE}

    if [ "$EXTENSION" == "cpp" ]; then
        echo "g++ -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}"
        g++ -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}
    elif [ "$EXTENSION" == "c" ]; then
        echo "gcc -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}"
        gcc -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}
    else
        echo "unrecognized extenstion $EXTENSION $SOURCE"
    fi
done

