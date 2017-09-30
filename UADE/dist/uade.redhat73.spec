%define name uade
%define version 0.71
%define release 1.rh73

Summary: Unix Amiga Delitracker Emulator
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.bz2
Patch:	 uade-0.71-pwrap-path.patch.bz2
URL: http://uade.ton.tut.fi
License: GPL
Group: Sound
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
Requires: xmms
Requires: lha
BuildRequires: xmms-devel
BuildRequires: lha

%description
Plays old amiga tunes with UAE emulation and cloned m68k-assembler
Amiga delitracker API. With cloned delitracker API you don't have to
port old players from Amiga, you can re-use old Deliplayers that use
Amiga Delitracker API. Deliplayers are used like on Amiga, they exist
in some directory and you can copy/remove them as you wish. "Installing"
new players is just copying files to your 'players' directory.


%prep
%setup -q
%patch

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix}
make

%install
rm -rf %buildroot
mkdir -p %{buildroot}%{_libdir}/xmms/Input %{buildroot}%_bindir
make install SYSDATADIR=%buildroot%{_datadir}/uade  BINDIR=%{buildroot}/%_bindir DOCDIR=%buildroot/usr/doc
cp plugindir/.libs/libuade.so %buildroot%{_libdir}/xmms/Input
#fix permissions
find . %buildroot/%datadir/uade -type f |xargs chmod 644
find . %buildroot/%datadir/uade -type d |xargs chmod 755
%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%doc *.txt COPYING INSTALL README uade-docs/* 
%{_bindir}/*
%{_datadir}/uade
%{_libdir}/xmms/Input/libuade.so

%changelog
* Fri Aug 16 2002 Michael Schwendt <mschwendt@yahoo.com> 0.71-1.rh73
- Adapted spec file for Red Hat Linux 7.3.
- libxmms1-devel -> xmms-devel
- +RPM_OPT_FLAGS

* Thu Jun  6 2002 Götz Waschk <waschk@linux-mandrake.com> 0.71-1mdk
- don't use %%make
- patch pwrap frontend to use the HOME environment variable
- requires lha
- new version

* Thu Feb  7 2002 Götz Waschk <waschk@linux-mandrake.com> 0.70-1mdk
- fixed documentation
- players are now in %%_datadir/uade
- remove patch
- 0.70

* Tue Jan  8 2002 Götz Waschk <waschk@linux-mandrake.com> 0.60-2mdk
- really fix directory to %%_libdir/uade

* Tue Jan  8 2002 Götz Waschk <waschk@linux-mandrake.com> 0.60-1mdk
- added programs to %%_bindir
- fixed installation
- regenerate patch
- 0.60

* Mon Dec  3 2001 Götz Waschk <waschk@linux-mandrake.com> 0.51-1mdk
- 0.51
- added db-content file to doc

* Thu Nov  1 2001 Götz Waschk <waschk@linux-mandrake.com> 0.50-2mdk
- added URL

* Wed Oct 24 2001 Götz Waschk <waschk@linux-mandrake.com> 0.50-1mdk
- updated doc
- 0.50

* Tue Sep 11 2001 Lenny Cartier <lenny@mandrakesoft.com> 0.40-1mdk
- added by Götz Waschk <waschk@linux-mandrake.com> :
	- patch to install in libdir
	- initial package
