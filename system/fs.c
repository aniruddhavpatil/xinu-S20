#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD]; // open file table
int next_open_fd = 0;

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (((fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock)
{
    int diskblock;

    if (fileblock >= INODEBLOCKS - 2)
    {
        printf("No indirect block support\n");
        return SYSERR;
    }

    diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

    return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in)
{
    int bl, inn;
    int inode_off;

    if (dev != 0)
    {
        printf("Unsupported device\n");
        return SYSERR;
    }
    if (inode_number > fsd.ninodes)
    {
        printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
        return SYSERR;
    }

    bl = inode_number / INODES_PER_BLOCK;
    inn = inode_number % INODES_PER_BLOCK;
    bl += FIRST_INODE_BLOCK;

    inode_off = inn * sizeof(struct inode);

    /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

    bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
    memcpy(in, &block_cache[inode_off], sizeof(struct inode));

    return OK;
}

/* write inode indicated by pointer to device */
int fs_put_inode_by_num(int dev, int inode_number, struct inode *in)
{
    int bl, inn;

    if (dev != 0)
    {
        printf("Unsupported device\n");
        return SYSERR;
    }
    if (inode_number > fsd.ninodes)
    {
        printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
        return SYSERR;
    }

    bl = inode_number / INODES_PER_BLOCK;
    inn = inode_number % INODES_PER_BLOCK;
    bl += FIRST_INODE_BLOCK;

    /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

    bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
    memcpy(&block_cache[(inn * sizeof(struct inode))], in, sizeof(struct inode));
    bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

    return OK;
}

/* create file system on device; write file system block and block bitmask to
 * device */
int fs_mkfs(int dev, int num_inodes)
{
    int i;

    if (dev == 0)
    {
        fsd.nblocks = dev0_numblocks;
        fsd.blocksz = dev0_blocksize;
    }
    else
    {
        printf("Unsupported device\n");
        return SYSERR;
    }

    if (num_inodes < 1)
    {
        fsd.ninodes = DEFAULT_NUM_INODES;
    }
    else
    {
        fsd.ninodes = num_inodes;
    }

    i = fsd.nblocks;
    while ((i % 8) != 0)
    {
        i++;
    }
    fsd.freemaskbytes = i / 8;

    if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR)
    {
        printf("fs_mkfs memget failed.\n");
        return SYSERR;
    }

    /* zero the free mask */
    for (i = 0; i < fsd.freemaskbytes; i++)
    {
        fsd.freemask[i] = '\0';
    }

    fsd.inodes_used = 0;

    /* write the fsystem block to SB_BLK, mark block used */
    fs_setmaskbit(SB_BLK);
    bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));

    /* write the free block bitmask in BM_BLK, mark block used */
    fs_setmaskbit(BM_BLK);
    bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

    return 1;
}

/* print information related to inodes*/
void fs_print_fsd(void)
{

    printf("fsd.ninodes: %d\n", fsd.ninodes);
    printf("sizeof(struct inode): %d\n", sizeof(struct inode));
    printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
    printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b)
{
    int mbyte, mbit;
    mbyte = b / 8;
    mbit = b % 8;

    fsd.freemask[mbyte] |= (0x80 >> mbit);
    return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b)
{
    int mbyte, mbit;
    mbyte = b / 8;
    mbit = b % 8;

    return (((fsd.freemask[mbyte] << mbit) & 0x80) >> 7);
    return OK;
}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b)
{
    int mbyte, mbit, invb;
    mbyte = b / 8;
    mbit = b % 8;

    invb = ~(0x80 >> mbit);
    invb &= 0xFF;

    fsd.freemask[mbyte] &= invb;
    return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void)
{ // print block bitmask
    int i, j;

    for (i = 0; i < fsd.freemaskbytes; i++)
    {
        for (j = 0; j < 8; j++)
        {
            printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
        }
        if ((i % 8) == 7)
        {
            printf("\n");
        }
    }
    printf("\n");
}

int fs_open(char *filename, int flags)
{
    int entryNode = -1;
    int i;
    struct inode in;
    int j;
    int new_index;

    if (flags != O_RDWR && flags != O_WRONLY && flags != O_RDONLY)
        return error_handler("Wrong file permissions in fs_open");

    for (i = 0; i < DIRECTORY_SIZE; i++)
    {
        if (strcmp(fsd.root_dir.entry[i].name, filename) == 0)
            if (fsd.root_dir.entry[i].inode_num >= 0)
                if (fsd.root_dir.entry[i].inode_num <= fsd.ninodes)
                {
                    entryNode = i;
                    break;
                }
    }

    if (entryNode == -1 || i >= DIRECTORY_SIZE)
        return error_handler("entryNode or i is out of bounds in fs_open");

    for (i = 0; i < NUM_FD; i++)
    {
        if (oft[i].de->inode_num == fsd.root_dir.entry[entryNode].inode_num)
            if (oft[i].de->name == fsd.root_dir.entry[entryNode].name)
            {
                if (oft[i].state == FSTATE_OPEN)
                    return error_handler("oft[i].state == FSTATE_OPEN in fs_open");
                else
                {
                    oft[i].state = FSTATE_OPEN;
                    return i;
                }
            }
            else if (oft[i].state == FSTATE_CLOSED)
                break;
    }

    if (i >= NUM_FD)
        return error_handler("Error in fs_open");

    if (fs_get_inode_by_num(0, oft[i].in.id, &in) == SYSERR)
        return SYSERR;

    oft[i].fileptr = 0;
    oft[i].de = &fsd.root_dir.entry[entryNode];
    oft[i].flag = flags;
    oft[i].in = in;
    oft[i].state = FSTATE_OPEN;

    return i;
}

