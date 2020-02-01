#
# gnome-arcade
#
# TO OBTAIN commit/gittag use:
# git ls-remote https://github.com/strippato/gnome-arcade.git 
# see https://fedoraproject.org/wiki/Packaging:SourceURL
#
# to download source:
# spectool -C ../SOURCES -g gnome-arcade.spec

%global commit d0fd1dfc0daaaf7c9b71010327f03e34f9902a2b
%global gittag v.%{version}
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:           gnome-arcade
Version:        0.217.3
Release:        1%{?dist}
Summary:        a minimal MAME frontend 

License:        GPLv3+
URL:            https://github.com/strippato/gnome-arcade
Source0:        https://github.com/strippato/%{name}/archive/%{gittag}/%{name}-%{version}.tar.gz   
Group:          Unspecified

BuildRequires:  gcc
BuildRequires:  cmake >= 2.8.11
BuildRequires:  gtk3-devel >= 3.16
BuildRequires:  libarchive-devel
BuildRequires:  libevdev-devel
BuildRequires:  vlc-devel
BuildRequires:  gdk-pixbuf2-devel

Requires:       mame
Requires:       gtk3
Requires:       gdk-pixbuf2
Requires:       libarchive
Requires:       libevdev
Requires:       vlc
Requires:       xdg-utils

%description
Gnome Arcade, a minimal MAME frontend

%prep
%autosetup -n %{name}-%{gittag}

%build
mkdir -p build
cd build
%cmake -DAPP_RES=\"%{_datadir}/gnome-arcade\" \
       -DCMAKE_C_FLAGS="-g -O3 -fmessage-length=0 \
       -D_FORTIFY_SOURCE=2 -fstack-protector \
       -funwind-tables -fasynchronous-unwind-tables -fPIC" \
       --no-warn-unused-cli ..

#make %{?_smp_mflags}
%make_build


%install
install -D -m 0755 gnome-arcade %{buildroot}%{_bindir}/gnome-arcade
install -D -m 0644 res/gnome-arcade.png %{buildroot}%{_datadir}/pixmaps/gnome-arcade.png
install -D -m 0644 gnome-arcade.desktop %{buildroot}%{_datadir}/applications/gnome-arcade.desktop

mkdir -p %{buildroot}%{_datadir}/gnome-arcade/res/
install -D -m 0644 res/* %{buildroot}%{_datadir}/gnome-arcade/res/

mkdir -p %{buildroot}%{_datadir}/gnome-arcade/data/rom/
install -D -m 0644 data/rom/* %{buildroot}%{_datadir}/gnome-arcade/data/rom/

mkdir -p %{buildroot}%{_datadir}/gnome-arcade/data/tile
install -D -m 0644 data/tile/* %{buildroot}%{_datadir}/gnome-arcade/data/tile/


%files
%doc README.md
%{_bindir}/*
%{_datadir}/pixmaps/gnome-arcade.png
%{_datadir}/applications/gnome-arcade.desktop
%dir %{_datadir}/gnome-arcade/
%{_datadir}/gnome-arcade/*

%changelog
* Fri Jan  12 2020 strippato <strippato@gmail.com>
- romset 217 to 209
* Fri Jan  3 2020 strippato <strippato@gmail.com>
- test
