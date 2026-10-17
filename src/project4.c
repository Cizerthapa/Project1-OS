#include "project4.h"

// FAT Copies
// First copy is fat0 stored at 
// Second copy is fat1
// There were issues declaring the FATs as non-pointers
// When they would get read from floppy, it would overwrite wrong areas of memory
fat_t *fat0;
fat_t *fat1;

directory_t currentDirectory;  // The current directory we have opened
directory_entry_t rootDirectoryEntry;   // The root directory's directory entry (this does not exist on the disk since the root is not inside of another directory)
file_t currentFile;            // The current file we have opened

#if PROJECT == 4
int kernel()
{
    startkernel(startshell);
    return 0;
}
#endif

int writeFATs()
{
    // Comparing both FAT copies and returning error if they don't match
    for (int i = 0; i < FAT_ENTRIES_MAX; i++)
    {
        if (fat0->clusters[i] != fat1->clusters[i])
        {
            print("Error Something happened");
            return 1;
        }
    }

    // Write FAT 0 to sectors 1-11
    for (int i = 0; i < FAT_SIZE / 512; i++)
    {
        int error = floppy_write(0, FAT0_SECTOR + i, (void *)fat0 + (512 * i), 512);
        if (error)
        {
            print("Error Something happened");
            return 1;
        }
    }

    // Write FAT 1 to sectors 12-22
    for (int i = 0; i < FAT_SIZE / 512; i++)
    {
        int error = floppy_write(0, FAT1_SECTOR + i, (void *)fat1 + (512 * i), 512);
        if (error)
        {
            print("Error Something happened");
            return 1;
        }
    }

    return 0;
}

int writeCurrentDirectory()
{
    // Check whether the current directory is opened
    if (!currentDirectory.isOpened)
    {
        print("Error: No directory is currently opened!\n");
        return 1;
    }

    // If it is the root directory, write it to sector 23
    if (currentDirectory.directoryEntry->startingCluster == 0)
    {
        int error = floppy_write(0, ROOT_DIR_SECTOR, (void *)currentDirectory.startingAddress, ROOT_SIZE);
        if (error)
        {
            print("Error: Failed to write the root directory!\n");
            return 1;
        }
        return 0;
    }

    // Other mwise follow the FAT chain and write each cluster to its sector
    uint16 cluster = currentDirectory.directoryEntry->startingCluster;
    uint16 sectorCount = 0;

    while (cluster != 0xFFFF)
    {
        uint32 sector = cluster + CLUSTER_TO_SECTOR;
        int error = floppy_write(0, sector, (void *)currentDirectory.startingAddress + (512 * sectorCount), 512);
        if (error)
        {
            print("Error: Failed to write the directory!\n");
            return 1;
        }
        sectorCount++;
        cluster = fat0->clusters[cluster];
    }

    return 0;
}

int writeCurrentFile()
{
    // Checking whether the current file is opened
    if (!currentFile.isOpened)
    {
        print("Error: No file is currently opened!\n");
        return 1;
    }

    // Following FAT chain and writing each cluster to its sector
    uint16 cluster = currentFile.directoryEntry->startingCluster;
    uint16 sectorCount = 0;

    while (cluster != 0xFFFF)
    {
        uint32 sector = cluster + CLUSTER_TO_SECTOR;
        int error = floppy_write(0, sector, (void *)currentFile.startingAddress + (512 * sectorCount), 512);
        if (error)
        {
            print("Error: Failed to write the file!\n");
            return 1;
        }
        sectorCount++;
        cluster = fat0->clusters[cluster];
    }

    return 0;
}

int resizeFile(uint16 startingCluster, uint32 newFileSize)
{
    // Calculating the number of clusters needed
    uint32 clustersNeeded = (newFileSize + 511) / 512;

    // Validating that the number is between 1 and FAT_ENTRIES_MAX
    if (clustersNeeded < 1 || clustersNeeded > FAT_ENTRIES_MAX)
    {
        print("Error: Invalid number of clusters needed!\n");
        return 1;
    }

    // Count the currently allocated clusters by following the FAT chain
    uint32 currentCount = 0;
    uint16 cluster = startingCluster;
    while (cluster != 0xFFFF)
    {
        currentCount++;
        cluster = fat0->clusters[cluster];
    }

    // If the count is unchanged, return 0
    if (currentCount == clustersNeeded)
    {
        return 0;
    }

    // If shrinking, marking the new last cluster as 0xFFFF and freeing the rest
    if (clustersNeeded < currentCount)
    {
        // Walking to the new last cluster
        cluster = startingCluster;
        for (uint32 i = 1; i < clustersNeeded; i++)
        {
            cluster = fat0->clusters[cluster];
        }

        // Free all clusters after the new last cluster
        uint16 toFree = fat0->clusters[cluster];
        fat0->clusters[cluster] = 0xFFFF;
        fat1->clusters[cluster] = 0xFFFF;

        while (toFree != 0xFFFF)
        {
            uint16 next = fat0->clusters[toFree];
            fat0->clusters[toFree] = 0x0000;
            fat1->clusters[toFree] = 0x0000;
            toFree = next;
        }
    }
    else
    {
        // If growing, check that enough free clusters exist
        uint32 freeCount = 0;
        for (int i = 2; i < FAT_ENTRIES_MAX; i++)
        {
            if (fat0->clusters[i] == 0) freeCount++;
        }

        if (freeCount < (clustersNeeded - currentCount))
        {
            print("Error: Not enough free clusters to grow the file!\n");
            return 1;
        }

        // Walking to the current last cluster
        cluster = startingCluster;
        while (fat0->clusters[cluster] != 0xFFFF)
        {
            cluster = fat0->clusters[cluster];
        }

        // Allocate new clusters and extend the FAT chain
        for (uint32 i = currentCount; i < clustersNeeded; i++)
        {
            uint16 newCluster = nextFreeCluster();
            fat0->clusters[cluster] = newCluster;
            fat1->clusters[cluster] = newCluster;
            fat0->clusters[newCluster] = 0xFFFF;
            fat1->clusters[newCluster] = 0xFFFF;
            cluster = newCluster;
        }
    }

    // Persistance FAT changes to disk
    return writeFATs();
}