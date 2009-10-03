#! /bin/bash

# videocapture is a tool with no special purpose
# 
# Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>


BUILD_CXX="g++-4.4 -std=c++0x"


SCRIPT_DIRECTORY=$(dirname $0)
SOURCES=$(ls $SCRIPT_DIRECTORY | grep -e "\.cpp$")

#additional include paths
INCLUDE="-I$SCRIPT_DIRECTORY/../"

TARGET_DIRECTORY="."

#"build"/"all", "clean"
TARGETS=$*
if [ -z "$TARGETS" ]; then
    TARGETS="build"
fi


for TARGET in $TARGETS;
do

    for SOURCE in $SOURCES;
    do
        SOURCE_WITHOUT_EXTENSION=$(echo "$SOURCE" | sed -e 's/\.cpp//g')
        SOURCE_WITH_PATH=$SCRIPT_DIRECTORY/$SOURCE
        TARGET_WITH_PATH="$TARGET_DIRECTORY/lib$SOURCE_WITHOUT_EXTENSION.so"

        COMMAND=""

        #determine command to be executed
        if [ "$TARGET" == "build" -o "$TARGET" == "all" -o $TARGET == "$SOURCE" ]; then

            if [ "$SOURCE_WITH_PATH" -nt "$TARGET_WITH_PATH" -o $TARGET == "$SOURCE" ]; then

                COMMAND="$BUILD_CXX -shared -fPIC $INCLUDE $SOURCE_WITH_PATH -o $TARGET_WITH_PATH"

            fi

        elif [ "$TARGET" == "clean" ]; then

            COMMAND="rm -f $TARGET_WITH_PATH"

        fi


        #execute
        if [ -n "$COMMAND" ]; then
            echo "$COMMAND"
            eval "$COMMAND"
        fi

    done

done

