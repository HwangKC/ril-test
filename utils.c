#include <sys/select.h>

/*
 * return 0x02 when success
 * return 0x00 when timeout
 * return negative number when error happens.
 *
 * */
int my_read_select (int read_fd, long timeout)
{
	int    i;
	int    ret;
	int    max_fd;
	struct timeval waittime;
	fd_set read_fdset;

	max_fd = 0;
	FD_ZERO(&read_fdset);
	FD_SET (read_fd, &read_fdset);
	max_fd = read_fd;
	max_fd += 1;

	if (timeout >= 0)
	{
		waittime.tv_usec = (timeout % 1000) * 1000;
		waittime.tv_sec  = timeout / 1000;
		ret = select (max_fd, &read_fdset, NULL, NULL, &waittime);
	}
	else
	{
		ret = select (max_fd, &read_fdset, NULL, NULL, NULL);
	}

	if (ret > 0)
	{
		ret = 0x02;
	}

	return ret;
}


