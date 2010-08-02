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

#include <glib.h>

#include "debug.h"
#include "cmds.h"
#include "smiley.h"
#include "plugin.h"
#include "version.h"

#define PLUGIN_ID "code-esmil-simple"
#define PLUGIN_AUTHOR "Emil Renner Berthing <esmil@mailme.dk>"
#define PLUGIN_WEBSITE "http://github.com/esmil/formula-filter"

static PurpleCmdId command_id;

static PurpleCmdRet 
command_cb(PurpleConversation *conv, const gchar *cmd,
           gchar **arg, gchar **error, void *userdata)
{
	PurpleSmiley *smiley;

	(void)cmd;
	(void)userdata;

	purple_debug_misc(PLUGIN_ID, "command_cb: arg[0] = '%s', arg[1] = '%s'\n",
	                  arg[0], arg[0] ? arg[1] : "(not present)");

	if (!(purple_conversation_get_features(conv)
	    & PURPLE_CONNECTION_ALLOW_CUSTOM_SMILEY)) {
		*error = g_strdup("This conversation doesn't allow custom smileys");
		return PURPLE_CMD_RET_FAILED;
	}

	if (arg[0] == NULL || arg[1] == NULL) {
		*error = g_strdup("Invalid arguments");
		return PURPLE_CMD_RET_FAILED;
	}

	smiley = purple_smiley_new_from_file(arg[0], arg[1]);
	if (smiley == NULL) {
		*error = g_strdup_printf("Error creating custom smiley from '%s'",
		                         arg[1]);
		return PURPLE_CMD_RET_FAILED;
	}

	purple_debug_info(PLUGIN_ID, "created custom smiley with "
	                  "shortcut = '%s', checksum = %s\n",
			  purple_smiley_get_shortcut(smiley),
			  purple_smiley_get_checksum(smiley));

	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	(void)plugin;

	command_id = purple_cmd_register(
		"cs",                 /* command name            */ 
		"ws",                 /* command argument format */
		PURPLE_CMD_P_DEFAULT, /* command priority flags  */  
		PURPLE_CMD_FLAG_IM,   /* command usage flags     */
		PLUGIN_ID,            /* plugin ID               */
		command_cb,           /* callback function       */
		/* help message */
		"cs &lt;shortcut&gt; &lt;path&gt;: "
		"Create custom smiley from image at &lt;path&gt;",
		NULL                  /* user-defined data       */
	);
	return command_id != 0;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	(void)plugin;

	purple_cmd_unregister(command_id);
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC, PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION,

	PURPLE_PLUGIN_STANDARD,   /* STANDARD plugin, not a LOADER or PROTOCOL */
	NULL,                     /* no UI requirement                         */
	0,                        /* flags                                     */
	NULL,                     /* no dependencies (set in plugin_init)      */
	PURPLE_PRIORITY_DEFAULT,  /* DEFAULT priority, not HIGHEST or LOWEST   */

	PLUGIN_ID,                /* plugin id      */
	"Custom Smiley Shortcut", /* plugin name    */
	"0.1",                    /* plugin version */

	/* plugin summary */
	"Create custom smiley with /cs command",
	/* plugin description */
	"Type /cs &lt;shortcut&gt; &lt;path&gt; in any IM conversation "
	"to create a custom smiley from image at &lt;path&gt;",
	PLUGIN_AUTHOR,   /* author  */
	PLUGIN_WEBSITE,  /* website */

	plugin_load,    /* gboolean plugin_load(PurplePlugin *plugin);   */
	plugin_unload,  /* gboolean plugin_unload(PurplePlugin *plugin); */
	NULL,           /* void plugin_destroy(PurplePlugin *plugin);    */

	NULL,           /* UI data         */
	NULL,           /* protocol data   */
	NULL,           /* preference info */

	/* GList plugin_actions(PurplePlugin *plugin, gpointer *context); */
	NULL,

	/* reserved for future use */
	NULL, NULL, NULL, NULL 
};

static void
init_plugin(PurplePlugin *plugin)
{
	(void)plugin;
}

PURPLE_INIT_PLUGIN(simple, init_plugin, info)
