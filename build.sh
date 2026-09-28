cmake -S . -B build -G "Unix Makefiles"
cmake --build build
#cmake --install build --prefix renderer
cp build/MainProject/MainProject main

