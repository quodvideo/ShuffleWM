Xephyr :2 -ac -screen 1024x768 &
sleep 2
DISPLAY=:2 xterm &
DISPLAY=:2 ./shuffle 2>errors.txt &

