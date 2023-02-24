#!/bin/bash
VPL_CONFIG_DIR=/etc/vpl

function vpl_generate_selfsigned_certificate {
	local INAME
	echo "Generating self-signed SSL certificate formachine"
	#Get host name to generate the certificate
	INAME=$HNAME
	if [ "$INAME" = "" ] ; then
		INAME=localhost
	fi
	#Generate key
	openssl genrsa -passout pass:12345678 -des3 -out key.pem 2048
	#Generate certificate for this server
	local SUBJ="/C=ES/ST=State/L=Location/O=VPL/OU=Execution server/CN=$INAME"
	openssl req -new -subj "$SUBJ" -key key.pem -out certini.pem -passin pass:12345678
	#Remove key password
	cp key.pem keyini.pem
	openssl rsa -in keyini.pem -out key.pem -passin pass:12345678
	#Generate self signed certificate for 5 years
	openssl x509 -in certini.pem -out cert.pem -req -signkey key.pem -days 1826 
}
function vpl_start_service {
	service vpl-jail-system start
}
function vpl_info_using_service {
	echo
	echo "-------------------------------------------"
	echo "Notice: you may use next command to control the service"
	echo "service vpl-jail-system [start|stop|status]"
}

if [ ! -f $VPL_CONFIG_DIR/key.pem ] ; then
	echo "VPL execution server needs SSL certificates to accept https:"
	echo "and wss: requests."
	echo "If you have certificates then copy the key and the certificate file"
	echo "in pem format to $VPL_CONFIG_DIR/key.pem and $VPL_CONFIG_DIR/cert.pem"
	echo "If you don't have certificate the intaller can install and configure Let's Encrypt for you."
	echo "Notice that this machine must be accesible from internet and has port 80 available."
	echo "Generating a selfsigned certificate for you."
	if [ "$A" != "n" ] ; then
		vpl_generate_selfsigned_certificate
		cp key.pem $VPL_CONFIG_DIR
		cp cert.pem $VPL_CONFIG_DIR
		chmod 600 $VPL_CONFIG_DIR/*.pem
		rm key.pem keyini.pem certini.pem cert.pem
	fi
else
	echo "Found SSL certificate => Don't create a new one"
fi

sed -i "s/#PORT=.*/PORT=$JAIL_PORT/g" /etc/vpl/vpl-jail-system.conf
sed -i "s/#SECURE_PORT=.*/SECURE_PORT=$JAIL_SECURE_PORT/g" /etc/vpl/vpl-jail-system.conf
sed -i "s/URLPATH=\/.*/URLPATH=\/$JAIL_URL_PASS/g" /etc/vpl/vpl-jail-system.conf
echo "Starting the service (vpl-jail-system)"
vpl_start_service
vpl_info_using_service

tail -f /dev/null
