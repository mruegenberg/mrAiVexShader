# mrCumulo
Arnold implementation of the Resolution Independent Volumes / Cumulo / SELMA approach by Tessendorf and Kowalski.

Arnold version currently used: 4.2.13.0.

## Building
1. Open CMake and set the source dir to this folder
2. Create a build folder in this folder and point the build dir in CMake there
3. Click "Add Entry" in the CMake GUI and set `MTOA_BASE_DIR=O:\_software\plugins\maya\solidangle\mtoadeploy\2016`
4. Click `Configure` and `Generate`. You get a Visual Studio solution on Windows
5. Open the new VS solution
  6. Change the build type to *Release* and right-click on Cumulo in the Solution Explorer and click *Set as StartUp project"
7. Do *Build > Build Cumulo*

This should generate a dll somewhere in `build/Release`. Copy the `test.ass` file to the same place and do `kick.exe test.ass` for a quick test.
