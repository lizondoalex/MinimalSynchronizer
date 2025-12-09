#  MinimalSynchronizer

A simple, command-line utility written in C for basic file synchronization between a local host and a remote server using `rsync` over SSH, with a focus on tracking the last synchronization time using a local configuration file.

## Features

* **Host-to-Server Sync (`save`):** Replaces all files on the server directory with the files from the host directory using `rsync --delete`. It also updates the last synchronization time on both the host's and the server's configuration file.
* **Server-to-Host Sync (`load`):** Replaces all files on the host directory with the files from the server directory using `rsync --delete`. It also updates the host's synchronization time to match the server's.
* **Status Check (`status`):** Compares the last synchronization date stored in the host's config file against the date in the server's config file to determine which side is outdated.
* **Difference Check (`diff`):** Uses `rsync`'s dry-run feature (`-n`) to show which files are new, modified, or deleted on the host relative to the server **without** performing any synchronization.
* **Time Update (`update`):** Updates the local synchronization time based on the timestamp of the *most recently modified file* in the client directory. Can also manually change the date if an extra parameter is passed containing a base64 representation of a json time object.
* **Configuration:** Stores synchronization settings (IP, host directory, server directory, last sync time) in a JSON configuration file in ~/.config/minsync/config.jsonc.

## Installation

### Prerequisites

To compile and run `MinimalSynchronizer`, you need:
1.  A C compiler (e.g., `gcc`).
2.  The `json-c` library (for handling JSON configuration).
3.  `rsync` and `ssh` available on your system and server and configured for password-less login between the host and server.

### Build and Install

```bash
# Clone the repository
git clone [https://github.com/yourusername/MinimalSynchronizer.git](https://github.com/yourusername/MinimalSynchronizer.git)
cd MinimalSynchronizer

# Compile the project 
./build.sh

# Install the executable 
sudo ./install.sh
