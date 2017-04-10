/*
 * Copyright 2017 Leonid Yuriev <leo@yuriev.ru>
 * and other libmdbx authors: please see AUTHORS file.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "test.h"

#if defined(_MSC_VER) && !defined(strcasecmp)
#define strcasecmp(str, len) _stricmp(str, len)
#endif /* _MSC_VER && strcasecmp() */

namespace config {

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  const char **value, const char *default_value) {
  assert(narg < argc);
  const char *current = argv[narg];
  const size_t optlen = strlen(option);

  if (strncmp(current, "--", 2) || strncmp(current + 2, option, optlen))
    return false;

  if (!value) {
    if (current[optlen + 2] == '=')
      failure("Option '--%s' doen't accept any value\n", option);
    narg += 1;
    return true;
  }

  *value = nullptr;
  if (current[optlen + 2] == '=') {
    *value = &current[optlen + 3];
    narg += 1;
    return true;
  }

  if (narg + 1 < argc && strncmp("--", argv[narg + 1], 2)) {
    *value = argv[narg + 1];
    narg += 2;
    return true;
  }

  if (default_value) {
    *value = default_value;
    narg += 1;
    return true;
  }

  failure("No value given for '--%s' option\n", option);
}

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  std::string &value, bool allow_empty) {
  const char *value_cstr;
  if (!parse_option(argc, argv, narg, option, &value_cstr,
                    allow_empty ? "" : nullptr))
    return false;

  if (!allow_empty && strlen(value_cstr) == 0)
    failure("Value for option '--%s' could't be empty\n", option);

  value = value_cstr;
  return true;
}

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  size_t &mask, const option_verb *verbs) {
  const char *list;
  if (!parse_option(argc, argv, narg, option, &list))
    return false;

  mask = 0;
  while (*list) {
    if (*list == ',' || *list == ' ' || *list == '\t') {
      ++list;
      continue;
    }

    const char *const comma = strchr(list, ',');
    const size_t len = (comma) ? comma - list : strlen(list);
    const option_verb *scan = verbs;
    while (true) {
      if (!scan->verb)
        failure("Unknown verb '%.*s', for option '==%s'\n", (int)len, list,
                option);
      if (strlen(scan->verb) == len && strncmp(list, scan->verb, len) == 0) {
        mask |= scan->mask;
        list += len;
        break;
      }
      ++scan;
    }
  }

  return true;
}

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  uint64_t &value, const scale_mode scale,
                  const uint64_t minval, const uint64_t maxval) {

  const char *value_cstr;
  if (!parse_option(argc, argv, narg, option, &value_cstr))
    return false;

  char *suffix = nullptr;
  errno = 0;
  unsigned long raw = strtoul(value_cstr, &suffix, 0);
  if (errno)
    failure("Option '--%s' expects a numeric value (%s)\n", option,
            test_strerror(errno));

  uint64_t multipler = 1;
  if (suffix && *suffix) {
    if (scale == no_scale)
      failure("Option '--%s' doen't accepts suffixes, so '%s' is unexpected\n",
              option, suffix);
    if (strcmp(suffix, "K") == 0 || strcasecmp(suffix, "Kilo") == 0)
      multipler = (scale == decimal) ? UINT64_C(1000) : UINT64_C(1024);
    else if (strcmp(suffix, "M") == 0 || strcasecmp(suffix, "Mega") == 0)
      multipler =
          (scale == decimal) ? UINT64_C(1000) * 1000 : UINT64_C(1024) * 1024;
    else if (strcmp(suffix, "G") == 0 || strcasecmp(suffix, "Giga") == 0)
      multipler = (scale == decimal) ? UINT64_C(1000) * 1000 * 1000
                                     : UINT64_C(1024) * 1024 * 1024;
    else if (strcmp(suffix, "T") == 0 || strcasecmp(suffix, "Tera") == 0)
      multipler = (scale == decimal) ? UINT64_C(1000) * 1000 * 1000 * 1000
                                     : UINT64_C(1024) * 1024 * 1024 * 1024;
    else if (scale == duration &&
             (strcmp(suffix, "s") == 0 || strcasecmp(suffix, "Seconds") == 0))
      multipler = 1;
    else if (scale == duration &&
             (strcmp(suffix, "m") == 0 || strcasecmp(suffix, "Minutes") == 0))
      multipler = 60;
    else if (scale == duration &&
             (strcmp(suffix, "h") == 0 || strcasecmp(suffix, "Hours") == 0))
      multipler = 3600;
    else if (scale == duration &&
             (strcmp(suffix, "d") == 0 || strcasecmp(suffix, "Days") == 0))
      multipler = 3600 * 24;
    else
      failure(
          "Option '--%s' expects a numeric value with Kilo/Mega/Giga/Tera %s"
          "suffixes, but '%s' is unexpected\n",
          option, (scale == duration) ? "or Seconds/Minutes/Hours/Days " : "",
          suffix);
  }

  if (raw >= UINT64_MAX / multipler)
    failure("The value for option '--%s' is too huge\n", option);

  value = raw * multipler;
  if (maxval && value > maxval)
    failure("The maximal value for option '--%s' is %" PRIu64 "\n", option,
            maxval);
  if (value < minval)
    failure("The minimal value for option '--%s' is %" PRIu64 "\n", option,
            minval);
  return true;
}

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  unsigned &value, const scale_mode scale,
                  const unsigned minval, const unsigned maxval) {

  uint64_t huge;
  if (!parse_option(argc, argv, narg, option, huge, scale, minval, maxval))
    return false;
  value = (unsigned)huge;
  return true;
}

