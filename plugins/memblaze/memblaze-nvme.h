#undef CMD_INC_FILE
#define CMD_INC_FILE plugins/memblaze/memblaze-nvme

#if !defined(MEMBLAZE_NVME) || defined(CMD_HEADER_MULTI_READ)
#define MEMBLAZE_NVME

#include "cmd.h"

PLUGIN(NAME("memblaze", "Memblaze vendor specific extensions", NVME_VERSION),
	COMMAND_LIST(
		ENTRY("smart-log-add", "Get Memblaze SMART Log and show it", get_smart_log_add)
        ENTRY("lat-set-feature", "Set Enable/Disable for Latency Monitor feature", set_latency_feature)
        ENTRY("lat-get-feature", "Get Enabled/Disabled of Latency Monitor feature", get_latency_feature)
        ENTRY("lat-stats-print", "Get Latency Statistics log and show it.", get_latency_stats)
		ENTRY("lat-log-print", "Get High Latency log and show it.", get_high_latency_log)
        ENTRY("perf-stats-print", "Get Performance Stat log and show it.", get_performance_stats)
    )
);

#endif

#include "define_cmd.h"
