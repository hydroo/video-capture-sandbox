TARGET_DIRECTORY=$1"./"

SOURCE_DIRECTORY=`echo $0 | sed 's/\/[^\/]*$//g'`
SOURCES=`ls ${SOURCE_DIRECTORY} | grep -e "\.cpp$" -e "\.c$"`


for SOURCE in ${SOURCES};
do
    SOURCE_WITHOUT_EXTENSION=`echo "${SOURCE}" | sed -e 's/\.cpp//g' -e 's/\.c//g'`
    TARGET_WITH_PATH="${TARGET_DIRECTORY}/lib${SOURCE_WITHOUT_EXTENSION}.so"
    SOURCE_WITH_PATH=${SOURCE_DIRECTORY}/${SOURCE}
    echo "gcc -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}"
    g++ -shared -fPIC ${SOURCE_WITH_PATH} -o ${TARGET_WITH_PATH}
done

