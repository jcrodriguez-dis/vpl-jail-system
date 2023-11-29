#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2023 Juan Carlos Rodriguez-del-Pino jc.rodriguezdelpino@ulpgc.es
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.txt
# Description:  Script to run tests for vpl-jail-system

CHECK_MARK="✅"
X_MARK="❌"
ERRORS_LOG_FILE=".errors.log"
PLAIN_PORT=8080
SECURE_PORT=9000

function writeHeading {
	echo -e "\e[33m RUNNING \e[0m \e[34m$1\e[0m"
}
function writeInfo {
	echo -e $3 "\e[33m$1\e[0m$2"
}
function writeError {
    echo
	echo -e "\e[31m$1\e[0m$2"
}

function writeCorrect {
	echo -e " \e[32m$1\e[0m$2"
}

function write {
	echo -e "$1"
}

export -f writeHeading
export -f writeInfo
export -f writeError
export -f write

function removeImageContainer() {
    docker rm "$2" & > /dev/null
    docker rmi "$1" & > /dev/null
    docker volume rm "$3" & > /dev/null
}

function chekPortInUse() {
    local n=1
    while true ; do
        docker ps --filter publish=$1 | grep $1 &> /dev/null
        if [ "$?" == "0" ] ; then
            if [ n -gt 10 ] ; then
                echo
                writeError "The following container is using a port need for the tests" " Please stop it"
                docker ps --filter publish=$1
                exit 1
            fi
        else
            return
        fi
        sleep 1
        echo -n "*"
        ((n++))
    done
    echo
}

function checkDockerInstall() {
    chekPortInUse $PLAIN_PORT
    chekPortInUse $SECURE_PORT
    export VPL_BASE_DISTRO=$1
    export VPL_INSTALL_LEVEL=$2
    export IMAGE_NAME=jail-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    export CONTAINER_NAME=c-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    export VOLUMEN_NAME=ssl-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    local CLEAN="removeImageContainer \"$IMAGE_NAME\" \"$CONTAINER_NAME\" \"$VOLUMEN_NAME\""

    # Build image
    writeInfo "Creating image"
    docker build \
        --build-arg VPL_BASE_DISTRO=$VPL_BASE_DISTRO \
        --build-arg VPL_INSTALL_LEVEL=$VPL_INSTALL_LEVEL \
        --progress=plain -t $IMAGE_NAME . &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Build $IMAGE_NAME fail"
        return 1
    fi
    docker image ls $IMAGE_NAME
    writeCorrect "Image $IMAGE_NAME created" $CHECK_MARK

    # Run container
    docker volume create $VOLUMEN_NAME &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Error creating volume $VOLUMEN_NAME."
        $CLEAN
        return 2
    fi

    export CONTAINER_NAME=check-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    docker run --name $CONTAINER_NAME \
               --privileged -d \
               --mount type=volume,target=/etc/vpl/ssl,src="$VOLUMEN_NAME" \
               -p $PLAIN_PORT:80 -p $SECURE_PORT:443 \
               -d $IMAGE_NAME &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Container $CONTAINER_NAME run fail."
        $CLEAN
        return 2
    fi
    docker container ls | head -1
    docker container ls | grep $CONTAINER_NAME
    writeCorrect "Container $CONTAINER_NAME on $IMAGE_NAME image runnng" $CHECK_MARK

    # Test container
    URL="http://localhost:$PLAIN_PORT/nada"
    wget $URL &> $ERRORS_LOG_FILE
    if [[ $? == 0 ]] ; then
        rm nada &>/dev/null
        writeError "Container $CONTAINER_NAME answer to incorrect URL: $URL"
        $CLEAN
        return 3
    fi
    writeCorrect "Correct response for bad URL $URL" $CHECK_MARK
    URL="http://localhost:$PLAIN_PORT/OK"
    wget $URL &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Container $CONTAINER_NAME fail for correct URL: $URL"
        $CLEAN
        return 4
    fi
    rm OK &>/dev/null
    writeCorrect "Correct response for OK URL $URL" $CHECK_MARK

    # Stop container
    docker stop -t 3 $CONTAINER_NAME &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Error stopping $CONTAINER_NAME"
        $CLEAN
        return 5
    fi

    if [[ "$2" != "" ]] ; then
        return
    fi
    # Remove container
    docker container rm -f $CONTAINER_NAME &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Error removing container $CONTAINER_NAME"
        $CLEAN
        return 6
    fi

    # Remove volumen
    docker volume rm $VOLUMEN_NAME &> $ERRORS_LOG_FILE
    if [[ $? != 0 ]] ; then
        writeError "Error removing volumen $VOLUMEN_NAME"
        $CLEANE
        return 8
    fi

    # Remove image
    if [ "$1" = "" ] ; then
        docker rmi $IMAGE_NAME &> $ERRORS_LOG_FILE
        if [[ $? != 0 ]] ; then
            writeError "Error removing image $IMAGE_NAME"
            $CLEANE
            return 9
        fi
    fi
}

function runTests() {
    local n=1
	local DISTROS=( alpine ubuntu debian fedora )
    if [ "$1" == "full" ] ; then
	    local INSTALL_LEVELS=( minimum basic standard full )
        shift
    elif [ "$1" == "standard" ] ; then
	    local INSTALL_LEVELS=( minimum basic standard )
        shift
    elif [ "$1" == "basic" ] ; then
	    local INSTALL_LEVELS=( minimum basic)
        shift
    else
	    local INSTALL_LEVELS=( minimum )
    fi
    for VPL_INSTALL_LEVEL in "${INSTALL_LEVELS[@]}"
    do
        for VPL_BASE_DISTRO in "${DISTROS[@]}"
        do
            SECONDS=0
            writeInfo "Test $n: " "Testing vpl-jail-system in $VPL_BASE_DISTRO install $VPL_INSTALL_LEVEL"
            checkDockerInstall $VPL_BASE_DISTRO $VPL_INSTALL_LEVEL $1
            if [ "$?" != "0" ] ; then
                writeInfo "Logs:"
                cat $ERRORS_LOG_FILE
            fi
            ((n=n+1))
            ELT=$SECONDS
            writeInfo "Test took " "$(($ELT / 60)) minutes and $(($ELT % 60)) seconds"
        done
    done
}

writeHeading "Testing install vpl-jail-system in Docker $1"
runTests $1 $2
