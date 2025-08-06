---
title: Directus
description: Directus 9. An Instant App & API for your SQL Database.
buttonSource: https://railway.app/new/template/_dszdt?referralCode=codedgeekery
tags:
  - directus
  - cms
  - javascript
  - typescript
  - postgresql
  - s3
---

# Directus On Railway

This example deploys a self-hosted version of [Directus](https://directus.io). 

Internally it uses a PostgreSQL database to store the data and S3 to store files.

[![Deploy on Railway](https://railway.app/button.svg)](https://railway.app/new/template/_dszdt?referralCode=codedgeekery)

## ✨ Features

- Directus
- Postgres
- S3
- Slugs (via inclusion of [https://github.com/dimitrov-adrian/directus-extension-wpslug-interface](https://github.com/dimitrov-adrian/directus-extension-wpslug-interface))

## 💁‍♀️ How to use

- Click the Railway button 👆
- Add the environment variables
  - If you do not add the S3 related environment variables, your images/files will not be persisted between deploys.

## 📝 Notes

- After your app is deployed, visit the `/admin` endpoint to login using the initial admin user you entered during config.
- Railway's filesystem is ephemeral which is why any changes to the filesystem are not persisted between deploys. This is why, this example uses S3 for storage.

## Credit

Originally forked from [https://github.com/azrikahar/directus-railway-starter](https://github.com/azrikahar/directus-railway-starter) with S3 and Slugs support built in off the bat.

# Security Camera SMS Relay with Twilio

A local webhook server that receives security camera alerts and forwards them as SMS messages to your phone using Twilio. This solution works when your camera system doesn't have native SMS support but can send HTTP notifications.

## Features

- 🏠 **Local Operation**: Runs on your local network, cameras never connect to the cloud
- 📱 **Twilio SMS**: Reliable SMS delivery through Twilio's infrastructure
- 🎥 **Multi-Camera Support**: Works with Hikvision, Dahua, Blue Iris, and any camera with webhook support
- 🔄 **Smart Batching**: Groups multiple alerts to prevent SMS spam
- ⏰ **Rate Limiting**: Prevents excessive notifications
- 🌙 **Quiet Hours**: Suppress non-critical alerts during sleeping hours
- 🔐 **Security**: Optional webhook authentication and IP whitelisting
- 📊 **Monitoring**: Built-in health checks and statistics

## How It Works

```
Camera → Motion Detection → HTTP Webhook → Local Relay Server → Twilio API → SMS to Phone
```

1. Your camera detects motion or other events
2. Camera sends HTTP notification to your local relay server
3. Relay server processes the event and applies rules
4. Server sends SMS via Twilio API
5. You receive the alert on your phone

## Quick Start

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

### 2. Set Up Twilio

1. Sign up for a Twilio account at https://www.twilio.com
2. Get your Account SID and Auth Token from the Twilio Console
3. Buy a phone number (costs ~$1/month)

### 3. Configure the Relay

Run the script once to generate a default `config.yaml`:

```bash
python twilio_camera_relay.py
```

Edit `config.yaml` with your settings:

```yaml
twilio:
  account_sid: "YOUR_TWILIO_ACCOUNT_SID"
  auth_token: "YOUR_TWILIO_AUTH_TOKEN"
  from_number: "+1234567890"  # Your Twilio phone number

recipients:
  - name: "John"
    number: "+1987654321"  # Your mobile number
    cameras: ["all"]  # or specific camera IDs
    active: true

cameras:
  camera_id_mapping:
    "192.168.1.100": "Front Door"
    "192.168.1.101": "Backyard"
    "cam1": "Living Room"
```

### 4. Configure Your Cameras

#### Hikvision
1. Go to: Configuration → Event → Basic Event → Motion Detection
2. Enable motion detection
3. Go to: Configuration → Event → Basic Event → Notify Surveillance Center
4. Add URL: `http://YOUR_SERVER_IP:8888/webhook`
5. Protocol: HTTP

#### Dahua
1. Go to: Setting → Event → Video Detection → Motion Detection
2. Enable motion detection
3. Go to: Setting → Network → Alarm Server
4. Server Address: YOUR_SERVER_IP
5. Port: 8888
6. URL: /webhook

#### Blue Iris
1. Camera Properties → Alerts → Actions
2. On Alert: Web request or MQTT
3. URL: `http://YOUR_SERVER_IP:8888/webhook`
4. POST text: `{"camera_id":"&CAM","event_type":"motion"}`

### 5. Run the Server

```bash
python twilio_camera_relay.py
```

### 6. Test

Send a test notification:

```bash
curl -X POST http://localhost:8888/test
```

## Configuration Options

### Recipients

```yaml
recipients:
  - name: "Primary Contact"
    number: "+1234567890"
    cameras: ["all"]  # or ["front_door", "backyard"]
    priority: "high"
    active: true
```

### Notification Rules

```yaml
notifications:
  rate_limiting:
    enabled: true
    max_per_hour: 30
    max_per_camera_per_hour: 10
    
  batching:
    enabled: true
    window_seconds: 300  # Group alerts within 5 minutes
    max_batch_size: 5
    
  quiet_hours:
    enabled: true
    start: "22:00"
    end: "07:00"
    allow_priority: true  # Still send high priority alerts
```

### Security

```yaml
webhook:
  auth_enabled: true
  auth_token: "your_secret_token"
  allowed_ips: ["192.168.1.100", "192.168.1.101"]  # Camera IPs
```

## API Endpoints

- `POST /webhook` - Main webhook for camera events
- `POST /test` - Send test notification
- `GET /health` - Health check
- `GET /stats` - View statistics
- `GET /config` - View current configuration

## Monitoring

Check server health:
```bash
curl http://localhost:8888/health
```

View statistics:
```bash
curl http://localhost:8888/stats
```

Watch logs:
```bash
tail -f camera_relay.log
```

## Running as a Service

### Linux (systemd)

Create `/etc/systemd/system/camera-relay.service`:

```ini
[Unit]
Description=Security Camera SMS Relay
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/camera-relay
ExecStart=/usr/bin/python3 /home/pi/camera-relay/twilio_camera_relay.py
Restart=always

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable camera-relay
sudo systemctl start camera-relay
```

### Docker

Create a `Dockerfile`:

```dockerfile
FROM python:3.9-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY twilio_camera_relay.py .
EXPOSE 8888
CMD ["python", "twilio_camera_relay.py"]
```

Build and run:
```bash
docker build -t camera-relay .
docker run -d -p 8888:8888 -v $(pwd)/config.yaml:/app/config.yaml camera-relay
```

## Troubleshooting

### No SMS Received

1. Check Twilio credentials in `config.yaml`
2. Verify phone numbers include country code (+1 for US)
3. Check `camera_relay.log` for errors
4. Test Twilio connection: `curl http://localhost:8888/health`

### Camera Not Sending Events

1. Verify camera webhook URL is correct
2. Check firewall allows camera → server communication
3. Test manually: `curl -X POST http://localhost:8888/webhook -d '{"camera_id":"test"}'`
4. Enable debug logging in camera settings

### Too Many/Few Notifications

1. Adjust rate limiting in `config.yaml`
2. Enable/disable batching
3. Configure quiet hours
4. Set per-camera limits

## Cost

- Twilio phone number: ~$1/month
- SMS messages: ~$0.0075 per message (US)
- Typical usage (100 alerts/month): ~$1.75/month total

## Security Considerations

1. Run on isolated network segment if possible
2. Use webhook authentication
3. Implement IP whitelisting for cameras
4. Keep Twilio credentials secure
5. Regularly review logs for suspicious activity

## License

MIT License - feel free to modify and use as needed!
