#include <kore/kore.h>
#include <kore/http.h>
#include <sys/syslog.h>
#include "dbh.h"

int	page(struct http_request *);

int page(struct http_request *req)
{
    kore_log(LOG_INFO, "Index handler was called!");
    const char* html= "<h1>hello there</h1>";
    http_response(req, 200, html, strlen(html));
    return (KORE_RESULT_OK);
}

int create_user(struct http_request *req)
{
    struct kore_json json;
    struct kore_json_item *item;

    if (req->http_body == NULL)
    {
        http_response(req, 400, "Empty Body", 10);
        return KORE_RESULT_OK;
    }

    kore_json_init(&json, req->http_body, req->http_body->length);

    if (!kore_json_parse(&json))
    {
        kore_log(LOG_NOTICE, "JSON parse error: %s", kore_json_strerror());
        http_response(req, 400, "Invalid JSON", 12);
        kore_json_cleanup(&json);
        return KORE_RESULT_OK;
    }

    item = kore_json_find_string(json.root, "name");
    if (item != NULL)
    {
        kore_log(LOG_INFO, "Received name: %s", item->data.string);
    }

    kore_json_cleanup(&json);
    return KORE_RESULT_OK;
}
