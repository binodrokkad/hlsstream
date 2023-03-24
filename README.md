# HLS Sream
Simple hls stream receiver.
Is compatible with Android, iOS, macOS and Windows.
Currently, it depends on libcurl amd ffmpeg.

### Android
Library build.

- ```cd android```
- ```cd hlsstream ```
- ```./gradlew aR   ```

Test app is inside same directory.

### Windows
Library build using visual studio.

Open vs project inside windows directory.
 vcpkg can be used to link with libcurl and ffmpeg
- ``` vcpkg install curl ```
- ``` vcpkg install ffmpeg ```

### macOS
Library build using xcode

Open xcode project inside macOS directory
ffmpeg is required to be installed using homebrew.
- ``` brew install ffmpeg ```

### Linux
Library build using cmake

Open terminal and install dependencies.
- ```sudo apt install libavcodec-dev```
- ```sudo apt install libavformat-dev```
- ```sudo apt install curl```

building
- ``` mkdir build && cd build ```
- ``` cmake -DCMAKE_BUILD_TYPE=Debug .. && make```