int fs_close(int fd)
{

    if (fd < 0 || fd >= NUM_FD)
        return SYSERR;

    if (oft[fd].state == FSTATE_OPEN)
    {
        oft[fd].state = FSTATE_CLOSED;
        return OK;
    }
    return SYSERR;
}

int fs_create(char *filename, int mode)
{
    struct inode in;

    if (mode != O_CREAT)
        return SYSERR;

    if (strlen(filename) > FILENAMELEN)
        return SYSERR;

    if (fsd.inodes_used > fsd.ninodes)
        return SYSERR;

    int i = fsd.inodes_used;

    for (; i < fsd.ninodes; i++)
    {
        if (fs_get_inode_by_num(0, i, &in) == OK)
        {
            in.type = INODE_TYPE_FILE;
            in.device = 0;
            in.nlink = 1;
            in.size = 0;
            in.id = i;

            if (fs_put_inode_by_num(0, i, &in) == SYSERR)
                return SYSERR;
            break;
        }
    }

    if (i >= fsd.ninodes)
        return SYSERR;
    int n_entries = fsd.root_dir.numentries;
    fsd.root_dir.entry[n_entries].inode_num = i;
    strcpy(fsd.root_dir.entry[n_entries].name, filename);

    fsd.root_dir.numentries = ++n_entries;
    fsd.inodes_used += 1;

    int j = 0;
    for (; j < NUM_FD; j++)
    {
        if (oft[j].state == FSTATE_CLOSED)
        {
            oft[j].de = &fsd.root_dir.entry[i];
            oft[j].state = FSTATE_OPEN;
            oft[j].flag = O_RDWR;
            oft[j].in = in;
            oft[j].fileptr = 0;
            break;
        }
    }

    if (j >= NUM_FD)
        return SYSERR;
    return j;
}

int fs_seek(int fd, int offset)
{
    if (fd < 0 || fd >= NUM_FD)
        return SYSERR;

    if (oft[fd].state == FSTATE_OPEN)
    {
        int bound = oft[fd].fileptr + offset;
        if (bound < 0 || bound > oft[fd].in.size)
            return SYSERR;
        oft[fd].fileptr = bound;
        return oft[fd].fileptr;
    }
    return OK;
}

int fs_read(int fd, void *buf, int nbytes)
{

    if (fd < 0 || fd >= NUM_FD)
        return SYSERR;

    if (oft[fd].state != FSTATE_OPEN)
        return SYSERR;
    if (oft[fd].flag != O_RDWR && oft[fd].flag != O_RDONLY)
        return SYSERR;
    if (nbytes <= 0)
        return SYSERR;

    struct inode in;
    int inodeNum = 0;
    int blocksNum = 0;
    int startBlock = 0;
    int bytesToRead = 0;
    int bytesRead = 0;
    int startOffset = 0;

    inodeNum = oft[fd].de->inode_num;

    if ((fs_get_inode_by_num(0, inodeNum, &in)) == SYSERR)
        return SYSERR;
    if (nbytes > in.size)
        nbytes = in.size;

    blocksNum = nbytes / MDEV_BLOCK_SIZE;
    if (nbytes % MDEV_BLOCK_SIZE)
        blocksNum++;

    int i;
    for (i = 0; i < blocksNum; i++)
    {
        if (i != (blocksNum - 1))
            bytesToRead = MDEV_BLOCK_SIZE;
        else
            bytesToRead = nbytes - bytesRead;
        bs_bread(dev0, in.blocks[i], oft[fd].fileptr, &buf[bytesRead], bytesToRead);
        bytesRead += bytesToRead;
    }

    return bytesRead;
}

