# timestamp
A tool that allows you to rename EXIF and XMP files to reflect date found in metadata. The specific tag used to extract data from the file can be changed, as can the final date format used in the file rename.

## Installation
### Arch Linux
Available on the [AUR](https://aur.archlinux.org/packages/timestamp).

### Debian/Ubuntu
Download the deb file from the [latest release](https://github.com/amot-dev/timestamp/releases/latest/) and install it:
```
sudo apt install libexiv2-27
dpkg -i timestamp_1.1.1-1_amd64.deb
```
Note that while this should work on all Debian-based systems, it was only tested on Debian itself (all dependencies exist in Ubuntu repositories).

### Other
Ensure you have `exiv2`, `yaml-cpp`, and `gcc` (version 14+) installed on your system.
```bash
git clone https://github.com/amot-dev/timestamp.git
cd timestamp
make
```
## Usage
```
timestamp --help
Usage: timestamp [directory] [--config-file <path>] [-f|--force] [-i|--interactive] [-h|--help]
Options:
  -c, --config-file <path>        Specify YAML configuration file
  -f, --force                     Force execution (will delete clashing files, not recommended)
  -i, --interactive               Enable interactive mode
  -h, --help                      Show this help message
```
Can be run either in the current directory as `timestamp` or in another directory as `timestamp [directory]`. Follow instructions to rename your pictures and videos.

### Key Features
- Rename images and/or videos to given date format
- Modify proposed names to avoid name clashes
- Skip files to rename
- Prevent files being named the same (resulting in a file deletion), unless called with `-f or --force`

### Config File
The config for timestamp is in yaml form. The default config is generated on first run, and there is also a sample available in this repository. When running timestamp, it is possible to specify an override config file.

The config file consists of a date format (full list of options [here](https://man7.org/linux/man-pages/man1/date.1.html)), groupings of file extensions, and the list of tags for each group.
These tags will be tried in the order they are listed. Exiv2 has lists of EXIF and XMP tags [here](https://exiv2.org/metadata.html). For inode data, I've created 3 custom tags:
- `inode.mtime` is the last modified time
- `inode.atime` is the last access time
- `inode.ctime` is the creation time

Remember that metadata must contain a date, or timestamp will throw errors.

## Images
![timestamp](https://github.com/user-attachments/assets/63100609-7886-449e-8205-3c17f18d2424)


