FROM ubuntu:20.04

ENV VPL_JAIL_SYSTEM_VERSION 3.0.0
ENV JAIL_PORT 81
ENV JAIL_SECURE_PORT 444
ENV HNAME localhost
ENV JAIL_URL_PASS secret

RUN apt-get -qq update && apt-get -yqq install --no-install-recommends apt-utils make lsb lsb-core g++ openssl libssl-dev  \
  iptables xorg dbus-x11 tightvncserver xfonts-75dpi xfonts-100dpi openbox gconf2 xterm firefox wget curl net-tools bc gnat\
  nasm gcc mono-complete ddd gfortran gdb hugs default-jre openjdk-11-jre openjdk-8-jre default-jdk openjdk-11-jdk         \
  openjdk-8-jdk openjfx checkstyle junit4 junit nodejs octave fp-compiler perl php-cli php-readline php-sqlite3 swi-prolog \
  python python2 python-tk python-numpy python3 python3-tk python3-numpy python3-pandas python3-matplotlib python3-pudb    \
  python3-pycodestyle python3-networkx mypy pycodestyle pydocstyle thonny ruby scala plt-scheme racket sqlite3 tcl valgrind

ENV LC_ALL en_US.UTF-8

RUN apt-get -yqq install --no-install-recommends locales
RUN rm -rf /var/lib/apt/lists/*
RUN localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

WORKDIR /usr/src/vpl-jail
COPY . .
RUN chmod +x ./docker-install-vpl-sh
RUN ./docker-install-vpl-sh
RUN chmod +x ./entrypoint.sh

CMD [ "./entrypoint.sh" ]
