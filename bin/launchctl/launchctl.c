/*+
 * Copyright 2015 iXsystems, Inc.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <dirent.h>
#include <err.h>
#include <launch.h>
#include <jansson.h>

#define N(x)    ((sizeof(x)) / (sizeof(x[0])))

static launch_data_t to_launchd(json_t *json);
static void to_json_dict(const launch_data_t lval, const char *key, void *ctx);
static json_t *to_json(launch_data_t ld);
static json_t *launch_msg_json(json_t *msg);

static int cmd_start_stop(int argc, char * const argv[]);
static int cmd_bootstrap(int argc, char * const argv[]);
static int cmd_submit(int argc, char * const argv[]);
static int cmd_remove(int argc, char * const argv[]);
static int cmd_list(int argc, char * const argv[]);
static int cmd_dump(int argc, char * const argv[]);
static int cmd_help(int argc, char * const argv[]);

mach_port_t bootstrap_port;

static const struct {
	const char *name;
	int (*func)(int argc, char * const argv[]);
	const char *desc;
} cmds[] = {
	{ "start",	cmd_start_stop,	"Start specified job" },
	{ "stop",	cmd_start_stop,	"Stop specified job" },
	{ "submit",	cmd_submit,	"Submit a job from the command line" },
	{ "remove",	cmd_remove, 	"Remove specified job" },
	{ "bootstrap",	cmd_bootstrap,	"Bootstrap launchd" },
	{ "list",	cmd_list,	"List jobs and information about jobs" },
	{ "dump",	cmd_dump,       "Dumps job(s) plis(s)"},
	{ "help",	cmd_help,	"This help output" },
};


static const char *bootstrap_paths[] = {
	"/conf/base/etc/plists",
	"/etc/plists",
	"/usr/local/etc/plists",
};


static launch_data_t
to_launchd(json_t *json)
{
	size_t idx;
	launch_data_t arr, dict;
	const char *key;
	json_t *val;

	switch (json_typeof(json)) {
	case JSON_STRING:
		return launch_data_new_string(json_string_value(json));

	case JSON_INTEGER:
		return launch_data_new_integer(json_integer_value(json));

	case JSON_TRUE:
		return launch_data_new_bool(true);

	case JSON_FALSE:
		return launch_data_new_bool(false);

	case JSON_ARRAY:
		arr = launch_data_alloc(LAUNCH_DATA_ARRAY);
		json_array_foreach(json, idx, val) {
			launch_data_array_set_index(arr, to_launchd(val), idx);
		}

		return arr;

	case JSON_OBJECT:
		dict = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		json_object_foreach(json, key, val) {
			launch_data_dict_insert(dict, to_launchd(val), key);
		}

		return dict;

	case JSON_REAL:
	case JSON_NULL:
		return NULL;

	}

	return NULL;
}

static void
to_json_dict(const launch_data_t lval, const char *key, void *ctx)
{
	json_t *obj = (json_t *)ctx;
	json_object_set_new(obj, key, to_json(lval));
}

static json_t *
to_json(launch_data_t ld)
{
	json_t *arr, *obj;
	size_t i;

	switch (launch_data_get_type(ld)) {
	case LAUNCH_DATA_STRING:
		return json_string(launch_data_get_string(ld));

	case LAUNCH_DATA_INTEGER:
		return json_integer(launch_data_get_integer(ld));

	case LAUNCH_DATA_BOOL:
		return json_boolean(launch_data_get_bool(ld));

	case LAUNCH_DATA_ARRAY:
		arr = json_array();
		for (i = 0; i < launch_data_array_get_count(ld); i++) {
			json_array_append_new(arr, to_json(launch_data_array_get_index(ld, i)));
		}

		return arr;

	case LAUNCH_DATA_DICTIONARY:
		obj = json_object();
		launch_data_dict_iterate(ld, to_json_dict, obj);
		return obj;

	default:
		return json_null();
	}
}

static json_t *
launch_msg_json(json_t *input)
{
	launch_data_t result;

	result = launch_msg(to_launchd(input));

	if (result == NULL) {
		fprintf(stderr, "launchd returned NULL in response\n");
		return (NULL);
	}

	return to_json(result);
}

static int
cmd_bootstrap(int argc, char * const argv[])
{
	struct dirent **files;
	char *args[2] = {(char *)"submit", NULL};
	char *name, *path;
	unsigned long i;
	int n;

	(void)argc;
	(void)argv;

	printf("Bootstrap:\n");

	system("/sbin/mount -uw /");

	for (i = 0; i < N(bootstrap_paths); i++) {
		n = scandir(bootstrap_paths[i], &files, NULL, alphasort);
		if (n < 0)
			continue;

		while (n--) {
			name = files[n]->d_name;
			if (name[0] == '.')
				continue;

			printf("\t");
			asprintf(&path, "%s/%s", bootstrap_paths[i], name);
			args[1] = path;
			cmd_submit(2, args);
		}
	}

	return (0);
}

static int
cmd_submit(int argc, char * const argv[])
{
	FILE *input;
	json_error_t err;
	json_t *msg, *plist;

	if (argc < 2)
		errx(1, "Usage: launchctl submit <plist>");

	input = strcmp(argv[1], "-") ? fopen(argv[1], "r") : stdin;
	if (input == NULL)
		errx(1, "Cannot open file %s: %s\n", argv[1], strerror(errno));

	plist = json_loadf(input, JSON_DECODE_ANY, &err);
	msg = json_object();
	json_object_set_new(msg, "SubmitJob", plist);

	if (launch_msg_json(msg) == NULL)
		return (1);

	printf("%s\n", json_string_value(json_object_get(plist, "Label")));
	return (0);
}

static int
cmd_start_stop(int argc, char * const argv[])
{
	json_t *msg;

	msg = json_object();

	if (argc < 2)
		errx(1, "Usage: %s <jobname>\n", argv[0]);

	if (!strcmp(argv[0], "start"))
		json_object_set(msg, "StartJob", json_string(argv[1]));

	if (!strcmp(argv[0], "stop"))
		json_object_set(msg, "StartJob", json_string(argv[1]));

	launch_msg_json(msg);
	return (0);
}

static int
cmd_remove(int argc, char * const argv[])
{
	json_t *msg;

	msg = json_object();

	if (argc < 2)
		errx(1, "Usage: remove <jobname>");

	json_object_set(msg, "RemoveJob", json_string(argv[1]));
	launch_msg_json(msg);
	return (0);
}

static int
cmd_list(int argc, char * const argv[])
{
	json_t *msg, *result, *job;
	const char *key;

	(void)argc;
	(void)argv;

	msg = json_string("GetJobs");
	result = launch_msg_json(msg);

	if (result == NULL)
		errx(1, "Invalid response from launchd");

	json_object_foreach(result, key, job) {
		printf("%s\n", key);
	}

	return (0);
}


static int
cmd_dump(int argc, char * const argv[])
{
	json_t *msg, *result;

	if (argc == 1)
		msg = json_string("GetJobs");

	if (argc == 2) {
		msg = json_object();
		json_object_set(msg, "GetJob", json_string(argv[1]));
	}

	result = launch_msg_json(msg);
	json_dumpf(result, stdout, JSON_INDENT(4));
	return (0);
}

static int
cmd_help(int argc, char * const argv[])
{
	size_t i;

	(void)argc;
	(void)argv;

	fprintf(stderr, "Usage: launchctl <subcommand> [arguments...]\n");
	fprintf(stderr, "\n");

	for (i = 0; i < N(cmds); i++) {
		fprintf(stderr, "%s - %s\n", cmds[i].name, cmds[i].desc);
	}

	return (0);
}


int
main(int argc, char * const argv[])
{
	const char *cmd;
	size_t i;
	int c;

	while ((c = getopt(argc, argv, "S:h")) != -1) {
		switch (c) {

		}
	}

	cmd = argv[optind];

	for (i = 0; i < N(cmds); i++) {
		if (!strcmp(cmd, cmds[i].name))
			return (cmds[i].func(argc - optind, argv + optind));
	}

	fprintf(stderr, "Usage: launchctl <subcommand> [arguments...]\n");
	return (1);
}
