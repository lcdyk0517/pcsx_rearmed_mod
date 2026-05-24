/*
 * (C) 2024 - Embedded cheat system for PCSX ReARMed libretro
 *
 * Parses embedded cheat database, generates core options,
 * and applies selections to the PCSX cheat engine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro_cheats.h"
#include "../libpcsxcore/cheat.h"

cheat_db_t g_cheat_db = {0};

/* External: generated cheat database */
#include "cheat_db.c"

static int parse_cheats_for_game(const uint8_t *data, size_t data_len)
{
  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  g_cheat_db.n_cheats = 0;
  g_cheat_db.n_codes = 0;

  while (p + 2 <= end) {
    uint16_t name_len = p[0] | (p[1] << 8);
    p += 2;

    if (name_len == 0)
      break; /* end of game marker */

    if (p + name_len + 2 > end)
      return -1;

    /* Check limits */
    if (g_cheat_db.n_cheats >= CHEAT_DB_MAX_CHEATS)
      break;

    /* Allocate name copy */
    char *name = (char *)malloc(name_len + 1);
    if (!name) return -1;
    memcpy(name, p, name_len);
    name[name_len] = '\0';
    p += name_len;

    uint16_t code_count = p[0] | (p[1] << 8);
    p += 2;

    if (g_cheat_db.n_codes + code_count > CHEAT_DB_MAX_CODES)
      break;
    if (p + code_count * 6 > end)
      return -1;

    g_cheat_db.names[g_cheat_db.n_cheats] = name;
    g_cheat_db.cheat_first_code[g_cheat_db.n_cheats] = g_cheat_db.n_codes;
    g_cheat_db.cheat_num_codes[g_cheat_db.n_cheats] = code_count;

    for (int j = 0; j < code_count; j++) {
      uint32_t addr = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
      p += 4;
      uint16_t val = p[0] | (p[1] << 8);
      p += 2;
      g_cheat_db.code_addrs[g_cheat_db.n_codes] = addr;
      g_cheat_db.code_values[g_cheat_db.n_codes] = val;
      g_cheat_db.n_codes++;
    }

    g_cheat_db.n_cheats++;
  }

  return 0;
}

int cheat_db_init(const char *cdrom_id)
{
  cheat_db_free();

  if (!cdrom_id || !cdrom_id[0])
    return 0;

  /* Look up game in database */
  const cheat_db_index_t *entry = cheat_db_find(cdrom_id);
  if (!entry)
    return 0; /* no cheats for this game - not an error */

  /* Allocate buffers */
  memset(&g_cheat_db, 0, sizeof(g_cheat_db));
  g_cheat_db.names            = (char **)calloc(CHEAT_DB_MAX_CHEATS, sizeof(char *));
  g_cheat_db.cheat_first_code = (int *)calloc(CHEAT_DB_MAX_CHEATS, sizeof(int));
  g_cheat_db.cheat_num_codes  = (int *)calloc(CHEAT_DB_MAX_CHEATS, sizeof(int));
  g_cheat_db.code_addrs       = (uint32_t *)calloc(CHEAT_DB_MAX_CODES, sizeof(uint32_t));
  g_cheat_db.code_values      = (uint16_t *)calloc(CHEAT_DB_MAX_CODES, sizeof(uint16_t));

  if (!g_cheat_db.names || !g_cheat_db.cheat_first_code ||
      !g_cheat_db.cheat_num_codes || !g_cheat_db.code_addrs ||
      !g_cheat_db.code_values) {
    cheat_db_free();
    return -1;
  }

  /* Calculate data range for this game in the blob */
  const uint8_t *blob_start = cheat_db_blob + entry->blob_offset;
  const uint8_t *blob_end;
  int idx = (int)(entry - cheat_db_index);
  if (idx + 1 < CHEAT_DB_GAME_COUNT)
    blob_end = cheat_db_blob + cheat_db_index[idx + 1].blob_offset;
  else
    blob_end = cheat_db_blob + sizeof(cheat_db_blob);

  return parse_cheats_for_game(blob_start, (size_t)(blob_end - blob_start));
}

void cheat_db_free(void)
{
  if (g_cheat_db.names) {
    for (int i = 0; i < g_cheat_db.n_cheats; i++)
      free(g_cheat_db.names[i]);
    free(g_cheat_db.names);
  }
  free(g_cheat_db.cheat_first_code);
  free(g_cheat_db.cheat_num_codes);
  free(g_cheat_db.code_addrs);
  free(g_cheat_db.code_values);
  free(g_cheat_db.option_defs);
  memset(&g_cheat_db, 0, sizeof(g_cheat_db));
}

