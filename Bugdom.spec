#
# spec file for package Bugdom by <https://www.notabug.org/tux_peng>
#
# Bugdom® is a registered trademark of Pangea Software, Inc.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://github.com/jorio/Bugdom/issues
#


Name:           Bugdom
Version:        1.3.0
Release:        0
Summary:        Save Bugdom from Thorax's evil Fire Ants
License:        CC BY-NC-SA 4.0
Group:          Amusements/Games/3D/Other
URL:            https://github.com/jorio/Bugdom/
Source:         Bugdom-%{version}.tar.gz
BuildRequires:  SDL2-devel
BuildRequires:  cmake >= 3.3
BuildRequires:  glibc >= 2.2.5
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(gl)
BuildRequires:  pkgconfig(glu)
%if "%{_repository}" == "openSUSE_Leap_15.3"
BuildRequires:  gcc10-c++
%else
BuildRequires:  gcc-c++
%endif

%description
You are Rollie McFly, the only remaining bug capable of saving the Lady Bugs and restoring peace to the Bugdom. Rollie has been hiding in the Lawn area of the Bugdom and will need to travel far to get to the Ant Hill where the battle with King Thorax must take place. There will be water to cross, bugs to ride, and plenty of enemy forces to defeat, but once the Fire Ants and King Thorax have been defeated, you will become the new ruler of the Bugdom and peace will be restored.

%global debug_package %{nil}

%prep
%setup -q


%build
%if "%{_repository}" == "openSUSE_Leap_15.3"
export CXX=gcc-10
export CC=gcc-10
%endif
# FIXME: you should use the %%cmake macros
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
# FIXME: you should use the %%cmake macros
cmake --build build-release

%install
mkdir -p %{buildroot}%{_bindir}/%{name}
mkdir -p %{buildroot}%{_datadir}/{pixmaps,applications/}
echo -e '#!%{_bindir}/sh\n\ncd "$(dirname "$(readlink -f "$0")")"/%{name}/;./%{name}' > %{buildroot}%{_bindir}/%{name}.sh
chmod +x %{buildroot}%{_bindir}/%{name}.sh
mv build-release/{Data,%{name}} %{buildroot}%{_bindir}/%{name}/
mv cmake/%{name}64.png %{buildroot}%{_datadir}/pixmaps/%{name}.png
mv %{name}.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop


%files
%license {LICENSE.md,docs,README.md}
%{_bindir}/{%{name},%{name}.sh}
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/applications/%{name}.desktop

%changelog
