# timestamp
A tool that modifies incoming image and video filenames into a standard time format.

## Installation
Ensure you have `exiv2` and `gcc` (version 14+) installed on your system.
```bash
git clone https://github.com/amot-dev/timestamp.git
cd timestamp
make
```
## Usage
timestamp can be run either in the current directory as `./timestamp` or in another directory as `./timestamp [path]`. Follow instructions to rename your pictures and videos.

### Key Features
- Rename images and/or videos to %Y-%m-%d-%H%M-%S format (ex. 2024-12-29-1648-16.jpg)
- Modify proposed names to avoid name clashes
- Skip files to rename
- Will not allow you to rename files if it would result in filenames clashing

## Images
![image](https://github.com/user-attachments/assets/1b9de74c-f9ae-4d1b-995c-bc84a41640fd)

