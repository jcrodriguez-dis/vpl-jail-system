ARG VPL_BASE_DISTRO=debian

FROM ${VPL_BASE_DISTRO}

USER root:root

COPY . /vpl-jail-system
WORKDIR /vpl-jail-system

# Install bash on distros with no bash
RUN /vpl-jail-system/install-bash-sh

# Install levels: minimum < basic < standard < full
ARG VPL_INSTALL_LEVEL=basic

RUN /vpl-jail-system/install-vpl-sh noninteractive ${VPL_INSTALL_LEVEL}

# Using HTTP and HTTPS
ENV VPL_CERTIFICATES_DIR="/etc/vpl/ssl"
ENV VPL_JAIL_SSL_CERT_FILE="${CERTIFICATES_DIR}/cert.pem"
ENV VPL_JAIL_SSL_KEY_FILE="${CERTIFICATES_DIR}/key.pem"
VOLUME [ "${VPL_CERTIFICATES_DIR}" ]
EXPOSE 80 443

STOPSIGNAL SIGTERM

CMD ["/usr/sbin/vpl/vpl-jail-system", "start_foreground"]
