# the CEF binary release we're going to download
CEFSDK_PULL_RELEASE="145.0.7632.0-2026-02-10-01"

URL_PREFIX="http://localhost:8080/api/v1/browser"
SLOT_0="wintest-1-0"
SLOT_1="wintest-1-1"
SLOT_3="wintest-1-2"

api_call() {
    echo "API: $URL_PREFIX/$*"
    curl "$URL_PREFIX/$*"
}

api_close() {
    api_call "close/$1"
}

api_back() {
    api_call "back/$1"
}

api_zoom() {
    api_call "zoom/$1/$2/$3"
}

api_print() {
    api_call "print/$1"
}

api_repaint() {
    api_call "repaint/$1"
}

api_list() {
    api_call "list"
}

api_seturl() {
    local slot="$1"
    local url="$2"
#    api_call "url/$slot/$url"
    curl -X GET -H "Url: $url" "$URL_PREFIX/navigate/$slot"
}

api_resize() {
    local slot="$1"
    local width="$2"
    local height="$3"
    api_call "resize/$slot/$width/$height"
}

api_stopload() {
    api_call "stop/$1"
}

api_closeall() {
    api_call "closeall"
}

api_script() {
    local slot="$1"
    shift
    curl -X GET -d "$*" "$URL_PREFIX/script/$slot"
}
