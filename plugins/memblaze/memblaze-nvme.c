#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "argconfig.h"
#include "common.h"
#include "linux/nvme_ioctl.h"
#include "nvme-ioctl.h"
#include "nvme-print.h"
#include "nvme.h"
#include "plugin.h"
#include "suffix.h"

#define CREATE_CMD
#include "memblaze-nvme.h"

// Global definitions

static inline int K2C(int k) {  // KELVINS_2_CELSIUS
    return (k - 273);
};

// Global ID definitions

enum {
    // feature ids
    FID_LATENCY_FEATURE = 0xd0,

    // log ids
    LID_SMART_LOG_ADD          = 0xca,
    LID_LATENCY_STATISTICS     = 0xd0,
    LID_HIGH_LATENCY_LOG       = 0xd1,
    LID_PERFORMANCE_STATISTICS = 0xd2,
};

// smart-log-add

struct smart_log_add_item {
    uint32_t index;
    char    *attr;
};

#pragma pack(push, 1)
struct wear_level {
    __le16 min;
    __le16 max;
    __le16 avg;
};

struct smart_log_add_item_12 {
    uint8_t id;
    uint8_t rsvd[2];
    uint8_t norm;
    uint8_t rsvd1;
    union {
        struct wear_level wear_level;  // 0xad
        struct temp_since_born {       // 0xe7
            __le16 max;
            __le16 min;
            __le16 curr;
        } temp_since_born;
        struct power_consumption {  // 0xe8
            __le16 max;
            __le16 min;
            __le16 curr;
        } power_consumption;
        struct temp_since_power_on {  // 0xaf
            __le16 max;
            __le16 min;
            __le16 curr;
        } temp_since_power_on;
        uint8_t raw[6];
    };
    uint8_t rsvd2;
};

struct smart_log_add_item_10 {
    uint8_t id;
    uint8_t norm;
    union {
        struct wear_level wear_level;  // 0xad
        uint8_t           raw[6];
    };
    uint8_t rsvd[2];
};
#pragma pack(pop)

struct smart_log_add {
    union {
        union {
            struct smart_log_add_v0 {
                struct smart_log_add_item_12 program_fail_count;
                struct smart_log_add_item_12 erase_fail_count;
                struct smart_log_add_item_12 wear_leveling_count;
                struct smart_log_add_item_12 end_to_end_error_count;
                struct smart_log_add_item_12 crc_error_count;
                struct smart_log_add_item_12 timed_workload_media_wear;
                struct smart_log_add_item_12 timed_workload_host_reads;
                struct smart_log_add_item_12 timed_workload_timer;
                struct smart_log_add_item_12 thermal_throttle_status;
                struct smart_log_add_item_12 retry_buffer_overflow_counter;
                struct smart_log_add_item_12 pll_lock_loss_count;
                struct smart_log_add_item_12 nand_bytes_written;
                struct smart_log_add_item_12 host_bytes_written;
                struct smart_log_add_item_12 system_area_life_remaining;
                struct smart_log_add_item_12 nand_bytes_read;
                struct smart_log_add_item_12 temperature;
                struct smart_log_add_item_12 power_consumption;
                struct smart_log_add_item_12 power_on_temperature;
                struct smart_log_add_item_12 power_loss_protection;
                struct smart_log_add_item_12 read_fail_count;
                struct smart_log_add_item_12 thermal_throttle_time;
                struct smart_log_add_item_12 flash_error_media_count;
            } v0;

            struct smart_log_add_item_12 v0_raw[22];
        };

        union {
            struct smart_log_add_v2 {
                struct smart_log_add_item_12 program_fail_count;
                struct smart_log_add_item_12 erase_fail_count;
                struct smart_log_add_item_12 wear_leveling_count;
                struct smart_log_add_item_12 end_to_end_error_count;
                struct smart_log_add_item_12 crc_error_count;
                struct smart_log_add_item_12 timed_workload_media_wear;
                struct smart_log_add_item_12 timed_workload_host_reads;
                struct smart_log_add_item_12 timed_workload_timer;
                struct smart_log_add_item_12 thermal_throttle_status;
                struct smart_log_add_item_12 lifetime_write_amplification;
                struct smart_log_add_item_12 pll_lock_loss_count;
                struct smart_log_add_item_12 nand_bytes_written;
                struct smart_log_add_item_12 host_bytes_written;
                struct smart_log_add_item_12 system_area_life_remaining;
                struct smart_log_add_item_12 firmware_update_count;
                struct smart_log_add_item_12 dram_cecc_count;
                struct smart_log_add_item_12 dram_uecc_count;
                struct smart_log_add_item_12 xor_pass_count;
                struct smart_log_add_item_12 xor_fail_count;
                struct smart_log_add_item_12 xor_invoked_count;
                struct smart_log_add_item_12 inflight_read_io_cmd;
                struct smart_log_add_item_12 flash_error_media_count;
                struct smart_log_add_item_12 nand_bytes_read;
                struct smart_log_add_item_12 temp_since_born;
                struct smart_log_add_item_12 power_consumption;
                struct smart_log_add_item_12 temp_since_bootup;
                struct smart_log_add_item_12 thermal_throttle_time;
            } v2;

