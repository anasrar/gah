if not exist build\ (
  mkdir build
)

if not exist sqlite3\sqlite3.lib (
	cd sqlite3
	lib /DEF:sqlite3.def /OUT:sqlite3.lib /MACHINE:x64
	cd ..
)

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
set CL=/DTRAGET_GIT_BASH
msbuild gah.sln /p:Configuration=Release /p:Platform=x64
cd ..
copy sqlite3\sqlite3.dll build\bin\Release