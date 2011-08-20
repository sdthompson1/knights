#!/bin/sh

# Script to automatically start the Knights Server if it is not
# currently running.

# To use, add something like this to your crontab (crontab -e):

# */5 * * * * /path/to/start_knights_server.sh >/dev/null 2>&1

# This will make the script run every 5 minutes.

# Don't forget to edit the /path/to/knights_config.txt below.

# Acknowledgement: Game Server HOWTO, http://www.faqs.org/docs/Linux-HOWTO/Game-Server-HOWTO.html#KEEPRUNNING

process=`ps auxw | grep knights_server | grep -v grep | grep -v start_knights_server.sh | awk '{print $11}'`

if [ -z "$process" ]; then
  echo "Couldn't find Knights Server running, restarting it."
  nohup knights_server -c /path/to/knights_config.txt &
  echo ""
fi
