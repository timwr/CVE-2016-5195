
if [ -f /data/local/tmp/test ]; then
    chmod 777 /data/local/tmp/test
    rm /data/local/tmp/test
fi

if [ -f /data/local/tmp/test2 ]; then
    chmod 777 /data/local/tmp/test2
    rm /data/local/tmp/test2
fi

echo vulnerable!!!!!!! > /data/local/tmp/test
echo yournotvulnerable > /data/local/tmp/test2

chmod 444 /data/local/tmp/test2
ls -l /data/local/tmp/test*

