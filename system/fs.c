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

int fs_open(char *filename, int mode)
{
    if (strlen(filename) == 0)
        return SYSERR;

    int i = 0;
    int n_entries = fsd.root_dir.numentries;
    for (; i < n_entries; i++)
    {
        if (strcmp(filename, fsd.root_dir.entry[i].name) == 0)
        {
            break;
        }
    }
    if (i == n_entries) return SYSERR;

    int fd = -1;
    for (int j = 0; j < NUM_FD; j++)
    {
        if (strcmp(fsd.root_dir.entry[i].name, oft[j].de->name) == 0)
        {
            fd = j;
            break;
        }
    }

    if (fd < 0) return SYSERR;
    if (oft[fd].state != FSTATE_CLOSED) return SYSERR;
    struct inode in;
    if (fs_get_inode_by_num(0, oft[fd].in.id, &in) == SYSERR) return SYSERR;

    oft[fd].fileptr = 0;
    oft[fd].in = in;
    oft[fd].flag = mode;
    oft[fd].de = &fsd.root_dir.entry[i];
    oft[fd].state = FSTATE_OPEN;

    return fd;
}

int fs_close(int fd)
{
    if (fd < 0 || fd > NUM_FD){
        // printf("File descriptor %d is out of bounds.\n", fd);
        return SYSERR;
    }
    if (oft[fd].state == FSTATE_CLOSED){
        // printf("\nFile already closed.\n");
        return SYSERR;
    }
    oft[fd].state = FSTATE_CLOSED;
    oft[fd].fileptr = 0;

    return OK;
}

int fs_create(char *filename, int mode)
{
    if (mode != O_CREAT)
        return SYSERR;
    if (strlen(filename) == 0)
        return SYSERR;
    if (strlen(filename) > FILENAMELEN)
        return SYSERR;

    int n_entries = fsd.root_dir.numentries;
    int file_num = 0;
    for (; file_num < n_entries; file_num++)
    {
        if (strcmp(filename, fsd.root_dir.entry[file_num].name) == 0)
            break;
    }

    if (file_num != n_entries)
        return SYSERR;
    if (fsd.inodes_used >= fsd.ninodes)
        return SYSERR;
    struct inode in;
    int get_inode_status = fs_get_inode_by_num(0, ++fsd.inodes_used, &in);
    if (get_inode_status == SYSERR)
        return SYSERR;

    in.id = fsd.inodes_used;
    in.size = 0;
    in.device = 0;
    in.nlink = 1;
    in.type = INODE_TYPE_FILE;

    int put_inode_status = fs_put_inode_by_num(0, file_num, &in);

    if (put_inode_status == SYSERR)
        return SYSERR;
    strcpy(fsd.root_dir.entry[n_entries].name, filename);
    fsd.root_dir.entry[n_entries].inode_num = file_num;
    oft[fsd.inodes_used].state = FSTATE_OPEN;
    oft[fsd.inodes_used].in = in;
    oft[fsd.inodes_used].de = &fsd.root_dir.entry[n_entries];
    oft[fsd.inodes_used].flag = O_RDWR;
    oft[fsd.inodes_used].fileptr = 0;
    fsd.root_dir.numentries++;

    return fsd.inodes_used;
}

int fs_seek(int fd, int offset)
{
    if (fd < 0 || fd > NUM_FD)
        return SYSERR;
    if (oft[fd].state == FSTATE_CLOSED)
        return SYSERR;
    if ((oft[fd].fileptr + offset) < 0)
        return SYSERR;
    oft[fd].fileptr += offset;
    return oft[fd].fileptr;
}

int fs_read(int fd, void *buf, int nbytes)
{
    if (fd < 0 || fd > NUM_FD)
        return SYSERR;
    if (oft[fd].state == FSTATE_CLOSED)
        return SYSERR;
    if (!(oft[fd].flag == O_RDONLY || oft[fd].flag == O_RDWR))
        return SYSERR;
    if (nbytes <= 0)
        return SYSERR;
    if (oft[fd].in.size == 0)
        return SYSERR;

    nbytes += oft[fd].fileptr;
    int blocksToRead = nbytes / MDEV_BLOCK_SIZE;
    if ((nbytes % MDEV_BLOCK_SIZE) != 0) blocksToRead++;

    if (oft[fd].in.size < blocksToRead)
        blocksToRead = oft[fd].in.size;

    int blockNum = (oft[fd].fileptr / MDEV_BLOCK_SIZE);
    memset(buf, NULL, (MDEV_BLOCK_SIZE * MDEV_NUM_BLOCKS));

    int bytesRead = 0;
    int offset = (oft[fd].fileptr % MDEV_BLOCK_SIZE);

    for (; blockNum < blocksToRead; blockNum++, offset = 0)
    {
        memset(block_cache, NULL, MDEV_BLOCK_SIZE + 1);
        int bs_status = bs_bread(0, oft[fd].in.blocks[blockNum], offset, block_cache, MDEV_BLOCK_SIZE - offset);
        if (bs_status == SYSERR)
            return SYSERR;
        strcpy((buf + bytesRead), block_cache);
        bytesRead = strlen(buf);
    }
    oft[fd].fileptr = bytesRead;
    return bytesRead;
}

