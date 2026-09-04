ARG VPL_BASE_DISTRO=debian

FROM ${VPL_BASE_DISTRO}

USER root:root

# Set default install levels: minimum < basic < standard < full
ARG VPL_INSTALL_LEVEL=standard
# Language servers to install: empty (none), "all" or a space separated list of languages
ARG VPL_INSTALL_LS=
ARG VPL_JAIL_JAILPATH=/
ARG VPL_JAIL_PORT=80
ARG VPL_JAIL_SECURE_PORT
ARG VPL_CERTIFICATES_DIR=/etc/vpl/ssl
ARG VPL_JAIL_SSL_CERT_FILE="${VPL_CERTIFICATES_DIR}/fullchain.pem"
ARG VPL_JAIL_SSL_KEY_FILE="${VPL_CERTIFICATES_DIR}/privkey.pem"

ENV VPL_JAIL_JAILPATH=${VPL_JAIL_JAILPATH}
ENV VPL_JAIL_PORT=${VPL_JAIL_PORT}
ENV VPL_JAIL_SECURE_PORT=${VPL_JAIL_SECURE_PORT}
ENV VPL_CERTIFICATES_DIR="${VPL_CERTIFICATES_DIR}"
ENV VPL_JAIL_SSL_CERT_FILE="${VPL_JAIL_SSL_CERT_FILE}"
ENV VPL_JAIL_SSL_KEY_FILE="${VPL_JAIL_SSL_KEY_FILE}"

# Copy installer
COPY . /vpl-jail-system
WORKDIR /vpl-jail-system

# Install bash on distros with no bash
RUN /vpl-jail-system/install-bash-sh

# Run VPL installer
RUN /vpl-jail-system/install-vpl-sh noninteractive ${VPL_INSTALL_LEVEL}

# Install the language servers and their launchers
RUN if [ -n "${VPL_INSTALL_LS}" ] ; then \
        /vpl-jail-system/install-vpl-ls-sh ${VPL_INSTALL_LS} ; \
    fi

# Remove installer
WORKDIR /
RUN rm -R vpl-jail-system

VOLUME [ "${VPL_CERTIFICATES_DIR}" ]
EXPOSE 80 443

STOPSIGNAL SIGTERM

CMD ["/usr/sbin/vpl/vpl-jail-system", "start_foreground"]
