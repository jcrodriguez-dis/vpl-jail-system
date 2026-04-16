Name:           vpl-jail-system
Version:        5.0.1
Release:        1%{?dist}
Summary:        Sandbox execution server for Virtual Programming Lab (VPL) for Moodle

License:        GPLv3+
URL:            https://vpl.dis.ulpgc.es/
Source0:        https://vpl.dis.ulpgc.es/releases/%{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  openssl-devel

Requires:       openssl
Requires:       bash
%{?systemd_requires}

%description
VPL Jail System is a daemon that provides a sandboxed execution environment
for the Virtual Programming Lab (VPL) activity plugin for Moodle.

It receives source code and scripts from the VPL Moodle plugin via HTTP/HTTPS,
compiles and executes them inside a chroot jail with resource limits enforced
through cgroups, and returns the results. It also provides a WebSocket interface
for interactive execution sessions.

This package ships the VPL own installer script (install-vpl-sh) which handles
installing required system packages (compilers, interpreters, etc.) and
configuring the service. After installing this RPM, run:

  %{_datadir}/%{name}/install-vpl-sh

as root to complete the setup.

%prep
%autosetup

%build
%configure
%make_build

%install
%make_install

# Install systemd unit file
install -Dm644 %{buildroot}%{_datadir}/%{name}/vpl-jail-system.service \
    %{buildroot}%{_unitdir}/vpl-jail-system.service

# Install configuration file
install -Dm640 %{buildroot}%{_datadir}/%{name}/vpl-jail-system.conf \
    %{buildroot}%{_sysconfdir}/vpl/vpl-jail-system.conf

# Install SysV init script
install -Dm755 %{buildroot}%{_datadir}/%{name}/vpl-jail-system.initd \
    %{buildroot}%{_initddir}/vpl-jail-system

%post
%systemd_post vpl-jail-system.service
echo "Run '%{_datadir}/%{name}/install-vpl-sh' as root to install required packages and complete setup."

%preun
%systemd_preun vpl-jail-system.service

%postun
%systemd_postun_with_restart vpl-jail-system.service

%files
%license COPYING
%doc AUTHORS ChangeLog INSTALL NEWS README README.md
%{_bindir}/vpl-jail-server
%{_datadir}/%{name}/
%{_unitdir}/vpl-jail-system.service
%{_initddir}/vpl-jail-system
%dir %{_sysconfdir}/vpl
%config(noreplace) %{_sysconfdir}/vpl/vpl-jail-system.conf

%changelog
* Wed Apr 16 2026 Juan Carlos Rodriguez-del-Pino <jc.rodriguezdelpino@ulpgc.es> - 5.0.1-1

