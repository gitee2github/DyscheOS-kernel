#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/param.h>
#include <linux/memblock.h>

static void __init parse_dysche_map_one(char *p)
{
	char *oldp;
	unsigned long start_at, mem_size;

	if (!p)
		return;

	oldp = p;
	mem_size = memparse(p, &p);
	if (p == oldp)
		return;

	if (*p == '@') {
		start_at = memparse(p + 1, &p);
		memblock_add(start_at, mem_size);
		memblock_reserve(start_at, mem_size);

	} else {
		pr_warn("Unrecognized syntax: %s\n", p);
	}
}

static int __init parse_dysche_map_opt(char *str)
{
	char *k;
	while (str) {
		k = strchr(str, ',');

		if (k)
			*k++ = 0;

		parse_dysche_map_one(str);
		str = k;
	}

	return 0;
}
early_param("dysche_reserve", parse_dysche_map_opt);

static bool dysche_mode = false;

bool is_dysche_mode(void)
{
	return dysche_mode;
}

static int __init parse_dysche_mode(char *str)
{
	dysche_mode = true;

	return 0;
}
early_param("dysche_mode", parse_dysche_mode);