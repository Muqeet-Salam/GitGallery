# GitHub Image Viewer

GitHub Image Viewer is a C backend + static frontend app for browsing images in GitHub repositories.

It supports:

- Repository URL input
- Folder-based navigation
- Breadcrumb navigation
- Recursive image loading toggle
- Server-side pagination (50 images per page)

## Tech Stack

- Backend: C, libmicrohttpd, libcurl
- Frontend: HTML, CSS, vanilla JavaScript

## Project Structure

```text
.
├── src/
│   ├── main.c
│   ├── server.c
│   ├── server.h
│   ├── router.c
│   ├── router.h
│   ├── github.c
│   ├── github.h
│   ├── parser.c
│   ├── parser.h
│   ├── utils.c
│   └── utils.h
├── public/
│   ├── index.html
│   ├── style.css
│   └── script.js
├── build/
├── Makefile
└── README.md
```

## Requirements

- gcc
- libcurl development headers
- libmicrohttpd development headers
- pkg-config (optional, recommended)

On Debian/Ubuntu:

```bash
sudo apt install build-essential libcurl4-openssl-dev libmicrohttpd-dev pkg-config
```

## Build and Run

```bash
make
./build/server
```

Open http://localhost:8080 in your browser.

Stop the server with Ctrl+C.

## API

Endpoint:

```text
GET /api/images
```

Query parameters:

- repo (required): full GitHub repository URL
- path (optional): folder path inside the repo
- recursive (optional): 1 (default) or 0
- page (optional): page number, default 1
- per_page (optional): page size, default 50

Response shape:

```json
{
   "path": "",
   "folders": [
      { "name": "assets", "path": "assets" }
   ],
   "images": [
      {
         "name": "logo.png",
         "path": "assets/logo.png",
         "download_url": "https://raw.githubusercontent.com/..."
      }
   ],
   "pagination": {
      "page": 1,
      "per_page": 50,
      "total_images": 192,
      "total_pages": 4
   }
}
```

## Notes

- Pagination reduces the number of images loaded per request.
- Image URLs point to original files on GitHub raw content.
- Very large repositories may still be slower due to GitHub API size and rate limits.


