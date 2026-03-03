:warning::construction:Under construction!

A simple pdf parser write in C++, implemented part of the PDF Specification 1.7. And the renderer is [plutovg](https://github.com/sammycage/plutovg), specifical thanks to [@sammycage](https://github.com/sammycage)'s work.

## Build & Run
```bash
git clone --recurse-submodules https://github.com/tktkku/pdf-renderer.git
cd pdf-renderer
cmake -S . -B build
cmake --build build
./build/output/test tiger.pdf

## License
```
MIT License

Copyright (c) 2025 tktkku <xbqiii@126.com>

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