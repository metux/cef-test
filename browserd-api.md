browserd REST API v1
====================

* API mountpoint: /api/v1/browser/
* browser ID's are client defined, allowed characters: [A-Za-z0-9\.\_\+]

/create -- create browser
-------------------------

* GET /create/{id}/{xid}/{width}/{height}
* Header: Webhook - URL prefix to notification webhook endpoint
* Header: Url - initial URL to load (optional)

/script -- inject and execute script
------------------------------------

* POST /script/{id}
* script is transferred as request body

/list -- list all active browser
--------------------------------

* GET /list
* reply body: list of active browser

/stop -- stop loading in a browser
----------------------------------

* GET /stop/{id}

/nagivate -- navigate to new URL
--------------------------------

* GET /navigate/{id}[/{url-urlencoded}]
* Header: Url - the new URL (optional)
* the new URL can be either given as path component (urlencoded) or as Url-header (plain)

/reload -- reload browser
-------------------------

* GET /reload/{id}

/back -- go back in history
---------------------------

* GET /back/{id}

/forward -- go forward in history
---------------------------------

* GET /forward/{id}

/closeall -- close all browser
------------------------------

* GET /closeall

/shutdown -- shut down browserd
-------------------------------

* GET /closeall

/close -- close browser
-----------------------

* GET /close/{id}
