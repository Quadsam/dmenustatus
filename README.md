# dmenustatus

A statusbar for dmenu

**This package is still in beta and may not work on your system, but
if you have any issues feel free to make an issue and I will try to help
to the best of my abilities. (Pull requests are also welcome)**

## Installing

### Arch Linux

The package `dmenustatus-git` is avaliable on the Arch Linux User
Repository (AUR). To install you need AUR helper such as `yay(1)`, `yay` is
used here.

```bash
yay -S dmenustatus-git
```

### From Source

- Clone the source repo:

``` bash
git clone https://github.com/Quadsam/dmenustatus.git
```

- Build the package:

``` bash
make
```

- Install the package:

``` bash
sudo make PREFIX='/usr/local' install
```

## License

``` license
dmenustatus - a statusbar for dwm's dmenu
Copyright (C) 2023-2026  Quadsam

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```


## TODO/BUGS

- Fix reading battery level from dynamic path
