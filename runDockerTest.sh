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
}

function show_progress() {
    local linea
    local num_columns=$COLUMNS
    [ -z "$num_columns" ] && num_columns=80
    ((num_columns--))
    local clean="printf \"\r%${num_columns}s\r\" "
    while read -r linea ; do
        linea=${linea:0:$num_columns}
        $clean
        printf "\r%s" "$linea"
    done
    $clean
}

function chekPortInUse() {
    local n=1
    while true ; do
        docker ps --filter publish=$1 | grep $1 &> /dev/null
        if [ "$?" == "0" ] ; then
            if [ $n -gt 10 ] ; then
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

function showMessageIfError() {
    if [[ $1 != 0 ]] ; then
        writeError "$2"
        writeInfo "Logs:"
        if [ -f "$ERRORS_LOG_FILE" ] ; then
            cat "$ERRORS_LOG_FILE"
            rm "$ERRORS_LOG_FILE"
        else
            echo "NO LOG FILE FOUND"
        fi
    fi
    return $1
}

function checkDockerRunImage() {
    local CONTAINER_NAME=$1
    local PRIVILEGED=$2
    local RUNOPTION="-e VPL_JAIL_JAILPATH=/"
    local result=

    [ "$PRIVILEGED" = "privileged" ] && RUNOPTION="--privileged"
    chekPortInUse $PLAIN_PORT
    chekPortInUse $SECURE_PORT
    docker run --name $CONTAINER_NAME \
               $RUNOPTION \
               -d \
               -p $PLAIN_PORT:80 -p $SECURE_PORT:443 \
               -d $IMAGE_NAME &>> $ERRORS_LOG_FILE
    showMessageIfError $? "Container $CONTAINER_NAME run fail."
    [[ $? != 0 ]] && return 2

    docker container ls | head -1
    docker container ls | grep $CONTAINER_NAME
    writeCorrect "Container $CONTAINER_NAME on $IMAGE_NAME image running" $CHECK_MARK

    # Test container
    result=1
    URL="http://localhost:$PLAIN_PORT/nada"
    wget $URL &>> $ERRORS_LOG_FILE
    [[ $? != 0 ]] && result=0
    rm nada &>/dev/null
    showMessageIfError $result "Container $CONTAINER_NAME answer to incorrect URL: $URL"
    [[ $result != 0 ]] && return 3
    writeCorrect "Correct response for bad URL $URL" $CHECK_MARK

    URL="http://localhost:$PLAIN_PORT/OK"
    wget $URL &>> $ERRORS_LOG_FILE
    local result=$?
    rm OK &>/dev/null
    showMessageIfError $result "Container $CONTAINER_NAME fail for correct URL: $URL"
    [[ $result != 0 ]] && return 4
    writeCorrect "Correct response for OK URL $URL" $CHECK_MARK

    # Stop container
    docker stop -t 3 $CONTAINER_NAME &>> $ERRORS_LOG_FILE
    showMessageIfError $? "Error stopping $CONTAINER_NAME"
    [[ $? != 0 ]] && return 5

    # Remove container
    docker container rm -f $CONTAINER_NAME &>> $ERRORS_LOG_FILE
    showMessageIfError $? "Error removing container $CONTAINER_NAME"
    [[ $? != 0 ]] && return 6
}

function checkDockerInstall() { 
    export VPL_BASE_DISTRO=$1
    export VPL_INSTALL_LEVEL=$2
    export IMAGE_NAME=jail-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    export CONTAINER_NAME=check-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    export VOLUMEN_NAME=ssl-$VPL_BASE_DISTRO-$VPL_INSTALL_LEVEL
    local CLEAN="removeImageContainer \"$IMAGE_NAME\" \"$CONTAINER_NAME\" \"$VOLUMEN_NAME\""

    # Build image
    writeInfo "Creating image"
    docker build \
        --build-arg VPL_BASE_DISTRO=$VPL_BASE_DISTRO \
        --build-arg VPL_INSTALL_LEVEL=$VPL_INSTALL_LEVEL \
        --progress=plain -t $IMAGE_NAME . 2>&1 | tee $ERRORS_LOG_FILE | show_progress
    showMessageIfError $? "Build $IMAGE_NAME fail"
    [[ $? != 0 ]] && return 1

    docker image ls $IMAGE_NAME
    writeCorrect "Image $IMAGE_NAME created" $CHECK_MARK

    # Run container in privileged and non-privileged mode
    checkDockerRunImage "$CONTAINER_NAME" "noprivileged"
    checkDockerRunImage "$CONTAINER_NAME-privileged" "privileged"

    # Remove image
    if [ "$3" = "" ] ; then
        docker rmi $IMAGE_NAME &> $ERRORS_LOG_FILE
        showMessageIfError $? "Error removing image $IMAGE_NAME"
        [[ $? != 0 ]] && return 9
    fi
}

function runTests() {
    local n=0
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
    rm vpl-jail-system-*.tar.gz &> /dev/null
    echo "Building distribution package"
    make distcheck &> /dev/null
    [ $? != 0 ] && (echo "Distribution build fails" ; exit 1)
    [ -f ./config.h ] && VERSION=$(grep -E "PACKAGE_VERSION" ./config.h | sed -e "s/[^\"]\+\"\([^\"]\+\).*/\1/")
    PACKAGE="vpl-jail-system-$VERSION"
    TARPACKAGE="$PACKAGE.tar.gz"
    [ ! -s $TARPACKAGE ] && (echo "Package file not found" ; exit 1)
    echo "Unpacking distribution $VERSION"
    tar xvf "$PACKAGE.tar.gz" > /dev/null
    cd $PACKAGE
    local nfails=0
    for VPL_INSTALL_LEVEL in "${INSTALL_LEVELS[@]}"
    do
        for VPL_BASE_DISTRO in "${DISTROS[@]}"
        do
            SECONDS=0
            ((n=n+1))
            writeInfo "Test $n: " "Testing vpl-jail-system in $VPL_BASE_DISTRO install $VPL_INSTALL_LEVEL"
            checkDockerInstall $VPL_BASE_DISTRO $VPL_INSTALL_LEVEL $1
            [ "$?" != "0" ] && ((nfails++))
            ELT=$SECONDS
            writeInfo "Test took " "$(($ELT / 60)) minutes and $(($ELT % 60)) seconds"
        done
    done
    if [ $nfails = 0 ] ; then
        writeInfo "$CHECK_MARK All $n tests passed"
    else
        writeInfo "$X_MARK $nfails of $n tests failed"
    fi
    cd ..
    rm -R $PACKAGE
    rm $TARPACKAGE
}

writeHeading "Testing install vpl-jail-system in Docker $1"
runTests $1 $2
