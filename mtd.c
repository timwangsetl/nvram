int mtd_erase(const char *mtd)
{
	int mtd_fd;
	mtd_info_t mtd_info;
	erase_info_t erase_info;
	
	/* Open MTD device */
	if ((mtd_fd = open(mtd, O_RDWR)) < 0) {
		return 1;
	}

	/* Get sector size */
	if (ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		close(mtd_fd);
		return 1;
	}

	erase_info.length = mtd_info.erasesize;

	for (erase_info.start = 0;
	     erase_info.start < mtd_info.size;
	     erase_info.start += mtd_info.erasesize) {
		if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0) {
			close(mtd_fd);
			return 1;
		}
	}

	close(mtd_fd);
	return 0;
}

