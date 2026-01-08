## Run on WSL

### Start the HTTP server
```bash
cd /mnt/c/Academy/Main_project/Weather-Client-Server/Weather
make WeatherServer
./WeatherServer
```

Health check (new terminal):
```bash
curl -v http://127.0.0.1:8080/health
```

### Run the client
```bash
cd /mnt/c/Academy/Main_project/Weather-Client-Server/Weather
make Weather
./Weather
```

When prompted, choose a city. The client will request:
```
http://127.0.0.1:8080/weather?city=<CityName>
```
