%global debug_package %{nil}
%global __strip /bin/true

Name:          bcache-ctl 	
Version:        %{ver}
Release:        %{rel}%{?dist}

Summary:	This is an command line tool for bcache,which used to manage bcache devices.

Group:		Storage	
License:	GPL
URL:		https://github.com/dahefanteng/bcache-ctl
Source0:	%{name}-%{version}-%{rel}.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Requires:	libsmartcols
Requires:	libblkid
Requires:	libuuid

%description


%prep
%setup -q -n %{name}-%{version}-%{rel}


%build
make

%install
rm -rf %{buildroot}
install -D -m 755 %{_builddir}/bcache-ctl-%{version}-%{rel}/bcache-ctl %{buildroot}%{_bindir}/bcache-ctl

%post

%preun

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/bin/bcache-ctl

%changelog
