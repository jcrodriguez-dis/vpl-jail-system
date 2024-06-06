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
    if [[ $1 -ne 0 ]] ; then
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

function checkDockerComposeUp() {
    local PROJECT_NAME=$1
    local PRIVILEGED=$2
    chekPortInUse $PLAIN_PORT
    chekPortInUse $SECURE_PORT

    docker compose up -d 2>&1 | tee $ERRORS_LOG_FILE | show_progress
    showMessageIfError $? "Docker compose up of '$PROJECT_NAME' $PRIVILEGED fail"
    [[ $? -ne 0 ]] && return 1
    docker compose ls

    # Test project
    result=1
    URL="http://localhost:$PLAIN_PORT/nada"
    wget -t 3 $URL &>> $ERRORS_LOG_FILE
    [[ $? -ne 0 ]] && result=0
    rm nada &>/dev/null
    showMessageIfError $result "Project '$PROJECT_NAME' $PRIVILEGED answer to incorrect URL: $URL"
    [[ $result -ne 0 ]] && return 3
    writeCorrect "Correct response for bad URL $URL" $CHECK_MARK

    URL="http://localhost:$PLAIN_PORT/OK"
    wget -t 3 $URL &>> $ERRORS_LOG_FILE
    local result=$?
    rm OK &>/dev/null
    showMessageIfError $result "Project '$PROJECT_NAME' $PRIVILEGED fail for correct URL: $URL"
    [[ $result -ne 0 ]] && return 4
    writeCorrect "Correct response for OK URL $URL" $CHECK_MARK
    writeInfo "Service '$PROJECT_NAME' logs"
    docker compose logs vpl-jail
    # Stop container
    docker compose down 2> $ERRORS_LOG_FILE
    showMessageIfError $? "Error in compose down of '$PROJECT_NAME' $PRIVILEGED"
    [[ $? -ne 0 ]] && return 5

    # Remove container
    docker compose rm 2> $ERRORS_LOG_FILE
    showMessageIfError $? "Error removing container '$PROJECT_NAME' $PRIVILEGED"
    [[ $? -ne 0 ]] && return 6
    return 0
}

function checkDockerCompose() { 
    export VPL_BASE_DISTRO=$1
    export VPL_INSTALL_LEVEL=$2
    export VPL_EXPOSE_PORT=$PLAIN_PORT
    export VPL_EXPOSE_SECURE_PORT=$SECURE_PORT
    local PROJECT_NAME="jail-$1-$2"
    # Build compose
    export VPL_RUN_PRIVILEGED=false
    writeInfo "Creating project '$PROJECT_NAME'"
    docker compose build --no-cache 2>&1 | tee $ERRORS_LOG_FILE | show_progress
    showMessageIfError $? "Docker compose build of '$PROJECT_NAME' fail"
    [[ $? -ne 0 ]] && return 1

    # Run project
    checkDockerComposeUp "$PROJECT_NAME"

    # Build compose
    export VPL_RUN_PRIVILEGED=true
    writeInfo "Creating project '$PROJECT_NAME' privileged"
    docker compose build --no-cache 2>&1 | tee $ERRORS_LOG_FILE | show_progress
    showMessageIfError $? "Docker compose build of '$PROJECT_NAME' privileged fail"
    [[ $? -ne 0 ]] && return 1

    # Run project
    checkDockerComposeUp "$PROJECT_NAME" "privileged"
}

function runTests() {
    # Parameters:
    #    $1 Ditro name [Optional]
    #    $2 Install level [Optional]
    local n=0
	local DISTROS=( alpine ubuntu debian fedora )
    local INSTALL_LEVELS=( minimum basic standard full )
    local AUX=
    if [ "$1" != "" ] ; then
        for AUX in "${DISTROS[@]}"; do
            if [[ "$AUX" == "$1" ]]; then
                DISTROS=( $AUX )
                shift
                break
            fi
        done
    fi
    if [ "$1" != "" ] ; then
        for AUX in "${INSTALL_LEVELS[@]}"; do
            if [[ "$AUX" == "$1" ]]; then
                INSTALL_LEVELS=( $AUX )
                shift
                break
            fi
        done
    fi
    # Build distribution package
    rm vpl-jail-system-*.tar.gz &> /dev/null
    writeInfo "Test Matrix [ ${DISTROS[@]} ] X [ ${INSTALL_LEVELS[@]} ]"
    writeInfo "Building distribution package"
    (
        autoreconf -i
        ./configure
        make distcheck
     ) >> $ERRORS_LOG_FILE
    if [ $? -ne 0 ] ; then
        writeError "Distribution build fails. See '$ERRORS_LOG_FILE' file"
        exit 1
    fi
    [ -f ./config.h ] && VERSION=$(grep -E "PACKAGE_VERSION" ./config.h | sed -e "s/[^\"]\+\"\([^\"]\+\).*/\1/")
    PACKAGE="vpl-jail-system-$VERSION"
    TARPACKAGE="$PACKAGE.tar.gz"
    if [ ! -s $TARPACKAGE ] ; then 
        writeError "Package file not found"
        exit 1
    fi

    # Use distribution package
    writeInfo "Unpacking distribution $VERSION"
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
            checkDockerCompose $VPL_BASE_DISTRO $VPL_INSTALL_LEVEL
            [ $? -ne 0 ] && ((nfails++))
            ELT=$SECONDS
            writeInfo "Test took " "$(($ELT / 60)) minutes and $(($ELT % 60)) seconds"
        done
    done
    if [ $nfails = 0 ] ; then
        writeInfo "$CHECK_MARK All $n tests passed"
    else
        writeError "$X_MARK $nfails of $n tests failed"
    fi
    cd ..
    rm -R -f $PACKAGE
    rm $TARPACKAGE
}

writeHeading "Testing vpl-jail-system running in Docker with compose $1 $2"
runTests $1 $2
