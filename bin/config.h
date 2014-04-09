#ifndef CONFIG_H
#define CONFIG_H
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
//#include <cstdint>
#include "log.h"

//macro from common.h
#define SUCCEED 1
#define FAIL 0
#define MAX_STRING_LEN 2048

#define	TYPE_INT		0
#define	TYPE_STRING		1

#define	PARM_OPT	0
#define	PARM_MAND	1


#define SLAVE_KIBIBYTE		1024
#define SLAVE_MEBIBYTE		1048576
#define SLAVE_GIBIBYTE		1073741824

#define SLAVE_MAX_UINT64_LEN 21
#define SLAVE_CFG_LTRIM_CHARS	"\t "
#define SLAVE_CFG_RTRIM_CHARS	SLAVE_CFG_LTRIM_CHARS "\r\n"

struct cfg_line
{
	const char *parameter;
	void	*variable;
	int	type;
	int	mandatory;
	int	min;
	int	max;
};

#define is_uint64(src, value)	is_uint64_n(src, SLAVE_MAX_UINT64_LEN, value)
int	is_uint64_n(const char *str, size_t n, uint64_t *value);
int	parse_cfg_file(const char *cfg_file, struct cfg_line *cfg);


#endif
