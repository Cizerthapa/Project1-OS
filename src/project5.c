#include "project5.h"

#if PROJECT == 5
int kernel()
{
    startkernel(startshell);
    return 0;
}
#endif

int createSubdirectory(char *subdirectoryName)
{
    if (!currentDirectory.isOpened)
    {
        print("Err No directory is current on opened");
        return 1;
    }

    // Walk entries to find the first empty slot and count how many exist
    directory_entry_t *emptyEntry = (directory_entry_t *)currentDirectory.startingAddress;
    uint32 entryCount = 1;

    while (emptyEntry->filename[0] != 0)
    {
        emptyEntry++;
        entryCount++;
    }

    // If the directory is full, resize it (root cannot be resized)
    uint32 directorySizeNeeded = entryCount * 32;
    uint32 directorySize = currentDirectory.directoryEntry->fileSize;
    char directoryIsTooSmall = directorySizeNeeded > directorySize;
    char isRootDirectory = currentDirectory.directoryEntry->startingCluster == 0;

    if (directoryIsTooSmall && !isRootDirectory)
    {
        int resizeFailed = resizeFile(currentDirectory.directoryEntry->startingCluster, directorySizeNeeded);
        if (resizeFailed)
        {
            print("Err Could not resize the directory when creating new subdirectory");
            return 1;
        }
    }

    if (directoryIsTooSmall && isRootDirectory)
    {
        print("Err The root directory is full");
        return 1;
    }

    // Allocate a free FAT cluster for the new subdirectory and mark it as end of chain
    uint16 newCluster = nextFreeCluster();
    if (newCluster == 0xFFFF)
    {
        print("Err Disk is full");
        return 1;
    }

    fat0->clusters[newCluster] = 0xFFFF;
    fat1->clusters[newCluster] = 0xFFFF;

    if (writeFATs())
    {
        print("Err Failed to write FATs");
        return 1;
    }

    // Write the new subdirectory's entry into the parent directory
    stringcopy("        ", (char *)emptyEntry->filename, 8);
    stringcopy("   ", (char *)emptyEntry->ext, 3);
    stringcopy(subdirectoryName, (char *)emptyEntry->filename, 8);
    emptyEntry->attributes = 0x10;
    emptyEntry->startingCluster = newCluster;
    emptyEntry->fileSize = 512;

    if (writeCurrentDirectory())
    {
        print("Err Failed to write parent directory");
        return 1;
    }

    // Build the new subdirectory's first sector in a local buffer
    uint8 buffer[512];
    for (int i = 0; i < 512; i++)
        buffer[i] = 0;

    // "." points to this directory itself
    directory_entry_t *dotEntry = (directory_entry_t *)buffer;
    stringcopy(".       ", (char *)dotEntry->filename, 8);
    stringcopy("   ", (char *)dotEntry->ext, 3);
    dotEntry->attributes = 0x10;
    dotEntry->startingCluster = newCluster;
    dotEntry->fileSize = 512;

    // ".." points to the parent (cluster 0 means the parent is root)
    directory_entry_t *dotDotEntry = dotEntry + 1;
    stringcopy("..      ", (char *)dotDotEntry->filename, 8);
    stringcopy("   ", (char *)dotDotEntry->ext, 3);
    dotDotEntry->attributes = 0x10;
    dotDotEntry->startingCluster = currentDirectory.directoryEntry->startingCluster;
    dotDotEntry->fileSize = 512;

    // Write the buffer to the subdirectory's cluster on disk
    uint32 sector = newCluster + CLUSTER_TO_SECTOR;
    if (floppy_write(0, sector, (void *)buffer, 512))
    {
        print("Err Failed to write subdirectory sector");
        return 1;
    }

    return 0;
}

int openDirectory(char *directoryName)
{
    if (!currentDirectory.isOpened)
    {
        print("Err No directory is currently opened");
        return 1;
    }

    if (currentFile.isOpened)
    {
        print("Err Cannot change directory while a file is open");
        return 1;
    }

  // Find the directory with the given name in the current directory.
  // Check that its attribute is 0x10 to make sure it is a folder, not a file.
  // directory_entry_t *entry = (directory_entry_t *)currentDirectory.startingAddress;
    char found = 0;

    while (entry->filename[0] != 0)
    {
        if (stringcompare((char *)entry->filename, directoryName, 8) &&
            (entry->attributes & 0x10))
        {
            found = 1;
            break;
        }
        entry++;
    }

    if (!found)
    {
        print("Err Directory not found");
        return 1;
    }

    uint16 startingCluster = entry->startingCluster;

// Check if the directory's starting cluster is 0, which means it is the root directory.
// This happens when we use "cd .." to go back to root from a subdirectory.    if (startingCluster == 0)
    {
        currentDirectory.directoryEntry = &rootDirectoryEntry;
        if (floppy_read(0, ROOT_DIR_SECTOR, (void *)currentDirectory.startingAddress, ROOT_SIZE))
        {
            print("Err Failed to read root directory");
            return 1;
        }
        return 0;
    }

    // Follow the FAT chain and read each sector of the directory into memory
    // Use the FAT to find each cluster that belongs to the directory.
    // Read the data from each cluster and store it in memory.
    uint16 cluster = startingCluster;
    uint16 sectorCount = 0;

    while (cluster != 0xFFFF)
    {
        uint32 sector = cluster + CLUSTER_TO_SECTOR;
        if (floppy_read(0, sector, (void *)currentDirectory.startingAddress + (512 * sectorCount), 512))
        {
            print("Err Failed to read directory sector");
            return 1;
        }
        sectorCount++;
        cluster = fat0->clusters[cluster];
    }

    // The first entry in the directory is always ".", which holds its own metadata
    currentDirectory.directoryEntry = (directory_entry_t *)currentDirectory.startingAddress;

    return 0;
}