make modules modules=modules/curl
cp /home/cloud/opensips/opensips-1.8.2-tls/modules/curl/curl.so /usr/local/lib/opensips/modules/
tar czf /var/backup/curl_module.tgz modules/curl/*.c modules/curl/*.h ./util.sh /usr/local/etc/opensips/opensips.cfg
echo "backup done."
echo "start opensips with : (sudo /usr/local/sbin/opensips -P /var/run/opensips.pid -E) "
