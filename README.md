# gah

simple cli github token manager, support linux and windows.

## Feature

### List token

list all token on database

```bash
gah list
```

### Search token

search token on database

```bash
gah search [string:id|note|username|expired] [string,int:query]
```

### Clipboard

**NOTICE**: some minimal linux distro with only window manager may not work, try to install `libx11-xcb`

copy github token to clipboard

```bash
gah clipboard [int:id] --password <password> --show
```

### Add token

add token to database

```bash
gah add [string:note] [string:username] [string:token] [string:expired] --password <password>
```

### Remove token

remove token from database

```bash
gah remove [int:id]
```

## Git integration

simple integration with git, **keep in mind that command lack of git command flag and only support with https protocol**.

### Push

```bash
gah push [string:remote] [string:branch] [int:id] --password <password> --force
```

### Pull

```bash
gah pull [string:remote] [string:branch] [int:id] --password <password> --rebase
```

### Clone

```bash
gah clone [string:url] [int:id] [string:directory] --password <password> --recursive --branch <branch_name>
```

## Build

### Clone

```bash
git clone https://github.com/anasrar/gah.git --recursive && cd gah
```

### Compile SQLite

You can compile SQLite by yourself or just download from [https://www.sqlite.org/download.html](https://www.sqlite.org/download.html), download the `sqlite-amalgamation-*.zip` file

You need `shell.c`, `sqlite3.c`, `sqlite3.h`, and `sqlite3ext.h`

#### Linux

```sh
cd sqlite3
../submodules/sqlite3/configure
make
make sqlite3.c
```

#### Windows MSVC

```batch
cd sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc TOP=..\submodules\sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc sqlite3.c TOP=..\submodules\sqlite3
```

Further information compiling SQLite check [https://github.com/sqlite/sqlite#compiling-for-unix-like-systems](https://github.com/sqlite/sqlite#compiling-for-unix-like-systems)

### Dependency

linux using `xcb` and `pthread` for using clipboard, make sure `libx11-devel` or `libx11` is installed

### Linux

```bash
./build.sh
```

binary file in `build/bin`

### Windows / MSBuild

for cmd or powershell

```bash
build.bat
```

for git bash

```bash
build_gitbash.bat
```

binary file in `build\bin\Release`

## Database File

You can copy `.gahdb` file from linux to windows and vice versa

### Linux

```bash
echo $HOME/.gahdb
```

### Windows

```batch
echo %HOMEDRIVE%%HOMEPATH%\.gahdb
```

## Roadmap

- Add more git command flags
- Create bash auto completion

## Build With

- [CMake](https://cmake.org/)
- [linuxdeployqt](https://github.com/probonopd/linuxdeployqt)
- [SQLite](https://www.sqlite.org/index.html)
- [PicoSHA2](https://github.com/okdshin/PicoSHA2)
- [plusaes](https://github.com/kkAyataka/plusaes)
- [argparse](https://github.com/p-ranav/argparse)
- [tabulate](https://github.com/p-ranav/tabulate)
- [clip](https://github.com/dacap/clip)

## Contribute

how to contribute

- fork
- make some changes
- build using github action
- pull request

## License

[MIT](LICENSE) © Anas Rin
