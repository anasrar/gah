if not exist build\ (
  mkdir build
)

cd build
cmake -G "Visual Studio 16 2019" -A x64 -S ..
msbuild gah.sln /p:Configuration=Release /p:Platform=x64
