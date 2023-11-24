ARG VPL_BASE_DISTRO
ARG VPL_INSTALL_LEVEL
FROM ${VPL_BASE_DISTRO:-debian}
USER root:root

COPY . /vpl-jail-system
WORKDIR /vpl-jail-system

# Select the compilers/interpreters to install
#      options: minimum < basic < standard < full
RUN /vpl-jail-system/install-vpl-sh noninteractive ${VPL_INSTALL_LEVEL:-basic}

# Using HTTP only
# ENV VPL_JAIL_SECURE_PORT=0
# EXPOSE 80

# Using HTTPS only
# ENV CERTIFICATES_DIR="/etc/vpl/ssl"
# ENV VPL_JAIL_SSL_CERT_FILE="${CERTIFICATES_DIR}/cert.pem"
# ENV VPL_JAIL_SSL_KEY_FILE="${CERTIFICATES_DIR}/key.pem"
# ENV VPL_JAIL_PORT=0
# VOLUME [ "${CERTIFICATES_DIR}" ]
# EXPOSE 443

# Using HTTP and HTTPS
ENV CERTIFICATES_DIR="/etc/vpl/ssl"
ENV VPL_JAIL_SSL_CERT_FILE="${CERTIFICATES_DIR}/cert.pem"
ENV VPL_JAIL_SSL_KEY_FILE="${CERTIFICATES_DIR}/key.pem"
VOLUME [ "${CERTIFICATES_DIR}" ]
EXPOSE 80 443

STOPSIGNAL SIGTERM

CMD ["/usr/sbin/vpl/vpl-jail-system", "start_foreground"]