static struct retro_core_option_v2_definition *
build_option_defs(void)
{
  size_t n = g_cheat_db.n_cheats;
  if (n == 0) return NULL;

  struct retro_core_option_v2_definition *defs =
    (struct retro_core_option_v2_definition *)
    calloc(n + 1, sizeof(struct retro_core_option_v2_definition));

  if (!defs) return NULL;

  for (size_t i = 0; i < n; i++) {
    struct retro_core_option_v2_definition *opt = &defs[i];

    /* Key: pcsx_rearmed_cheat_N */
    char *key = (char *)malloc(64);
    if (!key) continue;
    snprintf(key, 64, "pcsx_rearmed_cheat_%zu", i);
    opt->key = key;

    /* Use cheat name as description */
    opt->desc = g_cheat_db.names[i];
    opt->desc_categorized = NULL;
    opt->info = NULL;
    opt->info_categorized = NULL;
    opt->category_key = "cheat";

    /* Boolean: disabled / enabled */
    opt->values[0].value = "disabled";
    opt->values[0].label = NULL;
    opt->values[1].value = "enabled";
    opt->values[1].label = NULL;
    opt->values[2].value = NULL;
    opt->values[2].label = NULL;

    opt->default_value = "disabled";
  }

  /* Sentinel */
  defs[n].key = NULL;
  defs[n].desc = NULL;
  defs[n].category_key = NULL;
  defs[n].values[0].value = NULL;

  return defs;
}

struct retro_core_option_v2_definition *
cheat_db_get_option_defs(size_t *count)
{
  if (g_cheat_db.n_cheats == 0) {
    *count = 0;
    return NULL;
  }

  if (!g_cheat_db.option_defs) {
    g_cheat_db.option_defs = build_option_defs();
    g_cheat_db.option_count = g_cheat_db.n_cheats;
  }

  *count = g_cheat_db.option_count;
  return g_cheat_db.option_defs;
}

/* Format a cheat's code lines for PCSX's AddCheat function.
   PCSX expects space-separated addr-value pairs, multi-line codes
   separated by newlines. */
static char *format_codes_for_addcheat(int cheat_idx)
{
  int first = g_cheat_db.cheat_first_code[cheat_idx];
  int num   = g_cheat_db.cheat_num_codes[cheat_idx];
  int bufsz = num * 20 + 1;

  char *buf = (char *)malloc(bufsz);
  if (!buf) return NULL;

  int pos = 0;
  for (int i = 0; i < num; i++) {
    int written = snprintf(buf + pos, bufsz - pos, "%08X %04X",
                           g_cheat_db.code_addrs[first + i],
                           g_cheat_db.code_values[first + i]);
    if (written < 0) { free(buf); return NULL; }
    pos += written;
    if (i < num - 1) {
      if (pos + 1 < bufsz) { buf[pos] = '\n'; pos++; }
    }
  }
  buf[pos] = '\0';
  return buf;
}

/**
 * Add all cheats for the current game into the PCSX cheat engine,
 * all initially disabled. Called once per game load.
 */
void cheat_db_load(void)
{
  if (g_cheat_db.n_cheats == 0)
    return;

  /* Clear any pre-existing cheats (from cheatpops.db etc.) */
  ClearAllCheats();

  /* Pre-add all cheats to PCSX engine, disabled */
  for (int i = 0; i < g_cheat_db.n_cheats; i++) {
    char *code = format_codes_for_addcheat(i);
    if (code) {
      AddCheat(g_cheat_db.names[i], code);
      free(code);
    }
  }
}

/**
 * Read cheat option values from frontend and update
 * Cheats[i].Enabled flags. Called initially and on variable change.
 */
void cheat_db_update(retro_environment_t cb)
{
  if (g_cheat_db.n_cheats == 0 || !cb)
    return;

  for (int i = 0; i < g_cheat_db.n_cheats; i++) {
    struct retro_variable var = {0};
    char key[64];
    snprintf(key, sizeof(key), "pcsx_rearmed_cheat_%d", i);
    var.key = key;

    if (!cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value)
      continue;

    if (i < NumCheats)
      Cheats[i].Enabled = (strcmp(var.value, "enabled") == 0);
  }
}

void cheat_db_update_variable(retro_environment_t cb, bool initial)
{
  if (g_cheat_db.n_cheats == 0 || !cb)
    return;

  if (initial) {
    struct retro_variable var;
    for (int i = 0; i < g_cheat_db.n_cheats; i++) {
      char key[64];
      snprintf(key, sizeof(key), "pcsx_rearmed_cheat_%d", i);
      var.key = key;
      var.value = "disabled";
      cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var);
    }
  }
}
