#include <btd700/btd700_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const struct {
  btd700_codec_t c;
  const char *name;
} CODECS[] = {
    {BTD700_CODEC_SBC, "sbc"},
    {BTD700_CODEC_APTX, "aptx"},
    {BTD700_CODEC_APTX_ADAPTIVE, "aptx-adaptive"},
    {BTD700_CODEC_APTX_LOSSLESS, "aptx-lossless"},
    {BTD700_CODEC_APTX_LITE, "aptx-lite"},
    {BTD700_CODEC_LC3, "lc3"},
};
static const size_t N_CODECS = sizeof(CODECS) / sizeof(CODECS[0]);

static int parse_codec(const char *s, btd700_codec_t *out) {
  for (size_t i = 0; i < N_CODECS; i++) {
    if (strcasecmp(s, CODECS[i].name) == 0) {
      *out = CODECS[i].c;
      return 0;
    }
  }
  return -1;
}

static void die(btd700_error_t err, const char *ctx) {
  if (err != BTD700_OK) {
    fprintf(stderr, "%s: %s\n", ctx, btd700_error_string(err));
    exit(1);
  }
}

static btd700_driver_t *open_driver(void) {
  btd700_driver_t *drv = NULL;
  die(btd700_driver_create(&drv), "driver_create");
  die(btd700_driver_connect(drv), "driver_connect");
  return drv;
}

static void cmd_list(btd700_driver_t *drv) {
  uint16_t supported = 0, active = 0;
  die(btd700_driver_supported_codecs(drv, &supported), "supported_codecs");
  die(btd700_driver_active_codec(drv, &active), "active_codec");

  printf("%-15s %-10s %s\n", "CODEC", "SUPPORTED", "ACTIVE");
  for (size_t i = 0; i < N_CODECS; i++) {
    uint16_t bit = (uint16_t)(1u << CODECS[i].c);
    printf("%-15s %-10s %s\n", CODECS[i].name, (supported & bit) ? "yes" : "no",
           (active & bit) ? "yes" : "no");
  }
}

static void cmd_current(btd700_driver_t *drv) {
  uint16_t active = 0;
  die(btd700_driver_active_codec(drv, &active), "active_codec");
  if (active == 0) {
    puts("none");
    return;
  }
  for (size_t i = 0; i < N_CODECS; i++) {
    if (active & (uint16_t)(1u << CODECS[i].c)) {
      puts(CODECS[i].name);
      return;
    }
  }
  printf("unknown (0x%04x)\n", active);
}

static void cmd_set(btd700_driver_t *drv, const char *name) {
  btd700_codec_t codec;
  if (parse_codec(name, &codec) != 0) {
    fprintf(stderr, "unknown codec '%s'\n", name);
    exit(2);
  }
  die(btd700_driver_set_codec(drv, codec), "set_codec");
  printf("set codec to %s\n", name);
}

static void cmd_enable(btd700_driver_t *drv, int argc, char **argv) {
  uint16_t mask = 0;
  for (int i = 0; i < argc; i++) {
    btd700_codec_t c;
    if (parse_codec(argv[i], &c) != 0) {
      fprintf(stderr, "unknown codec '%s'\n", argv[i]);
      exit(2);
    }
    mask |= (uint16_t)(1u << c);
  }
  die(btd700_driver_set_codec_mask(drv, mask), "set_codec_mask");
  printf("enabled codecs: 0x%04x\n", mask);
}

static void usage(void) {
  fprintf(stderr,
          "usage: btd700ctl <command> [args]\n"
          "  list                    show supported and active codecs\n"
          "  current                 show currently active codec\n"
          "  set <codec>             force a specific codec\n"
          "  enable <codec> [...]    restrict negotiation to listed codecs\n"
          "\n"
          "codecs: sbc, aptx, aptx-adaptive, aptx-lossless, aptx-lite, lc3\n");
  exit(2);
}

int main(int argc, char **argv) {
  if (argc < 2)
    usage();
  btd700_driver_t *drv = open_driver();
  if (strcmp(argv[1], "list") == 0)
    cmd_list(drv);
  else if (strcmp(argv[1], "current") == 0)
    cmd_current(drv);
  else if (strcmp(argv[1], "set") == 0) {
    if (argc < 3)
      usage();
    cmd_set(drv, argv[2]);
  } else if (strcmp(argv[1], "enable") == 0) {
    if (argc < 3)
      usage();
    cmd_enable(drv, argc - 2, argv + 2);
  } else
    usage();
  btd700_driver_disconnect(drv);
  btd700_driver_destroy(drv);
  return 0;
}
