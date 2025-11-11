URL_PREFIX="http://localhost:8080/api/v1/browser"

api_call() {
    curl "$URL_PREFIX/$*"
}