            struct smart_log_add_item_12 v2_raw[27];
        };

        union {
            struct smart_log_add_v3 {
                struct smart_log_add_item_10 program_fail_count;
                struct smart_log_add_item_10 erase_fail_count;
                struct smart_log_add_item_10 wear_leveling_count;
                struct smart_log_add_item_10 ext_e2e_err_count;
                struct smart_log_add_item_10 crc_err_count;
                struct smart_log_add_item_10 nand_bytes_written;
                struct smart_log_add_item_10 host_bytes_written;
                struct smart_log_add_item_10 reallocated_sector_count;
                struct smart_log_add_item_10 uncorrectable_sector_count;
                struct smart_log_add_item_10 nand_uecc_detection;
                struct smart_log_add_item_10 nand_xor_correction;
                struct smart_log_add_item_10 gc_count;
                struct smart_log_add_item_10 dram_uecc_detection_count;
                struct smart_log_add_item_10 sram_uecc_detection_count;
                struct smart_log_add_item_10 internal_raid_recovery_fail_count;
                struct smart_log_add_item_10 inflight_cmds;
                struct smart_log_add_item_10 internal_e2e_err_count;
                struct smart_log_add_item_10 die_fail_count;
                struct smart_log_add_item_10 wear_leveling_execution_count;
                struct smart_log_add_item_10 read_disturb_count;
                struct smart_log_add_item_10 data_retention_count;
                struct smart_log_add_item_10 capacitor_health;
            } v3;

            struct smart_log_add_item_10 v3_raw[24];
        };

        uint8_t raw[512];
    };
};

static void smart_log_add_v0_print(struct smart_log_add_item_12 *item, int item_count) {
    static const struct smart_log_add_item items[0xff] = {
        [0xab] = {0,  "program_fail_count"           },
        [0xac] = {1,  "erase_fail_count"             },
        [0xad] = {2,  "wear_leveling_count"          },
        [0xb8] = {3,  "end_to_end_error_count"       },
        [0xc7] = {4,  "crc_error_count"              },
        [0xe2] = {5,  "timed_workload_media_wear"    },
        [0xe3] = {6,  "timed_workload_host_reads"    },
        [0xe4] = {7,  "timed_workload_timer"         },
        [0xea] = {8,  "thermal_throttle_status"      },
        [0xf0] = {9,  "retry_buffer_overflow_counter"},
        [0xf3] = {10, "pll_lock_loss_count"          },
        [0xf4] = {11, "nand_bytes_written"           },
        [0xf5] = {12, "host_bytes_written"           },
        [0xf6] = {13, "system_area_life_remaining"   },
        [0xfa] = {14, "nand_bytes_read"              },
        [0xe7] = {15, "temperature"                  },
        [0xe8] = {16, "power_consumption"            },
        [0xaf] = {17, "power_on_temperature"         },
        [0xec] = {18, "power_loss_protection"        },
        [0xf2] = {19, "read_fail_count"              },
        [0xeb] = {20, "thermal_throttle_time"        },
        [0xed] = {21, "flash_error_media_count"      },
    };

    for (int i = 0; i < item_count; i++, item++) {
        if (item->id == 0) continue;

        printf("%#-12" PRIx8 "%-36s%-12d", item->id, items[item->id].attr, item->norm);
        switch (item->id) {
            case 0xad:
                printf("min: %d, max: %d, avg: %d\n",
                       le16_to_cpu(item->wear_level.min),
                       le16_to_cpu(item->wear_level.max),
                       le16_to_cpu(item->wear_level.avg));
                break;
            case 0xe7:
                printf("max: %d °C (%d K), min: %d °C (%d K), curr: %d °C (%d K)\n",
                       K2C(le16_to_cpu(item->temp_since_born.max)), le16_to_cpu(item->temp_since_born.max),
                       K2C(le16_to_cpu(item->temp_since_born.min)), le16_to_cpu(item->temp_since_born.min),
                       K2C(le16_to_cpu(item->temp_since_born.curr)), le16_to_cpu(item->temp_since_born.curr));
                break;
            case 0xe8:
                printf("max: %d, min: %d, curr: %d\n",
                       le16_to_cpu(item->power_consumption.max),
                       le16_to_cpu(item->power_consumption.min),
                       le16_to_cpu(item->power_consumption.curr));
                break;
            case 0xaf:
                printf("max: %d °C (%d K), min: %d °C (%d K), curr: %d °C (%d K)\n",
                       K2C(le16_to_cpu(item->temp_since_power_on.max)), le16_to_cpu(item->temp_since_power_on.max),
                       K2C(le16_to_cpu(item->temp_since_power_on.min)), le16_to_cpu(item->temp_since_power_on.min),
                       K2C(le16_to_cpu(item->temp_since_power_on.curr)), le16_to_cpu(item->temp_since_power_on.curr));
                break;
            default:
                printf("%" PRIu64 "\n", int48_to_long(item->raw));
                break;
        }
    }
}

