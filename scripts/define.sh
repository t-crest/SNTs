#!/bin/bash
# Find the directory of the script
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIRECTORY="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIRECTORY/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIRECTORY="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

# Source the paths file, containing paths to the different tools
source ${DIRECTORY}/paths.sh
if [[ -z "$SNTS_PATH" ]]; then
	echo "Paths was not set correctly, make project again."
	exit;
fi

PROG=SNTs
RUNFOR=300
FULL_PROG="${SNTS_PATH}/${PROG}"
command -v nproc >/dev/null && CORES=$(nproc) || CORES=32

GRAMME=false
if [ "${GRAMME}" = "true" ]
then
	RUNFOR=36000
	FULL_PROG="./dist/Gramme/GNU-Linux-x86/${PROG}"
fi

function SNTsRun {
	while [ $(pgrep SNTs | wc -l) -ge $CORES ] ; do 
		sleep $(( $RANDOM % 5 )).$(( $RANDOM % 1000 ))
	done
	echo "Executing $FULL_PROG $@"	
	$FULL_PROG -a $@ &
}