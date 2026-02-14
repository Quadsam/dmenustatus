# dmenustatus

A basic status bar for dmenu.

## Installing

### Arch Linux

The package `dmenustatus-git` is avaliable on the Arch User Repository (AUR).
To install you need AUR helper such as `yay(1)`.

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

- [x] ~~Fix reading battery level from dynamic path~~
