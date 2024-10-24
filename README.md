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
```
timestamp --help
Usage: timestamp [directory] [-f|--force] [-e|--exif <tag>] [-x|--xmp <tag>] [-d|--date-format <format>] [-i|--interactive] [-h|--help]
Options:
  -f, --force                     Force execution (will delete clashing files, not recommended)
  -e, --exif <tag>                Specify EXIF metadata date tag
  -x, --xmp <tag>                 Specify XMP metadata date tag
  -d, --date-format <format>      Specify date format for renamed files
  -i, --interactive               Enable interactive mode
  -h, --help                      Show this help message
```
Can be run either in the current directory as `timestamp` or in another directory as `timestamp [directory]`. Follow instructions to rename your pictures and videos.

### Default Values
- EXIF Tag (For images): `Exif.Photo.DateTimeOriginal`
- XMP Tag (For videos): `Xmp.video.ModificationDate`
- Date Format: `%Y-%m-%d-%H%M-%S` (ex. 2024-12-29-1648-16.jpg)

### Key Features
- Rename images and/or videos to given date format
- Modify proposed names to avoid name clashes
- Skip files to rename
- Prevent files being named the same (resulting in a file deletion), unless called with `-f or --force`

## Images
![image](https://github.com/user-attachments/assets/1b9de74c-f9ae-4d1b-995c-bc84a41640fd)