static void smart_log_add_v2_print(struct smart_log_add_item_12 *item, int item_count) {
    static const struct smart_log_add_item items[0xff] = {
        [0xab] = {0,  "program_fail_count"          },
        [0xac] = {1,  "erase_fail_count"            },
        [0xad] = {2,  "wear_leveling_count"         },
        [0xb8] = {3,  "end_to_end_error_count"      },
        [0xc7] = {4,  "crc_error_count"             },
        [0xe2] = {5,  "timed_workload_media_wear"   },
        [0xe3] = {6,  "timed_workload_host_reads"   },
        [0xe4] = {7,  "timed_workload_timer"        },
        [0xea] = {8,  "thermal_throttle_status"     },
        [0xf0] = {9,  "lifetime_write_amplification"},
        [0xf3] = {10, "pll_lock_loss_count"         },
        [0xf4] = {11, "nand_bytes_written"          },
        [0xf5] = {12, "host_bytes_written"          },
        [0xf6] = {13, "system_area_life_remaining"  },
        [0xf9] = {14, "firmware_update_count"       },
        [0xfa] = {15, "dram_cecc_count"             },
        [0xfb] = {16, "dram_uecc_count"             },
        [0xfc] = {17, "xor_pass_count"              },
        [0xfd] = {18, "xor_fail_count"              },
        [0xfe] = {19, "xor_invoked_count"           },
        [0xe5] = {20, "inflight_read_io_cmd"        },
        [0xe6] = {21, "flash_error_media_count"     },
        [0xf8] = {22, "nand_bytes_read"             },
        [0xe7] = {23, "temp_since_born"             },
        [0xe8] = {24, "power_consumption"           },
        [0xaf] = {25, "temp_since_bootup"           },
        [0xeb] = {26, "thermal_throttle_time"       },
    };

    for (int i = 0; i < item_count; i++, item++) {
        if (item->id == 0) continue;

        printf("%#-12" PRIx8 "%-36s%-12d", item->id, items[item->id].attr, item->norm);
        switch (item->id) {
            case 0xad:
                printf("min: %d, max: %d, avg: %d\n",
                       le16_to_cpu(item->wear_level.min),
                       le16_to_cpu(item->wear_level.max),
                       le16_to_cpu(item->wear_level.avg));
                break;
            case 0xe7:
                printf("max: %d °C (%d K), min: %d °C (%d K), curr: %d °C (%d K)\n",
                       K2C(le16_to_cpu(item->temp_since_born.max)), le16_to_cpu(item->temp_since_born.max),
                       K2C(le16_to_cpu(item->temp_since_born.min)), le16_to_cpu(item->temp_since_born.min),
                       K2C(le16_to_cpu(item->temp_since_born.curr)), le16_to_cpu(item->temp_since_born.curr));
                break;
            case 0xe8:
                printf("max: %d, min: %d, curr: %d\n",
                       le16_to_cpu(item->power_consumption.max),
                       le16_to_cpu(item->power_consumption.min),
                       le16_to_cpu(item->power_consumption.curr));
                break;
            case 0xaf:
                printf("max: %d °C (%d K), min: %d °C (%d K), curr: %d °C (%d K)\n",
                       K2C(le16_to_cpu(item->temp_since_power_on.max)), le16_to_cpu(item->temp_since_power_on.max),
                       K2C(le16_to_cpu(item->temp_since_power_on.min)), le16_to_cpu(item->temp_since_power_on.min),
                       K2C(le16_to_cpu(item->temp_since_power_on.curr)), le16_to_cpu(item->temp_since_power_on.curr));
                break;
            default:
                printf("%" PRIu64 "\n", int48_to_long(item->raw));
                break;
        }
    }
}