bool parse_option(int argc, char *const argv[], int &narg, const char *option,
                  bool &value) {
  const char *value_cstr = NULL;
  if (!parse_option(argc, argv, narg, option, &value_cstr, "yes")) {
    const char *current = argv[narg];
    if (strncmp(current, "--no-", 5) || strcmp(current + 5, option))
      return false;
    value = false;
    narg += 1;
    return true;
  }

  if (!value_cstr) {
    value = true;
    return true;
  }

  if (strcasecmp(value_cstr, "yes") == 0 || strcasecmp(value_cstr, "1") == 0) {
    value = true;
    return true;
  }

  if (strcasecmp(value_cstr, "no") == 0 || strcasecmp(value_cstr, "0") == 0) {
    value = false;
    return true;
  }

  failure(
      "Option '--%s' expects a 'boolean' value Yes/No, so '%s' is unexpected\n",
      option, value_cstr);
}

//-----------------------------------------------------------------------------

const struct option_verb mode_bits[] = {
    {"rdonly", MDB_RDONLY},           {"mapasync", MDB_MAPASYNC},
    {"utterly", MDBX_UTTERLY_NOSYNC}, {"nosubdir", MDB_NOSUBDIR},
    {"nosync", MDB_NOSYNC},           {"nometasync", MDB_NOMETASYNC},
    {"writemap", MDB_WRITEMAP},       {"notls", MDB_NOTLS},
    {"nordahead", MDB_NORDAHEAD},     {"nomeminit", MDB_NOMEMINIT},
    {"coasesce", MDBX_COALESCE},      {"lifo", MDBX_LIFORECLAIM},
    {"parturb", MDBX_PAGEPERTURB},    {nullptr, 0}};

const struct option_verb table_bits[] = {
    {"key.reverse", MDB_REVERSEKEY},
    {"key.integer", MDB_INTEGERKEY},
    {"data.integer", MDB_INTEGERDUP | MDB_DUPFIXED | MDB_DUPSORT},
    {"data.fixed", MDB_DUPFIXED | MDB_DUPSORT},
    {"data.reverse", MDB_REVERSEDUP | MDB_DUPSORT},
    {"data.dups", MDB_DUPSORT},
    {nullptr, 0}};

static void dump_verbs(FILE *out, const char *caption, size_t bits,
                       const struct option_verb *verbs) {
  fprintf(out, "%s: (%" PRIu64 ")", caption, (uint64_t)bits);

  while (verbs->mask && bits) {
    if ((bits & verbs->mask) == verbs->mask) {
      fprintf(out, ", %s", verbs->verb);
      bits -= verbs->mask;
    }
    ++verbs;
  }

  fprintf(out, "\n");
}

static void dump_duration(FILE *out, const char *caption, unsigned duration) {
  fprintf(out, "%s: ", caption);
  if (duration) {
    if (duration > 24 * 3600)
      fprintf(out, "%u_", duration / (24 * 3600));
    if (duration > 3600)
      fprintf(out, "%02u:", (duration % (24 * 3600)) / 3600);
    fprintf(out, "%02u:%02u", (duration % 3600) / 60, duration % 60);
  } else
    fprintf(out, "INFINITE");
  fprintf(out, "\n");
}

