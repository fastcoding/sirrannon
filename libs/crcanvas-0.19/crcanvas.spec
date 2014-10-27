%define packager %(whoami)
%define _topdir %(pwd)/build

Summary: crcanvas
Name: crcanvas
Version: 0.16
Release: 1%{?redhatvers:.%{redhatvers}}
License: LGPL
Group:  Development/Libraries
Source: http://downloads.sourceforge.net/geocanvas/%{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot

%description
A canvas widget for GTK/Cairo.

%prep
%setup -q

%build
if test `arch` = "x86_64"
then
        ./configure --prefix=/usr libdir=/usr/lib64 --enable-gtk-doc
else
        ./configure --prefix=/usr --enable-gtk-doc
fi
make
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/init.d
make DESTDIR=$RPM_BUILD_ROOT install
%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ChangeLog README NEWS
%{_libdir}/*
%{_datadir}/*
%{_includedir}/%{name}

%changelog
* Thu Dec 14 2006 Robert Gibbs
- quick way to get an RPM