static void smart_log_add_v3_print(struct smart_log_add_item_10 *item, int item_count) {
    static const struct smart_log_add_item items[0xff] = {
        [0xab] = {0,  "program_fail_count"               },
        [0xac] = {1,  "erase_fail_count"                 },
        [0xad] = {2,  "wear_leveling_count"              },
        [0xb8] = {3,  "ext_e2e_err_count"                },
        [0xc7] = {4,  "crc_err_count"                    },
        [0xf4] = {5,  "nand_bytes_written"               },
        [0xf5] = {6,  "host_bytes_written"               },
        [0xd0] = {7,  "reallocated_sector_count"         },
        [0xd1] = {8,  "uncorrectable_sector_count"       },
        [0xd2] = {9,  "nand_uecc_detection"              },
        [0xd3] = {10, "nand_xor_correction"              },
        [0xd4] = {12, "gc_count"                         }, // 11 is reserved
        [0xd5] = {13, "dram_uecc_detection_count"        },
        [0xd6] = {14, "sram_uecc_detection_count"        },
        [0xd7] = {15, "internal_raid_recovery_fail_count"},
        [0xd8] = {16, "inflight_cmds"                    },
        [0xd9] = {17, "internal_e2e_err_count"           },
        [0xda] = {19, "die_fail_count"                   }, // 18 is reserved
        [0xdb] = {20, "wear_leveling_execution_count"    },
        [0xdc] = {21, "read_disturb_count"               },
        [0xdd] = {22, "data_retention_count"             },
        [0xde] = {23, "capacitor_health"                 },
    };

    for (int i = 0; i < item_count; i++, item++) {
        if (item->id == 0) continue;

        printf("%#-12" PRIx8 "%-36s%-12d", item->id, items[item->id].attr, item->norm);
        switch (item->id) {
            case 0xad:
                printf("min: %d, max: %d, avg: %d\n",
                       le16_to_cpu(item->wear_level.min),
                       le16_to_cpu(item->wear_level.max),
                       le16_to_cpu(item->wear_level.avg));
                break;
            default:
                printf("%" PRIu64 "\n", int48_to_long(item->raw));
                break;
        }
    }
}

static void smart_log_add_print(struct smart_log_add *log, const char *devname) {
    uint8_t version = log->raw[511];

    printf("Version: %u\n", version);
    printf("\n");
    printf("Additional Smart Log for NVMe device: %s\n", devname);
    printf("\n");

    printf("%-12s%-36s%-12s%s\n", "Id", "Key", "Normalized", "Raw");

    switch (version) {
        case 0:
            return smart_log_add_v0_print(&log->v0_raw[0], sizeof(struct smart_log_add_v0) / sizeof(struct smart_log_add_item_12));
        case 2:
            return smart_log_add_v2_print(&log->v2_raw[0], sizeof(struct smart_log_add_v2) / sizeof(struct smart_log_add_item_12));
        case 3:
            return smart_log_add_v3_print(&log->v3_raw[0], sizeof(struct smart_log_add_v3) / sizeof(struct smart_log_add_item_10));

        case 1:
            fprintf(stderr, "Version %d: N/A\n", version);
            break;
        default:
            fprintf(stderr, "Version %d: Not supported yet\n", version);
            break;
    }
}

