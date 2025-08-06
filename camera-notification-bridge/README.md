# Security Camera Notification Bridge

A flexible system to send security camera notifications to mobile phones through local SMS gateways, even when your camera system doesn't support it natively.

## Features

- Webhook receiver for camera events
- Multiple SMS gateway support (Twilio, local GSM modem, email-to-SMS)
- Support for various camera systems (Hikvision, Dahua, generic ONVIF)
- Motion detection alerts
- Customizable notification rules
- Local network operation (no cloud dependency)

## Architecture

```
Camera System → Webhook/API → Notification Bridge → SMS Gateway → Mobile Phone
```

## Requirements

- Python 3.8+
- Security camera system with webhook/API support
- One of the following for SMS:
  - GSM modem connected to server
  - Twilio account (or similar SMS service)
  - Email-to-SMS gateway access

## Quick Start

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. Configure your camera system and SMS gateway in `config.yaml`

3. Run the notification bridge:
   ```bash
   python main.py
   ```

## Supported Camera Systems

- Hikvision (via ISAPI)
- Dahua (via HTTP API)
- Generic ONVIF cameras
- Any system with webhook support

## SMS Gateway Options

1. **Local GSM Modem** - Use a USB GSM modem for direct SMS
2. **Twilio** - Cloud SMS service with local fallback
3. **Email-to-SMS** - Use carrier gateways (e.g., number@vtext.com for Verizon)