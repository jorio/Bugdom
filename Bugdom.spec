#
# Icon by Exhumed under CC-BY-NC-SA 3.0 Unported License
# Spec file by <https://www.notabug.org/tux_peng>
#
# Bugdom® is a registered trademark of Pangea Software, Inc.

Name:           Bugdom 


Version:        1.3.0


Release:        0 


License:        CC BY-NC-SA 4.0


Group:          Amusements/Games/3D 


Summary:        Save Bugdom from Thorax's evil Fire Ants


Url:            https://github.com/jorio/Bugdom/ 


Source:         Bugdom-%{version}.tar.gz 


BuildRequires:  gcc-c++

BuildRequires:  cmake 

BuildRequires: SDL2-devel


BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description 
You are Rollie McFly, the only remaining bug capable of saving the Lady Bugs and restoring peace to the Bugdom. Rollie has been hiding in the Lawn area of the Bugdom and will need to travel far to get to the Ant Hill where the battle with King Thorax must take place. There will be water to cross, bugs to ride, and plenty of enemy forces to defeat, but once the Fire Ants and King Thorax have been defeated, you will become the new ruler of the Bugdom and peace will be restored. 

%prep 


%setup -q -n %{name}-%{version}


%build 

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

%install 
mkdir -p %{buildroot}%{_bindir}/%{name}
mkdir -p %{buildroot}%{_datadir}/{pixmaps,applications/}
echo -e '#!/usr/bin/sh\n\ncd "$(dirname "$(readlink -f "$0")")"/%{name}/;./%{name}' > %{buildroot}%{_bindir}/%{name}.sh
chmod +x %{buildroot}%{_bindir}/%{name}.sh
mv build-release/{Data,%{name}} %{buildroot}%{_bindir}/%{name}/
mv cmake/%{name}64.png %{buildroot}%{_datadir}/pixmaps/%{name}.png
mv %{name}.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop


%files 

%defattr(-,root,root)
%doc {LICENSE.md,docs,README.md}
%{_bindir}/{%{name},%{name}.sh}
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/applications/%{name}.desktop

%changelog 
