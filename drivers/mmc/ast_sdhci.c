#include <common.h>
#include <malloc.h>
#include <sdhci.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <mmc.h>

int ast_sdhi_init(u32 regbase, u32 max_clk, u32 min_clk)
{
	struct sdhci_host *host = NULL;
	host = (struct sdhci_host *)calloc(1, sizeof(struct sdhci_host));
	if (!host) {
		printf("sdhci_host malloc fail!\n");
		return 1;
	}

	host->name = "ast_sdhci";
	host->ioaddr = (void *)regbase;

	add_sdhci(host, max_clk, min_clk);
	
	return 0;
}
