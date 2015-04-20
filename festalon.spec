Summary: A command-line program for playing HES and NSF/NSFE music files.
Name: festalon
Version: 0.5.5
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source: festalon-0.5.5.tar.bz2
BuildRoot: /var/tmp/%{name}-buildroot

%description
Festalon plays HES and NSF/NSFE music files using a command-line+console interface.

%prep
%setup -n festalon

%build
./configure --prefix=/usr
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin

install -s -m 755 src/festalon $RPM_BUILD_ROOT/usr/bin/festalon

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README TODO COPYING ChangeLog

/usr/bin/festalon
