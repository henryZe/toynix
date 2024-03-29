#include <lib.h>

static void
usage(void)
{
	int i;

	printf("%s: debug_info", __func__);
	for (i = 0; i < ARRAY_SIZE(debug_option); i++)
		printf(" %s |", debug_option[i]);
	printf("\n");

	exit();
}

static void vma_usage(void)
{
	printf("usage: debug_info vma env_id\n");
	exit();
}

static const char * const env_status[] = {
	"free",
	"dying",
	"waiting",
	"running",
	"pending",
};

void
umain(int argc, char **argv)
{
	int i, fd, ret, envid;
	char buf[256] = {0};
	uint32_t *tmp = UTEMP;
	struct vm_area_struct *vma;

	if (argc == 1)
		usage();

	for (i = 0; i < MAXDEBUGOPT; i++) {
		ret = strcmp(argv[1], debug_option[i]);
		if (!ret)
			break;
	}

	switch (i) {
	case CPU_INFO:
	case MEM_INFO:
		fd = opendebug();
		if (fd < 0)
			return;

		write(fd, argv[1], strlen(argv[1]));
		read(fd, buf, sizeof(buf));

		close(fd);
		printf("%s", buf);
		break;

	case FS_INFO:
		ret = sys_page_alloc(0, tmp, PTE_W);

		ipc_send(ipc_find_env(ENV_TYPE_FS), FSREQ_INFO, tmp, PTE_W);
		ret = ipc_recv(NULL, NULL, NULL);
		if (ret < 0)
			return;

		printf("Total blocks: %d\n"
				" Used blocks: %d\n"
				"       Usage: %f%%\n",
				tmp[0], tmp[1], (float)tmp[1] * 100 / tmp[0]);

		sys_page_unmap(0, tmp);
		break;

	case ENV_INFO:
		printf("%8s %16s %8s %s %10s %8s %16s\n",
				"Env", "Name", "Status", "CPU", "Run times",
				"Father", "Father name");

		for (i = 0; i < NENV; i++) {
			if (envs[i].env_status != ENV_FREE) {
				printf("%8x %16s %8s %2d %10d %8x",
					envs[i].env_id, envs[i].binaryname,
					env_status[envs[i].env_status],
					envs[i].env_cpunum, envs[i].env_runs,
					envs[i].env_parent_id);
				if (envs[i].env_parent_id)
					printf(" %16s",
						envs[ENVX(envs[i].env_parent_id)].binaryname);
				printf("\n");
			}
		}
		break;

	case VMA_INFO:
		if (!argv[2])
			vma_usage();

		envid = strtol(argv[2], NULL, 16);

		if (envs[ENVX(envid)].env_status == ENV_FREE)
			return;

		printf("Env %x:\n", envid);
		printf("VMA \t Begin \t\t Size \t\t Perm\n");

		for (i = 0; i < envs[ENVX(envid)].vma_valid; i++) {
			vma = (void *)&(envs[ENVX(envid)].vma[i]);

			printf("%d \t 0x%08lx \t 0x%08x \t %d\n",
					i, vma->vm_start, vma->size, vma->vm_page_prot);
		}
		break;

	default:
		return;
	}
}
