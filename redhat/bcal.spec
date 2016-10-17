%global debug_package %{nil}

Name:		bcal
Version:	1.1
Release:	1%{?dist}
Summary:	Byte CALculator. The engineer's utility for storage conversions and calculations.

Group:		Applications/Engineering
License:	GPLv3
URL:		https://github.com/jarun/bcal
Source0:	%{name}-%{version}.tar.gz

BuildRequires:	gcc binutils make gzip
BuildRequires:	libquadmath-devel

%description
bcal (Byte CALculator) is a command-line utility for storage conversions and calculations. Storage, hardware and firmware developers work with numerical calculations regularly e.g., storage unit conversions, address calculations etc. If you are one and can't calculate the hex address offset for (512 - 16) MiB immediately, or the value when the 43rd bit of a 64-bit address is set, bcal is for you.

Though it started with storage, the scope of bcal isn't limited to the storage domain. Feel free to raise PRs to simplify other domain-specific numerical calculations so it can evolve into an engineer's tool.

bcal follows Ubuntu's standard unit conversion and notation policy.

%prep
%setup -q


%build
make %{?_smp_mflags}

%install
%{__install} -m755 -d %{buildroot}%{_bindir}
%{__install} -m755 -d %{buildroot}%{_mandir}/man1
%{__install} -m755 bcal %{buildroot}%{_bindir}/bcal
%{__install} -m644 bcal.1 %{buildroot}%{_mandir}/man1/bcal.1

%files
%{_bindir}/bcal
%{_mandir}/man1/bcal.1.gz
%doc README.md
%doc LICENSE
%doc CHANGELOG


%changelog
* Sat Oct 15 2016 Michael Fenn <michaelfenn87@gmail.com> - 1.1-1
- Initial RPM
