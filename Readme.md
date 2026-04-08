***NOT FOR PRODUCTION!!!***
A simple pdf parser write in C++, implemented part of the PDF Specification 1.7. And the renderer is [plutovg](https://github.com/sammycage/plutovg), specifical thanks to [@sammycage](https://github.com/sammycage)'s work.

Window: [MiniFB](https://github.com/emoon/minifb)

## Features
- [x] Object and Stream reading and parsing
- [x] Path rendering
- [x] XObject rendering
- [x] Text rendering and extracting (Type0, Type1, Type3, TrueType)

## Build & Run
```bash
git clone --recurse-submodules https://github.com/tktkku/pdf-renderer.git
cd pdf-renderer
# Linux
cmake -S . -B build
cmake --build build
./build/output/test tiger.pdf
# Visual Studio
cmake -S . -B build -G "Visual Studio 16 2019"
cmake --build build --config Debug
./build/output/Debug/test.exe tiger.pdf
```

Press  `A` for previous page, and `D` for next page.

## License
```
MIT License

Copyright (c) 2026 tktkku <xbqiii@126.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```