static int get_smart_log_add(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    struct config {
        bool raw_binary;
    };

    struct config cfg = {0};

    OPT_ARGS(opts) = {
        OPT_FLAG("raw-binary", 'b', &cfg.raw_binary, "dump the whole log buffer in binary format"),
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Get log

    struct smart_log_add log = {0};

    int err = nvme_get_log(fd, NVME_NSID_ALL, LID_SMART_LOG_ADD, false, NVME_NO_LOG_LSP, sizeof(struct smart_log_add), &log);
    if (!err) {
        if (!cfg.raw_binary)
            smart_log_add_print(&log, devicename);
        else
            d_raw((unsigned char *)&log, sizeof(struct smart_log_add));
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}

// performance-monitor

struct latency_stats_bucket {
    char *start_threshold;
    char *end_threshold;
};

#pragma pack(push, 1)
struct latency_stats {
    union {
        struct latency_stats_v2_0 {
            uint32_t minor_version;
            uint32_t major_version;
            uint32_t bucket_read_data[32];
            uint32_t rsvd[32];
            uint32_t bucket_write_data[32];
            uint32_t rsvd1[32];
            uint32_t bucket_trim_data[32];
            uint32_t rsvd2[32];
            uint8_t  rsvd3[248];
        } v2_0;
        uint8_t raw[1024];
    };
};

struct high_latency_log {
    union {
        struct high_latency_log_v1 {
            uint32_t version;
            struct high_latency_log_entry {
                uint64_t timestamp;  // ms
                uint32_t latency;
                uint32_t qid;
                uint32_t opcode : 8;
                uint32_t fuse   : 2;
                uint32_t psdt   : 2;
                uint32_t cid    : 16;
                uint32_t rsvd   : 4;
                uint32_t nsid;
                uint64_t slba;
                uint32_t nlb   : 16;
                uint32_t dtype : 8;
                uint32_t pinfo : 4;
                uint32_t fua   : 1;
                uint32_t lr    : 1;
                uint32_t rsvd1 : 2;
                uint8_t  rsvd2[28];
            } entries[1024];
        } v1;
        uint8_t raw[4 + 1024 * 64];
    };
};

struct performance_stats {
    union {
        struct performance_stats_v1 {
            uint8_t version;
            uint8_t rsvd[3];
            struct performance_stats_timestamp {
                uint8_t timestamp[6];
                struct performance_stats_entry {
                    uint16_t read_iops;          // K IOPS
                    uint16_t read_bandwidth;     // MiB
                    uint32_t read_latency;       // us
                    uint32_t read_latency_max;   // us
                    uint16_t write_iops;         // K IOPS
                    uint16_t write_bandwidth;    // MiB
                    uint32_t write_latency;      // us
                    uint32_t write_latency_max;  // us
                } entries[3600];
            } timestamps[24];
        } v1;
        uint8_t raw[4 + 24 * (6 + 3600 * 24)];
    };
};
#pragma pack(pop)

static int set_latency_feature(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    struct config {
        uint32_t perf_monitor;
        uint32_t cmd_mask;
        uint32_t read_threshold;
        uint32_t write_threshold;
        uint32_t de_allocate_trim_threshold;
    };

    struct config cfg = {0};

    OPT_ARGS(opts) = {
        OPT_UINT("sel-perf-log", 's', &cfg.perf_monitor,
                 "Select features to turn on, default: Disable\n"
                 "    bit 0: latency statistics\n"
                 "    bit 1: high latency log\n"
                 "    bit 2: Performance stat"),
        OPT_UINT("set-commands-mask", 'm', &cfg.cmd_mask,
                 "Set Enable, default: Disable\n"
                 "    bit 0: Read commands\n"
                 "    bit 1: high Write commands\n"
                 "    bit 2: De-allocate/TRIM (this bit is not worked for Performance stat.)"),
        OPT_UINT("set-read-threshold", 'r', &cfg.read_threshold, "set read high latency log threshold, it's a 0-based value and unit is 10ms"),
        OPT_UINT("set-write-threshold", 'w', &cfg.write_threshold, "set write high latency log threshold, it's a 0-based value and unit is 10ms"),
        OPT_UINT("set-trim-threshold", 't', &cfg.de_allocate_trim_threshold, "set trim high latency log threshold, it's a 0-based value and unit is 10ms"),
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Set feature
    uint8_t  fid        = FID_LATENCY_FEATURE;
    uint32_t save       = 0;
    uint32_t uuid_index = 0;

    struct nvme_admin_cmd nvme_cmd = {0};

    nvme_cmd.opcode = nvme_admin_set_features;
    nvme_cmd.nsid   = 0;
    nvme_cmd.cdw11  = 0 | cfg.perf_monitor;
    nvme_cmd.cdw12  = 0 | cfg.cmd_mask;
    nvme_cmd.cdw13  = 0 | (cfg.read_threshold & 0xff) | ((cfg.write_threshold & 0xff) << 8) | ((cfg.de_allocate_trim_threshold & 0xff) << 16);
    nvme_cmd.cdw10  = fid | (save ? 1 << 31 : 0);
    nvme_cmd.cdw14  = uuid_index;

    int err = nvme_submit_admin_passthru(fd, &nvme_cmd);
    if (!err) {
        printf("%s have done successfully. result = %#" PRIx32 ".\n", cmd->name, nvme_cmd.result);
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}

static int get_latency_feature(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    OPT_ARGS(opts) = {
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Get feature

    uint32_t result = 0;

    int err = nvme_get_feature(fd, 0, FID_LATENCY_FEATURE, 0, 0, 0, 0, NULL, &result);
    if (!err) {
        printf("%s have done successfully. result = %#" PRIx32 ".\n", cmd->name, result);

        printf("latency statistics enable status = %d\n", (result & (0x01 << 0) >> 0));
        printf("high latency enable status = %d\n", (result & (0x01 << 1)) >> 1);
        printf("performance stat enable status = %d\n", (result & (0x01 << 2)) >> 2);

        printf("Monitor Read command = %d\n", (result & (0x01 << 4) >> 4));
        printf("Monitor Write command = %d\n", (result & (0x01 << 5)) >> 5);
        printf("Monitor Trim command = %d\n", (result & (0x01 << 6)) >> 6);

        printf("Threshold for Read = %dms\n", (((result & (0xff << 8)) >> 8) + 1) * 10);
        printf("Threshold for Write = %dms\n", (((result & (0xff << 16)) >> 16) + 1) * 10);
        printf("Threshold for Trim = %dms\n", (((result & (0xff << 24)) >> 24) + 1) * 10);
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}

static void latency_stats_v2_0_print(struct latency_stats *log, int size) {
    static const struct latency_stats_bucket buckets[0xff] = {
        [1]  = {"0us",   "50us" },
        [2]  = {"50us",  "100us"},
        [3]  = {"100us", "150us"},
        [4]  = {"150us", "200us"},
        [5]  = {"200us", "300us"},
        [6]  = {"300us", "400us"},
        [7]  = {"400us", "500us"},
        [8]  = {"500us", "600us"},
        [9]  = {"600us", "700us"},
        [10] = {"700us", "800us"},
        [11] = {"800us", "900us"},
        [12] = {"900us", "1ms"  },
        [13] = {"1ms",   "5ms"  },
        [14] = {"5ms",   "10ms" },
        [15] = {"10ms",  "20ms" },
        [16] = {"20ms",  "50ms" },
        [17] = {"50ms",  "100ms"},
        [18] = {"100ms", "200ms"},
        [19] = {"200ms", "300ms"},
        [20] = {"300ms", "400ms"},
        [21] = {"400ms", "500ms"},
        [22] = {"500ms", "600ms"},
        [23] = {"600ms", "700ms"},
        [24] = {"700ms", "800ms"},
        [25] = {"800ms", "900ms"},
        [26] = {"900ms", "1s"   },
        [27] = {"1s",    "2s"   },
        [28] = {"2s",    "3s"   },
        [29] = {"3s",    "4s"   },
        [30] = {"4s",    "5s"   },
        [31] = {"5s",    "8s"   },
        [32] = {"8s",    "INF"  },
    };

    printf("Bucket 1-32 IO Read Command Data\n");
    printf("-------------------------------------------\n");
    printf("%-12s%-12s%-12s%-12s\n", "Bucket", "Start(>=)", "End(<)", "Value");
    int bucket_count = sizeof(log->v2_0.bucket_read_data) / sizeof(uint32_t);
    for (int i = 0; i < bucket_count; i++) {
        printf("%-12u%-12s%-12s%-12u\n", i + 1, buckets[i + 1].start_threshold, buckets[i + 1].end_threshold, log->v2_0.bucket_read_data[i]);
    }
    printf("\n");

    printf("Bucket 1-32 IO Write Command Data\n");
    printf("-------------------------------------------\n");
    printf("%-12s%-12s%-12s%-12s\n", "Bucket", "Start(>=)", "End(<)", "Value");
    bucket_count = sizeof(log->v2_0.bucket_write_data) / sizeof(uint32_t);
    for (int i = 0; i < bucket_count; i++) {
        printf("%-12u%-12s%-12s%-12u\n", i + 1, buckets[i + 1].start_threshold, buckets[i + 1].end_threshold, log->v2_0.bucket_write_data[i]);
    }
    printf("\n");

    printf("Bucket 1-32 IO Trim Command Data\n");
    printf("-------------------------------------------\n");
    printf("%-12s%-12s%-12s%-12s\n", "Bucket", "Start(>=)", "End(<)", "Value");
    bucket_count = sizeof(log->v2_0.bucket_trim_data) / sizeof(uint32_t);
    for (int i = 0; i < bucket_count; i++) {
        printf("%-12u%-12s%-12s%-12u\n", i + 1, buckets[i + 1].start_threshold, buckets[i + 1].end_threshold, log->v2_0.bucket_trim_data[i]);
    }
    printf("\n");
}

static void latency_stats_print(struct latency_stats *log, const char *devname) {
    uint32_t minor_version = *(uint32_t *)&log->raw[0];
    uint32_t major_version = *(uint32_t *)&log->raw[4];

    printf("Major Version: %u, Minor Version: %u\n", major_version, minor_version);
    printf("\n");
    printf("Latency Statistics Log for NVMe device: %s\n", devname);
    printf("\n");

    switch (major_version) {
        case 2:
            switch (minor_version) {
                case 0:
                    latency_stats_v2_0_print(log, sizeof(struct latency_stats));
                    break;

                default:
                    fprintf(stderr, "Major Version %u, Minor Version %u: Not supported yet\n", major_version, minor_version);
                    break;
            }
            break;

        default:
            fprintf(stderr, "Major Version %u: Not supported yet\n", major_version);
            break;
    }
}

static int get_latency_stats(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    struct config {
        bool raw_binary;
    };

    struct config cfg = {0};

    OPT_ARGS(opts) = {
        OPT_FLAG("raw-binary", 'b', &cfg.raw_binary, "dump the whole log buffer in binary format"),
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Get log

    struct latency_stats log = {0};

    int err = nvme_get_log(fd, NVME_NSID_ALL, LID_LATENCY_STATISTICS, false, NVME_NO_LOG_LSP, sizeof(struct latency_stats), &log);
    if (!err) {
        if (!cfg.raw_binary)
            latency_stats_print(&log, devicename);
        else
            d_raw((unsigned char *)&log, sizeof(struct latency_stats));
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}

static void high_latency_log_v1_print(struct high_latency_log *log, int size) {
    printf("%-24s%-12s%-12s%-6s%-6s%-6s%-6s%-12s%-24s%-6s%-6s%-6s%-6s%-6s\n",
           "Timestamp", "Latency(us)", "QID", "OpC", "Fuse", "PSDT", "CID", "NSID", "SLBA", "NLB", "DType", "PInfo", "FUA", "LR");

    for (int i = 0; i < 1024; i++) {
        if (log->v1.entries[i].timestamp == 0) break;

        // Get the timestamp

        time_t timestamp_ms    = log->v1.entries[i].timestamp;
        time_t timestamp_s     = timestamp_ms / 1000;
        int    time_ms         = timestamp_ms % 1000;
        char   str_time_s[20]  = {0};
        char   str_time_ms[24] = {0};

        strftime(str_time_s, sizeof(str_time_s), "%Y-%m-%d %H:%M:%S", localtime(&timestamp_s));
        sprintf(str_time_ms, "%s.%03d", str_time_s, time_ms);
        printf("%-24s", str_time_ms);

        //
        printf("%-12" PRIu32, log->v1.entries[i].latency);
        printf("%-12" PRIu32, log->v1.entries[i].qid);
        printf("%#-6" PRIx32, log->v1.entries[i].opcode);
        printf("%-6" PRIu32, log->v1.entries[i].fuse);
        printf("%-6" PRIu32, log->v1.entries[i].psdt);
        printf("%-6" PRIu32, log->v1.entries[i].cid);
        printf("%-12" PRIu32, log->v1.entries[i].nsid);
        printf("%-24" PRIu64, log->v1.entries[i].slba);
        printf("%-6" PRIu32, log->v1.entries[i].nlb);
        printf("%-6" PRIu32, log->v1.entries[i].dtype);
        printf("%-6" PRIu32, log->v1.entries[i].pinfo);
        printf("%-6" PRIu32, log->v1.entries[i].fua);
        printf("%-6" PRIu32, log->v1.entries[i].lr);
        printf("\n");
    }
}

static void high_latency_log_print(struct high_latency_log *log, const char *devname) {
    uint32_t version = *(uint32_t *)&log->raw[0];

    printf("Version: %u\n", version);
    printf("\n");
    printf("High Latency Log for NVMe device: %s\n", devname);
    printf("\n");

    switch (version) {
        case 1:
            high_latency_log_v1_print(log, sizeof(struct high_latency_log));
            break;

        default:
            fprintf(stderr, "Version %u: Not supported yet\n", version);
            break;
    }
}

static int get_high_latency_log(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    struct config {
        bool raw_binary;
    };

    struct config cfg = {0};

    OPT_ARGS(opts) = {
        OPT_FLAG("raw-binary", 'b', &cfg.raw_binary, "dump the whole log buffer in binary format"),
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Get log

    struct high_latency_log log = {0};

    int err = nvme_get_log(fd, NVME_NSID_ALL, LID_HIGH_LATENCY_LOG, false, NVME_NO_LOG_LSP, sizeof(struct high_latency_log), &log);
    if (!err) {
        if (!cfg.raw_binary)
            high_latency_log_print(&log, devicename);
        else
            d_raw((unsigned char *)&log, sizeof(struct high_latency_log));
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}

static void performance_stats_v1_print(struct performance_stats *log, int duration) {
    for (int i = 0; i < duration; i++) {
        // Print timestamp

        time_t timestamp_ms = int48_to_long(log->v1.timestamps[i].timestamp);
        time_t timestamp_s  = timestamp_ms / 1000;
        int    time_ms      = timestamp_ms % 1000;
        char   time_s[32]   = {0};

        strftime(time_s, sizeof(time_s), "%Y-%m-%d %H:%M:%S", localtime(&timestamp_s));
        printf("Timestamp %2d: %s.%03d\n", i + 1, time_s, time_ms);

        // Print entry title

        printf("%-8s%-14s%-21s%-22s%-22s%-15s%-22s%-23s%-23s\n",
               "Entry",
               "Read-IOs(K)", "Read-Bandwidth(MiB)", "Avg-Read-Latency(us)", "Max-Read-Latency(us)",
               "Write-IOs(K)", "Write-Bandwidth(MiB)", "Avg-Write-Latency(us)", "Max-Write-Latency(us)");

        // Print all entries content

        struct performance_stats_entry *entry = NULL;
        for (int j = 0; j < 3600; j++) {
            entry = &(log->v1.timestamps[i].entries[j]);
            if (entry->read_iops == 0 && entry->write_iops == 0) continue;

            printf("%-8u%-14u%-21u%-22u%-22u%-15u%-22u%-23u%-23u\n",
                   j + 1,
                   entry->read_iops, entry->read_bandwidth, entry->read_latency == 0 ? 0 : entry->read_latency / entry->read_iops, entry->read_latency_max,
                   entry->write_iops, entry->write_bandwidth, entry->write_latency == 0 ? 0 : entry->write_latency / entry->write_iops, entry->write_latency_max);
            usleep(100);
        }
        printf("\n");
    }
}

static void performance_stats_print(struct performance_stats *log, const char *devname, int duration) {
    uint8_t version = *(uint8_t *)&log->raw[0];

    printf("Version: %u\n", version);
    printf("\n");
    printf("Performance Stat log for NVMe device: %s\n", devname);
    printf("\n");

    switch (version) {
        case 1:
            performance_stats_v1_print(log, duration);
            break;

        default:
            fprintf(stderr, "Version %u: Not supported yet\n", version);
            break;
    }
}

static int get_performance_stats(int argc, char **argv, struct command *cmd, struct plugin *plugin) {
    // Get the configuration

    struct config {
        int  duration;
        bool raw_binary;
    };

    struct config cfg = {.duration = 1, .raw_binary = false};

    OPT_ARGS(opts) = {
        OPT_UINT("duration", 'd', &cfg.duration, "[1-24] hours: duration of the log to be printed, default is 1 hour"),
        OPT_FLAG("raw-binary", 'b', &cfg.raw_binary, "dump the whole log buffer in binary format"),
        OPT_END()};

    // Open device

    int fd = parse_and_open(argc, argv, cmd->help, opts);
    if (fd < 0) {
        return fd;
    }

    // Check parameters
    if (cfg.duration < 1 || cfg.duration > 24) {
        fprintf(stderr, "duration must be between 1 and 24.\n");
        exit(1);
    }

    // Get log

    struct performance_stats log = {0};

    int log_size = 4 + cfg.duration * sizeof(struct performance_stats_timestamp);
    // Get one more timestamp if duration is odd number to avoid non-dw alignment issues
    int xfer_size = (cfg.duration % 2) > 0 ? (4 + (cfg.duration + 1) * sizeof(struct performance_stats_timestamp)) : log_size;

    int err = nvme_get_log(fd, NVME_NSID_ALL, LID_PERFORMANCE_STATISTICS, false, NVME_NO_LOG_LSP, xfer_size, &log);
    if (!err) {
        if (!cfg.raw_binary)
            performance_stats_print(&log, devicename, cfg.duration);
        else
            d_raw((unsigned char *)&log, log_size);
    } else {
        nvme_show_status(err);
    }

    // Close device

    close(fd);
    return err;
}
