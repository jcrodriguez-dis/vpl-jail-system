FROM debian
USER root
COPY . /vpl-jail-system
WORKDIR /vpl-jail-system
RUN /vpl-jail-system/.install-vpl-sh
EXPOSE ${VPL_HTTP_PORT} ${VPL_HTTPS_PORT}
CMD ["/usr/sbin/vpl/vpl-jail-system", "start"]