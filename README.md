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

