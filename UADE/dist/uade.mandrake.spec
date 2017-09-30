%define name uade
%define version 0.91
%define pre pre2
%define fname %name-%version-%pre
%define release 0.%pre.1mdk
#define release 1mdk

Summary: Unix Amiga Delitracker Emulator
Name: %{name}
Version: %{version}
Release: %{release}
Source0: http://uade.ton.tut.fi/uade/%{fname}.tar.bz2
Patch: uade-0.91-pre2-beepmp.patch.bz2
URL: http://uade.ton.tut.fi/
License: GPL
Group: Sound
BuildRoot: %{_tmppath}/%{name}-buildroot
Requires: lha
BuildRequires: lha
BuildRequires: libalsa-devel

%description
Plays old amiga tunes with UAE emulation and cloned m68k-assembler
Amiga delitracker API. With cloned delitracker API you don't have to
port old players from Amiga, you can re-use old Deliplayers that use
Amiga Delitracker API. Deliplayers are used like on Amiga, they exist
in some directory and you can copy/remove them as you wish. "Installing"
new players is just copying files to your 'players' directory.

%package -n xmms-uade
Group: Sound
Summary: Unix Amiga Delitracker Emulator Xmms input plugin
BuildRequires: libxmms-devel
Requires: xmms
Requires: uade = %version-%release

%description -n xmms-uade
Plays old amiga tunes with UAE emulation and cloned m68k-assembler
Amiga delitracker API. With cloned delitracker API you don't have to
port old players from Amiga, you can re-use old Deliplayers that use
Amiga Delitracker API. Deliplayers are used like on Amiga, they exist
in some directory and you can copy/remove them as you wish. "Installing"
new players is just copying files to your 'players' directory.

This is the input plugin for xmms based on uade.

%package -n beep-media-player-uade
Group: Sound
Summary: Unix Amiga Delitracker Emulator Beep Media Player input plugin
BuildRequires: beep-media-player-devel
Requires: beep-media-player
Requires: uade = %version-%release

%description -n beep-media-player-uade
Plays old amiga tunes with UAE emulation and cloned m68k-assembler
Amiga delitracker API. With cloned delitracker API you don't have to
port old players from Amiga, you can re-use old Deliplayers that use
Amiga Delitracker API. Deliplayers are used like on Amiga, they exist
in some directory and you can copy/remove them as you wish. "Installing"
new players is just copying files to your 'players' directory.

This is the input plugin for Beep Media Player based on uade.


%prep
%setup -q -n %fname
#setup -q
%patch -p1

%build
export CFLAGS="%optflags"
./configure --prefix=%{_prefix} --libdir=%_libdir --package-prefix=%buildroot --with-alsa
%make

%install
rm -rf %buildroot installed-docs
make install PACKAGEPREFIX=%buildroot BINDIR=%buildroot%_bindir DATADIR=%buildroot%_datadir/%name DOCDIR=%buildroot%_prefix/doc
#fix permissions
find . %buildroot/%_datadir/uade -type f -print0 |xargs -0 chmod 644
find . %buildroot/%_datadir/uade -type d -print0 |xargs -0 chmod 755
mv %buildroot%_prefix/doc installed-docs
rm -f installed-docs/uade.1 installed-docs/COPYING installed-docs/INSTALL.*
rm -f %buildroot%_libdir/{bmp,xmms}/Input/libuade.la

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%doc installed-docs/* 
%doc songs
%{_bindir}/*
%{_datadir}/uade
%_mandir/man1/uade.1*

%files -n xmms-uade
%defattr(-,root,root)
%doc README
%{_libdir}/xmms/Input/libuade.so

%files -n beep-media-player-uade
%defattr(-,root,root)
%doc README
%{_libdir}/bmp/Input/libuade.so

%changelog
* Fri Jul 16 2004 Götz Waschk <waschk@linux-mandrake.com> 0.91-0.pre2.1mdk
- use installed-docs
- patch for beep-media-player build
- add beep-media-player plugin
- new version

* Thu Jul 15 2004 Götz Waschk <waschk@linux-mandrake.com> 0.91-0.pre1.1mdk
- split out xmms plugin
- update docs list
- build with optimization flags
- new version

* Tue Jun 15 2004 Götz Waschk <waschk@linux-mandrake.com> 0.90-1mdk
- enable alsa
- new version

* Fri Apr 16 2004 Götz Waschk <waschk@linux-mandrake.com> 0.90-0.pre2.1mdk
- drop merged patch 0
- new version

* Thu Apr  8 2004 Götz Waschk <waschk@linux-mandrake.com> 0.90-0.pre1.1mdk
- enable parallel build
- add man page
- new version

* Tue Jan  6 2004 Götz Waschk <waschk@linux-mandrake.com> 0.81-1mdk
- new version

* Mon Sep 22 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-2mdk
- fix buildrequires
- drop prefix
- fix url

* Sat Aug 16 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-1mdk
- fix buildrequires
- new version

* Thu Jun 26 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre7.1mdk
- new version

* Wed May 21 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre6.1mdk
- new version

* Wed May 14 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre5.1mdk
- new version

* Sun Mar 16 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre4.1mdk
- new version

* Wed Mar 12 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre3.2mdk
- fix buildrequires

* Sun Feb 16 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre3.1mdk
- fix installation
- new version

* Thu Jan 23 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre2.1mdk
- new version

* Mon Jan 20 2003 Götz Waschk <waschk@linux-mandrake.com> 0.80-0.pre1.1mdk
- new version

* Mon Jan  6 2003 Götz Waschk <waschk@linux-mandrake.com> 0.73-1mdk
- new version

* Tue Dec 10 2002 Götz Waschk <waschk@linux-mandrake.com> 0.73-0.pre6.1mdk
- new version

* Wed Nov 27 2002 Götz Waschk <waschk@linux-mandrake.com> 0.73-0.pre5.1mdk
- new version

* Tue Nov 19 2002 Götz Waschk <waschk@linux-mandrake.com> 0.73-0.pre4.1mdk
- fix installation
- disable parallel build
- fix URL
- new version

* Tue Nov 12 2002 Götz Waschk <waschk@linux-mandrake.com> 0.73-0.pre1.1mdk
- new version

* Fri Nov  8 2002 Götz Waschk <waschk@linux-mandrake.com> 0.72-0.pre1.1mdk
- fix the fixing of the file permissions
- add a fake libdir to configure to please rpmlint
- enable parallel build
- drop patch
- new version

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