int fs_write(int fd, void *buf, int nbytes)
{
    if (fd < 0 || fd > NUM_FD)
        return SYSERR;
    if (oft[fd].state == FSTATE_CLOSED)
        return SYSERR;
    if (oft[fd].flag == O_RDONLY)
        return SYSERR;

    // if (nbytes <= 0 || (strlen((char *)buf) == 0) || strlen((char *)buf) != nbytes)
        // return SYSERR;

    struct inode tempiNode;
    if (oft[fd].in.size > 0)
    {
        // tempiNode = oft[fd].in;
        while(oft[fd].in.size--)
        {
            if (fs_clearmaskbit(oft[fd].in.blocks[oft[fd].in.size - 1]) != OK)
                return SYSERR;
        }
    }

    int blocksToWrite = nbytes / MDEV_BLOCK_SIZE;
    if (nbytes % MDEV_BLOCK_SIZE) blocksToWrite++;

    int bytesToWrite = nbytes;
    int blockNum = FIRST_INODE_BLOCK + NUM_INODE_BLOCKS;
    for (int i = 0; ((i < blocksToWrite) && (blockNum < MDEV_BLOCK_SIZE)); blockNum++, i++)
    {
        if (fs_getmaskbit(blockNum) == 0)
        {
            memset(block_cache, NULL, MDEV_BLOCK_SIZE);

            if (bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
                return SYSERR;

            int minBytes = bytesToWrite;
            if (bytesToWrite > MDEV_BLOCK_SIZE) minBytes = MDEV_BLOCK_SIZE;

            memcpy(block_cache, buf, minBytes);

            if (bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
                return SYSERR;

            buf = (char *)buf + minBytes;
            bytesToWrite = bytesToWrite - minBytes;
            fs_setmaskbit(blockNum);
            oft[fd].in.blocks[i] = blockNum;
        }
    }

    oft[fd].in.size = blocksToWrite;

    int put_inode_status = fs_put_inode_by_num(0, oft[fd].in.id, &oft[fd].in);

    if (put_inode_status == SYSERR)
        return SYSERR;
    oft[fd].fileptr = nbytes;
    return nbytes;
}

int fs_link(char *src_filename, char *dst_filename)
{
    if (strlen(src_filename) == 0 || strlen(dst_filename) == 0)
        return SYSERR;
    if (strlen(src_filename) > FILENAMELEN || strlen(dst_filename) > FILENAMELEN)
        return SYSERR;

    int n_entries = fsd.root_dir.numentries;
    int src_file_num = 0;
    for (; src_file_num < n_entries; src_file_num++)
        if (strcmp(src_filename, fsd.root_dir.entry[src_file_num].name) == 0) 
            break;

    int dst_file_num = 0;
    for (; dst_file_num < n_entries; dst_file_num++)
        if (strcmp(dst_filename, fsd.root_dir.entry[dst_file_num].name) == 0)
            break;
    // fsd overflow???
    // if (dst_file_num != n_entries) return SYSERR;
    // if (fsd.inodes_used >= fsd.ninodes) return SYSERR;
    
    struct inode src_in;
    int get_inode_status = fs_get_inode_by_num(0, fsd.root_dir.entry[src_file_num].inode_num, &src_in);
    if (get_inode_status == SYSERR) return SYSERR;

    src_in.nlink++;

    int put_inode_status = fs_put_inode_by_num(0, ++fsd.inodes_used, &src_in);
    if (put_inode_status == SYSERR) return SYSERR;

    // Update src inode???
    // put_inode_status = fs_put_inode_by_num(0, fsd.root_dir.entry[src_file_num].inode_num, &src_in);
    // if (put_inode_status == SYSERR) return SYSERR;

    // put_inode_status = fs_put_inode_by_num(0, src_file_num, &src_in);
    strcpy(fsd.root_dir.entry[dst_file_num].name, dst_filename);
    fsd.root_dir.entry[dst_file_num].inode_num = fsd.inodes_used;
    // oft[fsd.inodes_used].state = FSTATE_OPEN;
    // oft[fsd.inodes_used].in = src_in;
    // oft[fsd.inodes_used].de = &fsd.root_dir.entry[n_entries];
    // oft[fsd.inodes_used].flag = O_RDWR;
    // oft[fsd.inodes_used].fileptr = 0;
    fsd.root_dir.numentries++;

    

    return OK;
}

int fs_unlink(char *filename)
{
    int n_entries = fsd.root_dir.numentries;
    int file_num = 0;
    
    for (; file_num < n_entries; file_num++)
    {
        if (strcmp(filename, fsd.root_dir.entry[file_num].name) == 0)
            break;
    }
    if (file_num == n_entries) return SYSERR;
    int fd = -1;
    for (int j = 0; j < NUM_FD; j++)
    {
        if (strcmp(fsd.root_dir.entry[file_num].name, oft[j].de->name) == 0)
        {
            fd = j;
            break;
        }
    }
    if (fd >= 0 && fd <= NUM_FD) oft[fd].state = FSTATE_CLOSED;
    struct inode in;
    int get_inode_status = fs_get_inode_by_num(0, fsd.root_dir.entry[file_num].inode_num, &in);
    if (get_inode_status == SYSERR) return SYSERR;
    if (in.nlink > 1){
        fsd.root_dir.entry[file_num].name[0] = '\0';
        fsd.root_dir.numentries--;
        in.nlink--;

        fs_put_inode_by_num(0, fsd.root_dir.entry[file_num].inode_num, &in);
    }
    else{
        for(int i = 0; i < in.size; i++){
            fs_clearmaskbit(in.blocks[i]);
            fsd.root_dir.entry[file_num].name[0] = '\0';
            fsd.root_dir.numentries--;
            in.nlink--;
            fs_put_inode_by_num(0, fsd.root_dir.entry[file_num].inode_num, &in);
        }
    }
    return OK;
}
#endif /* FS */