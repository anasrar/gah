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

copy github token to clipboard

```bash
gah clipboard [int:id] 
```

### Add token

add token to database

```bash
gah add [string:note] [string:username] [string:token] [string:expired]
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
gah push [string:remote] [string:branch] [int:id] --force
```

### Pull

```bash
gah push [string:remote] [string:branch] [int:id] --rebase
```
### Clone

```bash
gah clone [string:url] [int:id] [string:directory] --recursive --branch <branch_name>
```

## Build

### Clone

```bash
git clone https://github.com/anasrar/gah.git --recursive && cd gah
```
### Compile SQLite

You can compile SQLite by yourself or just download from [https://www.sqlite.org/download.html](https://www.sqlite.org/download.html), download the `sqlite-amalgamation-*.zip` file (and `sqlite-dll-win64-x64-*.zip` if you want to build from windows), extract to `sqlite3` folder

You only need `sqlite3.c` and `sqlite3.h`, additionally `sqlite3.deff` and `sqlite3.dll` for windows

#### Linux

```sh
cd sqlite3
../submodules/sqlite3/configure
make
make sqlite3.c
```

#### Windows MVSC

```batch
cd sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc TOP=..\submodules\sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc sqlite3.c TOP=..\submodules\sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc sqlite3.dll TOP=..\submodules\sqlite3
nmake /f ..\submodules\sqlite3\Makefile.msc sqlite3.exe TOP=..\submodules\sqlite3
```

Further information compiling SQLite check [https://github.com/sqlite/sqlite#compiling-for-unix-like-systems](https://github.com/sqlite/sqlite#compiling-for-unix-like-systems)

### Dependency

linux using qt5 gui for using clipboard, make sure `qt5-base` and `sqlite3` (optional, if you have it you don't need to compile / download sqlite3) was installed on your machine.

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

At the moment you can not copy `.gahdb` file from linux to windows and vice versa

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

- [SQLite](https://www.sqlite.org/index.html)
- [PicoSHA2](https://github.com/okdshin/PicoSHA2)
- [plusaes](https://github.com/kkAyataka/plusaes)
- [argparse](https://github.com/p-ranav/argparse)
- [tabulate](https://github.com/p-ranav/tabulate)
- [ClipboardXX](https://github.com/Arian8j2/ClipboardXX)

## License

[MIT](LICENSE) © Anas Rin
