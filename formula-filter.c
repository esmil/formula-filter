/*
 * This file is part of formula-filter.
 *
 * formula-filter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or(at your option) any later version.
 *
 * formula-filter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with formula-filter.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* config.h may define PURPLE_PLUGINS; protect the definition here so that
 * we don't get complaints about redefinition when it's not necessary. */
#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>

/* this will prevent compiler errors in some instances
 * and is better explained in the how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

#include "debug.h"
#include "notify.h"
#include "cmds.h"
#include "smiley.h"
#include "plugin.h"
#include "version.h"

#define PLUGIN_ID "code-esmil-formula-filter"
#define PLUGIN_AUTHOR "Emil Renner Berthing <esmil@mailme.dk>"
#define PLUGIN_WEBSITE "http://github.com/esmil/formula-filter"

#define PREFS_BASE    "/plugins/core/formula-filter"
#define PREF_PATH  PREFS_BASE "/path"

static PurplePlugin *this_plugin = NULL;
static gint smiley_id;
static gchar *new_message;
static gchar error_message[1];

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	(void)plugin;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_label("Script");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(PREF_PATH, "Path");
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static int
execute(char *cmd[])
{
	int exitcode = -1;
	int exitstatus;
	pid_t child_id = 0;

	child_id = fork();
	if (child_id == 0) {
		exitcode = execvp(cmd[0], cmd);
		exit(exitcode);
	}
	wait(&exitstatus);

	if (WIFEXITED(exitstatus))
		exitcode = WEXITSTATUS(exitstatus);

	return exitcode;
}

static void
show_error_message(PurpleAccount *account, const char *receiver,
                   const char *str, int ret)
{
	PurpleConversation *conv;
	gchar *msg;

	conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY,
	                                             receiver, account);

	if (conv == NULL) {
#ifndef NDEBUG
		g_print("show_error_message: conv == NULL\n");
#endif
		return;
	}

	if (ret)
		msg = g_strdup_printf("Error creating smiley from '%s', "
		                      "return code = %d", str, ret);
	else
		msg = g_strdup_printf("Error creating smiley from '%s'", str);

	purple_conversation_write(conv, NULL, msg,
	                          PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_ERROR,
	                          time(NULL));
	g_free(msg);
}

static void
sending_im_msg_cb(PurpleAccount *account, const char *receiver,
                  char **message)
{
	char **v;
	char **p;
	char *cmd[4];

	(void)receiver;

	if (strstr(*message, "$$") == NULL)
		return;

#ifndef NDEBUG
	g_print("sending_im_msg_cb: message = '%s'\n", *message);
#endif

	cmd[0] = (char *)purple_prefs_get_string(PREF_PATH);
	cmd[2] = "/tmp/formula-filter.png";
	cmd[3] = NULL;

	v = g_strsplit(*message, "$$", -1);

	p = v + 1;
	do {
		int ret;
		gchar *shortcut;
		PurpleSmiley *smiley;

		cmd[1] = *p;

		ret = execute(cmd);
		if (ret) {
			show_error_message(account, receiver, *p, ret);
			goto error;
		}


		shortcut = g_strdup_printf("ff-%d", smiley_id++);
		smiley = purple_smiley_new_from_file(shortcut, "/tmp/formula-filter.png");
		if (smiley == NULL) {
			show_error_message(account, receiver, *p, 0);
			goto error;
		}

#ifndef NDEBUG
		g_print("created new smiley, %s, checksum: %s\n",
		        shortcut, purple_smiley_get_checksum(smiley));
#endif
		g_free(*p);
		*p = shortcut;
	} while (*(++p) && *(++p));

	new_message = g_strjoinv(NULL, v);
	g_strfreev(v);

	free(*message);
	*message = g_strdup(new_message);

#ifndef NDEBUG
	g_print("final message = '%s'\n", *message);
#endif

	return;

error:
	g_strfreev(v);
	g_free(*message);
	*message = NULL;
	new_message = error_message;
}

static gboolean
writing_im_msg_cb(PurpleAccount *account, const char *who,
                  char **message, PurpleConversation *conv,
                  PurpleMessageFlags flags)
{
	(void)account;
	(void)who;
	(void)conv;

	purple_debug_info(PLUGIN_ID, "writing_im_msg_cb: message = '%s'\n",
	                  *message);

#ifndef NDEBUG
	g_print("writing_im_msg_cb: message = '%s'\n", *message);
#endif

	if (new_message == NULL || (flags & PURPLE_MESSAGE_SEND) == 0)
		return FALSE;

	if (new_message == error_message)
		return TRUE;

#ifndef NDEBUG
	g_print("new_message = '%s'\n", new_message);
#endif
	g_free(*message);
	*message = new_message;
	new_message = NULL;

	return FALSE;
}

static void
remove_smileys()
{
	GList *p;

	for (p = purple_smileys_get_all(); p; p = p->next) {
		if (g_str_has_prefix(purple_smiley_get_shortcut(p->data), "ff-"))
			purple_smiley_delete(p->data);
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	void *conv_handle;

	purple_debug_info(PLUGIN_ID, "Loading..\n");
	remove_smileys();

	this_plugin = plugin;

	conv_handle = purple_conversations_get_handle();
	purple_signal_connect(conv_handle, "writing-im-msg", plugin,
	                      PURPLE_CALLBACK(writing_im_msg_cb), NULL);
	purple_signal_connect(conv_handle, "sending-im-msg", plugin,
	                      PURPLE_CALLBACK(sending_im_msg_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	void *conv_handle = purple_conversations_get_handle();

	purple_debug_info(PLUGIN_ID, "Unloading..\n");

	purple_signal_disconnect(conv_handle, "writing-im-msg", plugin,
	                         PURPLE_CALLBACK(writing_im_msg_cb));
	purple_signal_disconnect(conv_handle, "sending-im-msg", plugin,
	                         PURPLE_CALLBACK(sending_im_msg_cb));

	remove_smileys();

	return TRUE;
}

static PurplePluginUiInfo prefs_info = {
	/* PurplePluginPrefFrame *get_plugin_pref_frame(PurplePlugin *plugin); */
	get_plugin_pref_frame,

	0,    /* page_num (Reserved) */
	NULL, /* frame (Reserved)    */

	/* Padding */
	NULL, NULL, NULL, NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC, PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION,

	PURPLE_PLUGIN_STANDARD,  /* STANDARD plugin, not a LOADER or PROTOCOL */
	NULL,                    /* no UI requirement                         */
	0,                       /* flags                                     */
	NULL,                    /* no dependencies (set in plugin_init)      */
	PURPLE_PRIORITY_DEFAULT, /* DEFAULT priority, not HIGHEST or LOWEST   */

	PLUGIN_ID,               /* plugin id      */
	"Formula Filter",        /* plugin name    */
	"0.1",                   /* plugin version */

	/* plugin summary */
	"Formula Filter plugin",
	/* plugin description */
	"Runs text between $$'s through a script to create"
	" custom smileys on the fly. E.g. using latex.",
	PLUGIN_AUTHOR,           /* author  */
	PLUGIN_WEBSITE,          /* website */

	plugin_load,    /* gboolean plugin_load(PurplePlugin *plugin);   */
	plugin_unload,  /* gboolean plugin_unload(PurplePlugin *plugin); */
	NULL,           /* void plugin_destroy(PurplePlugin *plugin);    */

	NULL,           /* UI data         */
	NULL,           /* protocol data   */
	&prefs_info,    /* preference info */

	/* GList plugin_actions(PurplePlugin *plugin, gpointer *context); */
	NULL,

	/* reserved for future use */
	NULL, NULL, NULL, NULL 
};

static void
init_plugin(PurplePlugin *plugin)
{
	(void)plugin;

	purple_prefs_add_none(PREFS_BASE);
	purple_prefs_add_string(PREF_PATH, "/usr/share/formula-filter/ff.sh");
}

PURPLE_INIT_PLUGIN(formula-filter, init_plugin, info)

/*
 * vim: ts=4 sw=4:
 */
