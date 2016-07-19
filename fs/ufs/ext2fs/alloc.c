
#include <fs/vnode.h>
#include <fs/bio.h>
#include <fs/mount.h>
#include <fs/vfs.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <bitmap.h>
#include <panic.h>
#include <errno.h>

/*
 * A simple brute-force search algorithm for available inode number.
 * Returns VGET'd vnode.
 * Changes in-memory superblock (including group descriptors) as well
 * as in-memory inode structure.
 * Changes and writes inode bitmap to disk.
 */
int
ext2fs_inode_alloc(struct vnode *dvp, int imode, struct vnode **vpp)
{
	struct vnode *devvp = VFSTOUFS(dvp->mount)->devvp;
	struct vnode *vp;	/* the vnode of allocated inode */
	struct inode *pip;	/* inode of @dvp */
	struct inode *ip;	/* the allocated inode */
	struct m_ext2fs *fs;
	struct buf *bp_ibitmap;
	ufsino_t ino = 0, ibase;
	int err, avail, i;

	pip = VTOI(dvp);
	fs = pip->superblock;
	if (fs->e2fs.ficount == 0)
		return -ENOSPC;

	for (i = 0; i < fs->ncg; ++i) {
		ibase = fs->e2fs.ipg * i;
		err = bread(devvp, fsbtodb(fs, fs->gd[i].i_bitmap), fs->bsize,
		    &bp_ibitmap);
		if (err) {
			brelse(bp_ibitmap);
			return err;
		}
		/* Search for available ino in this cylinder group.
		 * Note that bitmap_xxx are 1-based. */
		avail = bitmap_find_first_zero_bit(bp_ibitmap->data,
		    fs->e2fs.ipg);
		while (avail < EXT2_FIRSTINO && avail != 0 && i == 0) {
			/* if ino is below FIRSTINO, discard and continue */
			avail = bitmap_find_next_zero_bit(bp_ibitmap->data,
			    fs->e2fs.ipg, avail);
		}
		if (avail == 0) {
			brelse(bp_ibitmap);
			continue;
		}
		ino = avail + ibase;
		break;
		/* NOTREACHED */
	}
	if (ino == 0) {
		/* No free inodes in bitmap but superblock says there ARE free
		 * inodes.  Here, we don't attempt to fix it. */
		kprintf("%s: inconsistent inode bitmap and superblock\n", __func__);
		return -ENOSPC;
	}

	/* We found a free inode # @ino, but we delay modifying superblock and
	 * group descriptors as we need to VGET() and enforce checks first */
	err = VFS_VGET(dvp->mount, ino, &vp);
	if (err)
		goto rollback_ibitmap;
	ip = VTOI(vp);
	if (EXT2_DINODE(ip)->mode && EXT2_DINODE(ip)->nlink > 0)
		/* It's an unresolvable file system error... */
		panic("%s: dup alloc mode 0%o, nlinks %u, inum %u\n",
		    __func__, EXT2_DINODE(ip)->mode, EXT2_DINODE(ip)->nlink,
		    ip->ino);
	memset(EXT2_DINODE(ip), 0, sizeof(*EXT2_DINODE(ip)));

	/* Now we do the writes */
	atomic_set_bit(avail, bp_ibitmap->data);
	fs->e2fs.ficount--;
	fs->gd[i].nifree--;
	fs->fmod = 1;
	if ((imode & EXT2_IFMT) == EXT2_IFDIR)
		fs->gd[i].ndirs++;
	err = bwrite(bp_ibitmap);
	if (err)
		goto rollback_vget;

	brelse(bp_ibitmap);
	*vpp = vp;
	return 0;

rollback_vget:
	vput(vp);
rollback_ibitmap:
	brelse(bp_ibitmap);
	return err;
}

/*
 * Does the reverse of ext2fs_inode_alloc.
 * Changes in-memory superblock.
 * Does NOT change in-memory inode (do we need to?)
 * Changes and writes inode bitmap to disk
 */
void
ext2fs_inode_free(struct vnode *dvp, ufsino_t ino, int imode)
{
	struct inode *ip = VTOI(dvp);
	struct m_ext2fs *fs;
	void *ibp;
	struct buf *bp;
	int err, cg;

	fs = ip->superblock;
	assert(ino <= fs->e2fs.icount);
	assert(ino >= EXT2_FIRSTINO);
	cg = ino_to_cg(fs, ino);
	err = bread(ip->ufsmount->devvp, fsbtodb(fs, fs->gd[cg].i_bitmap),
	    fs->bsize, &bp);
	if (err) {
		brelse(bp);
		return;
	}
	ibp = bp->data;
	ino = (ino - 1) % fs->e2fs.ipg + 1;
	if (!bitmap_test_bit(ino, ibp))
		panic("%s: freeing free inode %u on %x\n", __func__, ino,
		    ip->devno);
	atomic_clear_bit(ino, ibp);
	fs->e2fs.ficount++;
	fs->gd[cg].nifree++;
	if ((imode & EXT2_IFMT) == EXT2_IFDIR)
		fs->gd[cg].ndirs--;
	fs->fmod = 1;
	bwrite(bp);
	brelse(bp);
}
