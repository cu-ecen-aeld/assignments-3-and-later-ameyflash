#! /bin/sh

echo "Checking for arguments..."
if [ $# -ne 1 ]; then
    echo "Error : Arguments not specified correctly!!!"
    echo "Usage: $ ./aesdsocket-start-stop.sh <start/stop>"
    exit 1
else
    echo "Pass"
fi

case "$1" in
	start)
		echo "Start AESD socket application"
		start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket
		# local --> -x /home/amey/AESD/assignment-5-part-1-ameyflash/server/aesdsocket -- -d
		# buildroot --> -a /usr/bin/aesdsocket
		;;
	stop)
		echo "Stop AESD socket application"
		start-stop-daemon -K -n aesdsocket
		;;
	*)
		echo "Usage: $ ./aesdsocket-start-stop.sh <start/stop>"
		exit 1
esac

exit 0	