int fs_write(int fd, void *buf, int nbytes)
{

    if (fd < 0 || fd >= NUM_FD)
        return SYSERR;

    if (oft[fd].state != FSTATE_OPEN)
        if (oft[fd].flag != O_RDWR && oft[fd].flag != O_WRONLY)
            return SYSERR;

    if (!nbytes)
        return SYSERR;

    int inodeNum = 0;
    int numOfBlocksToWrite = 0;
    int noOfBlocks = 0;
    int writeBlock = 0;
    int assignedBlocks = 0;
    int bytesToWrite = 0;
    int bytesWritten = 0;
    int startOffset = 0;
    struct inode in;
    inodeNum = oft[fd].de->inode_num;

    numOfBlocksToWrite = (nbytes + (oft[fd].fileptr % MDEV_BLOCK_SIZE)) / MDEV_BLOCK_SIZE;
    if ((nbytes + oft[fd].fileptr) % MDEV_BLOCK_SIZE)
        numOfBlocksToWrite++;
    if ((fs_get_inode_by_num(0, inodeNum, &in)) == SYSERR)
        return SYSERR;
    noOfBlocks = (in.size % MDEV_BLOCK_SIZE);

    if ((in.size % MDEV_BLOCK_SIZE) != 0)
        noOfBlocks++;

    writeBlock = noOfBlocks;
    int i = 15;
    for (; i < MDEV_NUM_BLOCKS; i++)
    {
        if (fs_getmaskbit(i) == 0)
        {
            fs_setmaskbit(i);
            in.blocks[assignedBlocks] = i;

            if (writeBlock == noOfBlocks)
            {
                startOffset = oft[fd].fileptr % MDEV_BLOCK_SIZE;
                int intermBytes = oft[fd].fileptr % MDEV_BLOCK_SIZE;
                bytesToWrite = MDEV_BLOCK_SIZE - intermBytes;
            }
            else if (writeBlock == numOfBlocksToWrite - 1)
            {
                startOffset = 0;
                bytesToWrite = nbytes - bytesWritten;
            }
            else
            {
                startOffset = 0;
                bytesToWrite = MDEV_BLOCK_SIZE;
            }

            bs_bwrite(dev0, in.blocks[writeBlock], startOffset, &buf[bytesWritten], bytesToWrite);
            ++assignedBlocks;
            ++writeBlock;
            bytesWritten += bytesToWrite;
            if (assignedBlocks == numOfBlocksToWrite)
                break;
        }
    }

    if (assignedBlocks != numOfBlocksToWrite)
        return SYSERR;

    in.size += bytesWritten;
    oft[fd].fileptr += bytesWritten;

    if ((fs_put_inode_by_num(0, inodeNum, &in)) == SYSERR)
        return SYSERR;
    return bytesWritten;
}

int fs_link(char *src_filename, char *dst_filename)
{
    if (strlen(src_filename) > FILENAMELEN || strlen(dst_filename) > FILENAMELEN)
        return SYSERR;
    int temp_inode_num = -1;
    int i = 0;
    int n_entries = fsd.root_dir.numentries;
    for (; i < n_entries; i++)
        if (strncmp(fsd.root_dir.entry[i].name, src_filename, FILENAMELEN) == 0)
            temp_inode_num = fsd.root_dir.entry[i].inode_num;

    if (temp_inode_num == -1)
        return SYSERR;

    fsd.root_dir.entry[n_entries].inode_num = temp_inode_num;
    strcpy(fsd.root_dir.entry[n_entries].name, dst_filename);

    struct inode in;
    ++fsd.root_dir.numentries;

    if (fs_get_inode_by_num(0, temp_inode_num, &in) != SYSERR)
    {
        ++in.nlink;
        if (fs_put_inode_by_num(0, temp_inode_num, &in) == SYSERR)
            return SYSERR;
    }
    else
        return SYSERR;
    return OK;
}

int fs_unlink(char *filename)
{
    if (strlen(filename) > FILENAMELEN)
        return SYSERR;

    int fileEntry = -1;
    int temp_inode_num = -1;
    int i = 0;

    for (; i <= fsd.root_dir.numentries; i++)
    {
        if (strncmp(fsd.root_dir.entry[i].name, filename, FILENAMELEN) == 0)
        {
            temp_inode_num = fsd.root_dir.entry[i].inode_num;
            fileEntry = i;
            break;
        }
    }

    if (i > fsd.root_dir.numentries)
        return SYSERR;

    struct inode in;
    if (fs_get_inode_by_num(0, temp_inode_num, &in) != SYSERR)
    {
        if (in.nlink == 1)
        {
            int j = 0;
            int blocks = in.size / MDEV_BLOCK_SIZE;
            if (in.size % MDEV_BLOCK_SIZE != 0)
                ++blocks;

            while (j < blocks)
                fs_clearmaskbit(in.blocks[j++]);
            --fsd.inodes_used;
        }
        --in.nlink;
        if (fs_put_inode_by_num(0, temp_inode_num, &in) == SYSERR)
            return SYSERR;
    }
    else
        return SYSERR;
    int k = 0;
    while (k < NUM_FD)
    {
        if (fileEntry != -1 && oft[k].de->inode_num == fsd.root_dir.entry[fileEntry].inode_num)
        {
            if (oft[k].state == FSTATE_OPEN)
            {
                oft[k].state = FSTATE_CLOSED;
                break;
            }
        }
        ++k;
    }
    k = i;
    while (k < fsd.root_dir.numentries)
    {
        fsd.root_dir.entry[k] = fsd.root_dir.entry[k + 1];
        // kprintf("\nK in unlink is %d\n", k);
        ++k;
    }
    --fsd.root_dir.numentries;
    --fsd.root_dir.entry[fileEntry].inode_num;

    return OK;
}
int error_handler(const char* error_message){
    printf("%s\n", error_message);
    return SYSERR;
}

#endif /* FS */