#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2023 Juan Carlos Rodriguez-del-Pino jc.rodriguezdelpino@ulpgc.es
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.txt
# Description:  Script to run tests for vpl-jail-system

CHECK_MARK="\u2713";
X_MARK="\u274C";
function writeHeading {
	echo -e "\e[33m RUNNING \e[0m \e[34m$1\e[0m"
}
function writeInfo {
	echo -e $3 "\e[33m$1\e[0m$2"
}
function writeError {
	echo -e "\e[31m$1\e[0m$2"
}

function writeCorrect {
	echo -e "\e[32m$1\e[0m$2"
}

function write {
	echo -e "$1"
}

export -f writeHeading
export -f writeInfo
export -f writeError
export -f write

function removeImageContainer() {
    docker rm $CONTAINER_NAME & > /dev/null
    docker rmi $IMAGE_NAME & > /dev/null
}

function checkDockerInstall() {
    export VPL_BASE_DISTRO=$1
    export VPL_INSTALL_LEVEL=$2
    export IMAGE_NAME=test-vpl-jail-system-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    # Build image
    docker build \
        --build-arg VPL_BASE_DISTRO \
        --build-arg VPL_INSTALL_LEVEL \
        --progress=plain -t $IMAGE_NAME . &> erros.log
    if [[ $? != 0 ]] ; then
        writeError "Build $IMAGE_NAME fail"
        return 1
    fi
    # Run container
    export CONTAINER_NAME=prueba-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    docker run --name $CONTAINER_NAME --privileged -d -p 8080:80 -p 9000:443 -d $IMAGE_NAME &> erros.log
    if [[ $? == 0 ]] ; then
        writeError "Container $CONTAINER_NAME answer to incorrect URL: $URL"
        removeImageContainer
        return 2
    fi
    # Test container
    URL=http://localhost:8080/nada
    wget $URL &> erros.log
    if [[ $? == 0 ]] ; then
        rm nada &>/dev/null
        writeError "Container $CONTAINER_NAME answer to incorrect URL: $URL"
        removeImageContainer
        return 3
    fi
    URL=http://localhost:8080/OK
    wget $URL &> erros.log
    if [[ $? != 0 ]] ; then
        writeError "Container $CONTAINER_NAME fail for correct URL: $URL"
        removeImageContainer
        return 4
    fi
    rm OK &>/dev/null
    docker stop $CONTAINER_NAME &> erros.log
    if [[ $? != 0 ]] ; then
        writeError "Error stopping $CONTAINER_NAME"
        removeImageContainer
        return 5
    fi
    # Remove container
    docker rm $CONTAINER_NAME &> erros.log
    if [[ $? != 0 ]] ; then
        writeError "Error removing $CONTAINER_NAME"
        removeImageContainer
        return 6
    fi
    # remove build
    docker rmi $IMAGE_NAME &> erros.log
    if [[ $? != 0 ]] ; then
        writeError "Error removing $IMAGE_NAME"
        removeImageContainer
        return 7
    fi
}

function runTests() {
    local n=1
	local DISTROS=( ubuntu debian fedora )
    if [ "$1" == "full" ] ; then
	    local INSTALL_LEVELS=( minimum basic standard full )
    else
	    local INSTALL_LEVELS=( minimum )
    fi
    for VPL_INSTALL_LEVEL in "${INSTALL_LEVELS[@]}"
    do
        for VPL_BASE_DISTRO in "${DISTROS[@]}"
        do
            writeInfo "Test $n: " "Testing vpl-jail-system in $VPL_BASE_DISTRO install $VPL_INSTALL_LEVEL" -n
            checkDockerInstall $VPL_BASE_DISTRO $VPL_INSTALL_LEVEL
            if [ "$?" == "0" ] ; then
                writeCorrect "$CHECK_MARK"
                cat error.log
            else
                writeError "Errors found" " $X_MARK"
            fi
        done
    done
}

writeHeading "Testing install vpl-jail-system in Docker $1"
runTests $1
