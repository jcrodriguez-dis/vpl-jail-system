%define name vpl-xmlrpc-jail
%define version 1.0
%define release 1

Summary: daemon jail for vpl using xmlrpc and init script to create jail file system
Name: %{name}
Version: %{version}
Release: %{release}
#Source: ftp://ftp.vpl.org/pub/%{name}-%{version}.tar.gz
#Vendor: VPL
#URL: http://www.vpl.org/
License: Distributable
Group: Servers
Prefix: %{_prefix}

%description
This package contains a daemon to server xmlrpc request from vpl server.
The vpl server send source code and scripts to compile and execute program in a chroot jail. Also add a init script to buil the jail file system based in NFS.

%prep
%setup -q

%build
./configure
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS README TODO
%{_prefix}/lib/lib*.so.*

%changelog
* Thu Mar 7 2002 T.R. Fullhart <kayos@kayos.org>
- First release