void dump(FILE *out) {
  for (auto i = global::actors.begin(); i != global::actors.end(); ++i) {
    fprintf(out, "testcase %s\n", testcase2str(i->testcase));
    if (i->id)
      fprintf(out, "\tid/table %u\n", i->id);

    if (i->params.loglevel) {
      fprintf(out, "\tlog: level %u, %s\n", i->params.loglevel,
              i->params.pathname_log.empty() ? "console"
                                             : i->params.pathname_log.c_str());
    }

    fprintf(out, "\tdatabase: %s, size %" PRIu64 "\n",
            i->params.pathname_db.c_str(), i->params.size);

    dump_verbs(out, "\tmode", i->params.mode_flags, mode_bits);
    dump_verbs(out, "\ttable", i->params.table_flags, table_bits);

    fprintf(out, "\tseed %u\n", i->params.seed);

    if (i->params.test_nrecords)
      fprintf(out, "\trecords %u\n", i->params.test_nrecords);
    else
      dump_duration(out, "\tduration", i->params.test_duration);

    if (i->params.nrepeat)
      fprintf(out, "\trepeat %u\n", i->params.nrepeat);
    else
      fprintf(out, "\trepeat ETERNALLY\n");

    fprintf(out, "\tthreads %u\n", i->params.nthreads);

    fprintf(out, "\tkey: minlen %u, maxlen %u\n", i->params.keylen_min,
            i->params.keylen_max);
    fprintf(out, "\tdata: minlen %u, maxlen %u\n", i->params.datalen_min,
            i->params.datalen_max);

    fprintf(out, "\tbatch: read %u, write %u\n", i->params.batch_read,
            i->params.batch_write);

    if (i->params.waitfor_nops)
      fprintf(out, "\twait: actor %u for %u ops\n", i->wait4id,
              i->params.waitfor_nops);
    else if (i->params.delaystart)
      dump_duration(out, "\tdelay", i->params.delaystart);
    else
      fprintf(out, "\tno-delay\n");

    fprintf(out, "\tlimits: readers %u, tables %u\n", i->params.max_readers,
            i->params.max_tables);

    fprintf(out, "\tdrop table: %s\n", i->params.drop_table ? "Yes" : "No");

    fprintf(out, "\t#---\n");
  }

  dump_duration(out, "timeout", global::config::timeout);
  fprintf(out, "cleanup: before %s, after %s\n",
          global::config::dont_cleanup_before ? "No" : "Yes",
          global::config::dont_cleanup_after ? "No" : "Yes");
}

} /* namespace config */

//-----------------------------------------------------------------------------

using namespace config;

actor_config::actor_config(actor_testcase testcase, const actor_params &params,
                           unsigned id, unsigned wait4id)
    : params(params) {
  this->id = id;
  this->order = (unsigned)global::actors.size();
  this->testcase = testcase;
  this->wait4id = wait4id;
  signal_nops = 0;
}

const std::string actor_config::serialize(const char *prefix) const {
  simple_checksum checksum;

  std::string result;
  if (prefix)
    result.append(prefix);

  checksum.push(params.pathname_db);
  result.append(params.pathname_db);
  result.append("|");

  checksum.push(params.pathname_log);
  result.append(params.pathname_log);
  result.append("|");

  static_assert(std::is_pod<actor_params_pod>::value,
                "actor_params_pod should by POD");
  result.append(data2hex(static_cast<const actor_params_pod *>(&params),
                         sizeof(actor_params_pod), checksum));
  result.append("|");

  static_assert(std::is_pod<actor_config_pod>::value,
                "actor_config_pod should by POD");
  result.append(data2hex(static_cast<const actor_config_pod *>(this),
                         sizeof(actor_config_pod), checksum));
  result.append("|");

  result.append(osal_serialize(checksum));
  result.append("|");

  result.append(std::to_string(checksum.value));
  return result;
}

bool actor_config::deserialize(const char *str, actor_config &config) {
  simple_checksum checksum;

  TRACE(">> actor_config::deserialize: %s\n", str);

  const char *slash = strchr(str, '|');
  if (!slash) {
    TRACE("<< actor_config::deserialize: slash-1\n");
    return false;
  }
  config.params.pathname_db.assign(str, slash - str);
  checksum.push(config.params.pathname_db);
  str = slash + 1;

  slash = strchr(str, '|');
  if (!slash) {
    TRACE("<< actor_config::deserialize: slash-2\n");
    return false;
  }
  config.params.pathname_log.assign(str, slash - str);
  checksum.push(config.params.pathname_log);
  str = slash + 1;

  slash = strchr(str, '|');
  if (!slash) {
    TRACE("<< actor_config::deserialize: slash-3\n");
    return false;
  }
  static_assert(std::is_pod<actor_params_pod>::value,
                "actor_params_pod should by POD");
  if (!hex2data(str, slash, static_cast<actor_params_pod *>(&config.params),
                sizeof(actor_params_pod), checksum)) {
    TRACE("<< actor_config::deserialize: actor_params_pod(%.*s)\n",
          (int)(slash - str), str);
    return false;
  }
  str = slash + 1;

  slash = strchr(str, '|');
  if (!slash) {
    TRACE("<< actor_config::deserialize: slash-4\n");
    return false;
  }
  static_assert(std::is_pod<actor_config_pod>::value,
                "actor_config_pod should by POD");
  if (!hex2data(str, slash, static_cast<actor_config_pod *>(&config),
                sizeof(actor_config_pod), checksum)) {
    TRACE("<< actor_config::deserialize: actor_config_pod(%.*s)\n",
          (int)(slash - str), str);
    return false;
  }
  str = slash + 1;

  slash = strchr(str, '|');
  if (!slash) {
    TRACE("<< actor_config::deserialize: slash-5\n");
    return false;
  }
  if (!config.osal_deserialize(str, slash, checksum)) {
    TRACE("<< actor_config::deserialize: osal\n");
    return false;
  }
  str = slash + 1;

  uint64_t verify = std::stoull(std::string(str));
  if (checksum.value != verify) {
    TRACE("<< actor_config::deserialize: checksum mismatch\n");
    return false;
  }

  TRACE("<< actor_config::deserialize: OK\n");
  return true;
}