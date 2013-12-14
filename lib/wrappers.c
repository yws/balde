/**
 * balde: A microframework for C based on GLib and bad intentions.
 * Copyright (C) 2013 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the LGPL-2 License.
 * See the file COPYING.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <string.h>
#include <balde/app.h>
#include <balde/exceptions.h>
#include <balde/wrappers.h>


void
balde_response_set_header(balde_response_t *response, gchar *name, gchar *value)
{
    // http header name is ascii
    gchar *new_name = g_new(gchar, strlen(name) + 1);
    guint i;
    for (i = 0; name[i] != '\0'; i++) {
        new_name[i] = g_ascii_tolower(name[i]);
    }
    new_name[i] = '\0';
    g_hash_table_replace(response->headers, new_name, g_strdup(value));
}


void
balde_response_append_body(balde_response_t *response, gchar *content)
{
    g_string_append(response->body, content);
}


balde_response_t*
balde_make_response(gchar *content)
{
    balde_response_t *response = g_new(balde_response_t, 1);
    response->status_code = 200;
    response->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    balde_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    response->body = g_string_new(content);
    return response;
}


void
balde_response_free(balde_response_t *response)
{
    if (response == NULL)
        return;
    g_hash_table_destroy(response->headers);
    g_string_free(response->body, TRUE);
    g_free(response);
}


balde_response_t*
balde_make_response_from_exception(GError *error)
{
    if (error == NULL)
        return NULL;
    guint status_code = error->code;
    const gchar *name = balde_exception_get_name_from_code(status_code);
    const gchar *description;
    gchar* new_description;
    if (name == NULL) {
        status_code = 500;
        name = balde_exception_get_name_from_code(status_code);
        const gchar *description = balde_exception_get_description_from_code(status_code);
        new_description = g_strdup_printf("%s\n\n%s", description,
            error->message);
    }
    else
        new_description = g_strdup(balde_exception_get_description_from_code(status_code));
    gchar *content = g_strdup_printf(
        "Error: %d %s\n\n%s\n", status_code, name, new_description);
    balde_response_t *response = balde_make_response(content);
    g_free(content);
    g_free(new_description);
    response->status_code = status_code;
    balde_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");
    return response;
}


void
balde_fix_header_name(gchar *name)
{
    for (guint i = 0; name[i] != '\0'; i++)
        if (i == 0 || name[i - 1] == '-')
            name[i] = g_ascii_toupper(name[i]);
}


void
balde_header_render(gchar *key, gchar *value, GString *str)
{
    balde_fix_header_name(key);
    g_string_append_printf(str, "%s: %s\r\n", key, value);
}


gchar*
balde_response_render(balde_response_t *response)
{
    if (response == NULL)
        return NULL;
    GString *str = g_string_new("");
    if (response->status_code != 200)
        g_string_append_printf(str, "Status: %d %s\r\n", response->status_code,
            balde_exception_get_name_from_code(response->status_code));
    gchar *len = g_strdup_printf("%zu", strlen(response->body->str));
    // we shouldn't trust response->body->len
    balde_response_set_header(response, "Content-Length", len);
    g_free(len);
    g_hash_table_foreach(response->headers, (GHFunc) balde_header_render, str);
    g_string_append(str, "\r\n");
    g_string_append(str, response->body->str);
    return g_string_free(str, FALSE);
}


void
balde_response_print(balde_response_t *response)
{
    g_print("%s", balde_response_render(response));
}