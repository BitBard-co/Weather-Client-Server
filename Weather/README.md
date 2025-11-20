# Weather HTTP Server & Client

Modernized version focused on serving and querying weather data over HTTP. The legacy TCP chat demo and original CLI still exist but are secondary.

---
## 1. Overview
Components:
- `server_http`: Minimal C HTTP server exposing weather & city endpoints (GET + POST).
- Interactive HTTP client (`client`): Lists cities, fetches weather.
- Shared modules: cities, meteo (Open‑Meteo APIs), caching, normalization for Swedish characters.

Default HTTP port: **22** (matches existing router forward external→internal). Use an unprivileged port (e.g. 18080) if you cannot run with `sudo` or port 22 is taken.

---
## 2. Endpoints
GET:
- `/health` → `{"status":"ok"}`
- `/cities` → JSON array of known cities
- `/weather?city=<Name>` → Raw Open‑Meteo forecast JSON (server auto‑adds missing city)

POST (JSON bodies):
- `/city` `{"name":"Göteborg"}` → 200 if existing, 201 if added; returns name + coordinates
- `/weather` `{"city":"Stockholm"}` → Same weather JSON as GET variant

Swedish names (å ä ö) accepted directly. Internal matching normalizes diacritics; stored names keep originals.

---
## 3. Quick Start (WSL)
```bash
cd /mnt/c/Academy/Main_project/Weather-Client-Server/Weather
make -C server http                 # build HTTP server
sudo ./server/server_http           # listen on 0.0.0.0:22
```
Test:
```bash
curl -s http://127.0.0.1:22/health
curl -s http://127.0.0.1:22/cities | jq .
curl -s "http://127.0.0.1:22/weather?city=Stockholm" | jq .
```
Client:
```bash
make -C client                      # build client
./client                            # defaults to http://127.0.0.1:22
```
Alternate port (no sudo):
```bash
make -C server http
./server/server_http --port 18080   # now client must specify base URL
./client http://127.0.0.1:18080
```

---
## 4. Building (General)
Dependencies: C compiler (C17), libcurl, OpenSSL (MD5 hashing), cJSON (vendored).
- Linux (Ubuntu): `sudo apt install -y build-essential libssl-dev libcurl4-openssl-dev`
- macOS (Homebrew): Install `openssl@3` then `make`
- Windows: Use WSL (recommended). Native build requires curl + OpenSSL dev libs (MSYS2 or vcpkg).

Top-level Weather CLI build (optional):
```bash
make
make run
```

---
## 5. Running the HTTP Server
From repository root (`Weather/`):
```bash
make -C server http
sudo ./server/server_http                # port 22
```
Specify address/port:
```bash
sudo ./server/server_http --addr 0.0.0.0 --port 22
./server/server_http --port 18080        # unprivileged
```
Indicators: On success it prints `HTTP server listening on 0.0.0.0:<port>`.

Background (example high port):
```bash
nohup ./server/server_http --port 18080 > server_http.log 2>&1 &
ss -lntp | grep 18080
```

Stopping:
```bash
pkill -f server_http
```

---
## 6. Interactive Client
```bash
make -C client
./client                 # uses DEFAULT_HTTP_BASE (port 22)
./client http://127.0.0.1:18080   # override base URL
```
Loop: lists cities → prompt → fetch weather JSON.
Failure message `Not connected to HTTP server` indicates connection refusal (server not listening / wrong port).

---
## 7. POST Examples
```bash
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Göteborg"}' http://127.0.0.1:22/city | jq .

curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"city":"Stockholm"}' http://127.0.0.1:22/weather | jq .
```
PowerShell quoting workaround:
```powershell
echo '{"city":"Stockholm"}' > body.json
curl -s -X POST -H "Content-Type: application/json" --data @body.json http://127.0.0.1:22/weather
```

---
## 8. External Access (Teacher Testing)
Requirements:
- Router forwarding external_port → internal `192.168.1.210:22`.
- Server running (sudo) in WSL on port 22.
- If WSL needs portproxy (Windows host intercepts):
  ```powershell
  netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=22 connectaddress=<WSL_IP> connectport=22
  ```
Test from outside LAN (mobile data):
```bash
curl http://<public_ip>:<external_port>/health
```
Returns `{"status":"ok"}` if reachable.

---
## 9. Caching & City Addition
- Cities stored as JSON under `cities/` (added automatically when missing via geocoding API).
- Weather responses cached raw under `cache/` for ~15 minutes (MD5 of request URL as filename).

---
## 10. Swedish Character Handling
Input normalization maps å, ä → a; ö → o (both UTF‑8 and Latin‑1 forms). Matching is case-insensitive; original spelling preserved in stored files and responses.

---
## 11. Troubleshooting
```bash
ss -lntp | grep :22           # confirm listening
curl -v http://127.0.0.1:22/health
```
Common issues:
- Permission denied on port 22 → use `sudo` or a high port.
- Address in use → another service (sshd) occupies 22; stop it or pick 18080.
- Client says not connected → mismatch port or server exited.
- POST returns invalid json → body quoting error; use file or proper escape.

---
## 12. Development Notes
- `server/http_server.c` parses minimal HTTP (single request per connection).
- No concurrency (each accept handled sequentially). For scaling: add fork/thread or event loop.
- Security is minimal; do not expose directly to the internet without hardening (rate limits, validation, HTTPS proxy).

---
## 13. Extending
- Add hourly/daily forecasts: extend query string.
- Implement persistent city list preloading on server start instead of per-request init.
- Add structured client output (parse weather JSON instead of raw print).

---
## 14. License / Attribution
Uses Open‑Meteo APIs (free). Includes cJSON & libcurl; OpenSSL for MD5 hashing of cache keys.

---
## 15. Quick Command Reference
```bash
# Build server & client
make -C server http
make -C client

# Run server (privileged)
sudo ./server/server_http

# Run server (alternate port)
./server/server_http --port 18080

# Test endpoints
curl http://127.0.0.1:22/health
curl "http://127.0.0.1:22/weather?city=Gävle"

# Client interaction
./client
./client http://127.0.0.1:18080
```

---
Happy hacking!
