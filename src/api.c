#include <stdlib.h>

#include "api.h"
#include "config.h"
#include "log.h"
#include "options.h"
#include "util.h"


int api_init() {
    char *user_agent;
    ds_asprintf(&user_agent, "Floobits Diffshipper %s", PACKAGE_VERSION);
    curl_global_init(CURL_GLOBAL_ALL);
    req = calloc(1, sizeof(api_req_t));
    req->curl = curl_easy_init();
    curl_easy_setopt(req->curl, CURLOPT_USERAGENT, user_agent);
    /* TODO: verify cert! */
    curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(req->curl, CURLOPT_VERBOSE, 1);

    free(user_agent);

    return 0;
}


void api_cleanup() {
    curl_global_cleanup();
    free(req);
}


static int api_go() {
    CURLcode res;
    long http_status;

    curl_formadd(&(req->p_first),
                 &(req->p_last),
                 CURLFORM_COPYNAME, "username",
                 CURLFORM_COPYCONTENTS, opts.username,
                 CURLFORM_END);
    curl_formadd(&(req->p_first),
                 &(req->p_last),
                 CURLFORM_COPYNAME, "secret",
                 CURLFORM_COPYCONTENTS, opts.secret,
                 CURLFORM_END);

    res = curl_easy_perform(req->curl);
    if (res)
        die("Request failed: %s", curl_easy_strerror(res));

    curl_easy_getinfo(req->curl, CURLINFO_RESPONSE_CODE, &http_status);

    log_debug("Got HTTP status code %li\n", http_status);

    if (http_status <= 199 || http_status > 299) {
      if (http_status == 401) {
          log_err("Access denied. Probably a bad username or API secret.");
      } else if (http_status == 403) {
          log_err("HTTP Status 403: Your credentials are valid, but you don't have permission.");
      } else if (http_status == 409) {
          log_err("HTTP Status 409: Conflict. A room with the same name and owner already exists.");
      }
      return -1;
    }

    curl_formfree(req->p_first);
    return 0;
}


int api_create_room() {
    curl_formadd(&(req->p_first),
                 &(req->p_last),
                 CURLFORM_COPYNAME, "name",
                 CURLFORM_COPYCONTENTS, opts.room,
                 CURLFORM_END);

    if (opts.room_perms >= 0) {
        char *room_perms;
        ds_asprintf(&room_perms, "%i", opts.room_perms);
        curl_formadd(&(req->p_first),
                     &(req->p_last),
                     CURLFORM_COPYNAME, "perms",
                     CURLFORM_COPYCONTENTS, room_perms,
                     CURLFORM_END);
        free(room_perms);
    }

    curl_easy_setopt(req->curl, CURLOPT_HTTPPOST, req->p_first);
    curl_easy_setopt(req->curl, CURLOPT_URL, opts.api_url);
    return api_go();
}


int api_delete_room() {
    char *url;
    ds_asprintf(&url, "%s%s/%s/", opts.api_url, opts.owner, opts.room);
    curl_easy_setopt(req->curl, CURLOPT_HTTPPOST, req->p_first);
    curl_easy_setopt(req->curl, CURLOPT_URL, url);
    free(url);

    curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    return api_go();
}
