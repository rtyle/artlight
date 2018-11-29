# create firmware server certificate with unencrypted key.
# CN must match the host part in the FIRMWARE_UPDATE_URL (e.g. artlightserver)

	(cd certificates; openssl req -x509 -nodes -newkey rsa:2048 -keyout firmware_ca_key.pem -out firmware_ca_cert.pem -days 10000)

# start server

	(cd build; openssl s_server -WWW -key ../certificates/firmware_ca_key.pem -cert ../certificates/firmware_ca_cert.pem)

# create provision server certificate with unencrypted key.
# CN must match the host part in the device's AP URL (e.g. 192.168.4.1)

	(cd certificates; openssl req -x509 -nodes -newkey rsa:2048 -keyout provision_ca_key.pem -out provision_ca_cert.pem -days 10000)
