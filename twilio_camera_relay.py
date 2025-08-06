#!/usr/bin/env python3
"""
Security Camera SMS Relay using Twilio
Local webhook receiver that forwards camera alerts to phones via Twilio SMS
"""

import os
import json
import time
import logging
import threading
from datetime import datetime, timedelta
from collections import defaultdict
from typing import Dict, List, Optional

from flask import Flask, request, jsonify
from twilio.rest import Client
from twilio.base.exceptions import TwilioRestException
import yaml

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('camera_relay.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Default configuration
DEFAULT_CONFIG = {
    "server": {
        "host": "0.0.0.0",
        "port": 8888,
        "debug": False
    },
    
    "twilio": {
        "account_sid": "YOUR_TWILIO_ACCOUNT_SID",
        "auth_token": "YOUR_TWILIO_AUTH_TOKEN",
        "from_number": "+1234567890",  # Your Twilio phone number
        "verify_ssl": True,
        "retry_attempts": 3,
        "retry_delay": 5
    },
    
    "recipients": [
        {
            "name": "Primary Contact",
            "number": "+1234567890",
            "cameras": ["all"],  # or specific camera IDs
            "priority": "high",
            "active": True
        }
    ],
    
    "notifications": {
        "rate_limiting": {
            "enabled": True,
            "max_per_hour": 30,
            "max_per_camera_per_hour": 10
        },
        "batching": {
            "enabled": True,
            "window_seconds": 300,  # 5 minutes
            "max_batch_size": 5
        },
        "quiet_hours": {
            "enabled": False,
            "start": "22:00",
            "end": "07:00",
            "allow_priority": True
        },
        "message_format": "{camera_name}: {event_type} detected at {time}"
    },
    
    "cameras": {
        "camera_id_mapping": {
            "192.168.1.100": "Front Door",
            "192.168.1.101": "Backyard",
            "192.168.1.102": "Garage",
            "cam1": "Living Room",
            "cam2": "Driveway"
        }
    },
    
    "webhook": {
        "auth_enabled": False,
        "auth_token": "your_webhook_secret",
        "allowed_ips": []  # Empty means allow all
    }
}


class ConfigManager:
    """Manages configuration with live reload support"""
    
    def __init__(self, config_file='config.yaml'):
        self.config_file = config_file
        self.config = DEFAULT_CONFIG.copy()
        self.load_config()
        
    def load_config(self):
        """Load configuration from file"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    loaded_config = yaml.safe_load(f)
                    if loaded_config:
                        self.config.update(loaded_config)
                        logger.info(f"Configuration loaded from {self.config_file}")
            else:
                # Save default config
                self.save_config()
                logger.info(f"Created default configuration file: {self.config_file}")
        except Exception as e:
            logger.error(f"Error loading config: {e}")
            
    def save_config(self):
        """Save current configuration to file"""
        try:
            with open(self.config_file, 'w') as f:
                yaml.dump(self.config, f, default_flow_style=False, sort_keys=False)
        except Exception as e:
            logger.error(f"Error saving config: {e}")
            
    def get(self, key_path: str, default=None):
        """Get config value by dot notation path"""
        keys = key_path.split('.')
        value = self.config
        for key in keys:
            if isinstance(value, dict) and key in value:
                value = value[key]
            else:
                return default
        return value


class TwilioSender:
    """Handles Twilio SMS sending with retry logic"""
    
    def __init__(self, config: Dict):
        self.config = config
        self.client = None
        self.init_client()
        
    def init_client(self):
        """Initialize Twilio client"""
        try:
            self.client = Client(
                self.config['account_sid'],
                self.config['auth_token']
            )
            # Test the connection
            self.client.api.accounts(self.config['account_sid']).fetch()
            logger.info("Twilio client initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize Twilio client: {e}")
            self.client = None
            
    def send_sms(self, to_number: str, message: str) -> bool:
        """Send SMS with retry logic"""
        if not self.client:
            logger.error("Twilio client not initialized")
            return False
            
        retry_attempts = self.config.get('retry_attempts', 3)
        retry_delay = self.config.get('retry_delay', 5)
        
        for attempt in range(retry_attempts):
            try:
                msg = self.client.messages.create(
                    body=message[:1600],  # Twilio SMS limit
                    from_=self.config['from_number'],
                    to=to_number
                )
                logger.info(f"SMS sent successfully to {to_number}, SID: {msg.sid}")
                return True
                
            except TwilioRestException as e:
                logger.error(f"Twilio error (attempt {attempt + 1}): {e}")
                if e.code == 20003:  # Authentication error
                    logger.error("Authentication failed - check your credentials")
                    return False
                elif e.code == 21211:  # Invalid 'To' Phone Number
                    logger.error(f"Invalid phone number: {to_number}")
                    return False
                    
            except Exception as e:
                logger.error(f"Unexpected error sending SMS (attempt {attempt + 1}): {e}")
                
            if attempt < retry_attempts - 1:
                time.sleep(retry_delay)
                
        return False


class RateLimiter:
    """Implements rate limiting for notifications"""
    
    def __init__(self):
        self.sent_messages = defaultdict(list)
        self.camera_messages = defaultdict(lambda: defaultdict(list))
        self.lock = threading.Lock()
        
    def can_send(self, recipient: str, camera_id: str, config: Dict) -> bool:
        """Check if message can be sent based on rate limits"""
        if not config.get('enabled', True):
            return True
            
        with self.lock:
            now = datetime.now()
            hour_ago = now - timedelta(hours=1)
            
            # Clean old entries
            self.sent_messages[recipient] = [
                ts for ts in self.sent_messages[recipient] 
                if ts > hour_ago
            ]
            self.camera_messages[recipient][camera_id] = [
                ts for ts in self.camera_messages[recipient][camera_id]
                if ts > hour_ago
            ]
            
            # Check global limit
            max_per_hour = config.get('max_per_hour', 30)
            if len(self.sent_messages[recipient]) >= max_per_hour:
                logger.warning(f"Rate limit exceeded for {recipient}")
                return False
                
            # Check per-camera limit
            max_per_camera = config.get('max_per_camera_per_hour', 10)
            if len(self.camera_messages[recipient][camera_id]) >= max_per_camera:
                logger.warning(f"Camera rate limit exceeded for {camera_id} to {recipient}")
                return False
                
            return True
            
    def record_sent(self, recipient: str, camera_id: str):
        """Record that a message was sent"""
        with self.lock:
            now = datetime.now()
            self.sent_messages[recipient].append(now)
            self.camera_messages[recipient][camera_id].append(now)


class NotificationBatcher:
    """Batches notifications to reduce SMS spam"""
    
    def __init__(self, config: Dict, sender: TwilioSender, rate_limiter: RateLimiter):
        self.config = config
        self.sender = sender
        self.rate_limiter = rate_limiter
        self.pending = defaultdict(list)
        self.first_event_time = {}
        self.lock = threading.Lock()
        self.running = True
        
        # Start batch processor thread
        self.thread = threading.Thread(target=self._process_batches)
        self.thread.daemon = True
        self.thread.start()
        
    def add_notification(self, recipient: Dict, camera_id: str, event: Dict):
        """Add notification to batch"""
        if not self.config.get('enabled', True):
            # Send immediately if batching disabled
            self._send_immediate(recipient, camera_id, event)
            return
            
        with self.lock:
            key = (recipient['number'], camera_id)
            
            if key not in self.first_event_time:
                self.first_event_time[key] = datetime.now()
                
            self.pending[key].append({
                'recipient': recipient,
                'camera_id': camera_id,
                'event': event,
                'time': datetime.now()
            })
            
    def _send_immediate(self, recipient: Dict, camera_id: str, event: Dict):
        """Send notification immediately"""
        if not self.rate_limiter.can_send(recipient['number'], camera_id, 
                                         config_manager.get('notifications.rate_limiting', {})):
            return
            
        message = self._format_message([event], camera_id)
        if self.sender.send_sms(recipient['number'], message):
            self.rate_limiter.record_sent(recipient['number'], camera_id)
            
    def _process_batches(self):
        """Process pending notification batches"""
        while self.running:
            time.sleep(10)  # Check every 10 seconds
            
            with self.lock:
                window = self.config.get('window_seconds', 300)
                max_batch = self.config.get('max_batch_size', 5)
                now = datetime.now()
                
                # Process each pending batch
                for key in list(self.pending.keys()):
                    if key not in self.pending or not self.pending[key]:
                        continue
                        
                    first_time = self.first_event_time.get(key)
                    if not first_time:
                        continue
                        
                    # Check if window expired or batch is full
                    elapsed = (now - first_time).total_seconds()
                    if elapsed >= window or len(self.pending[key]) >= max_batch:
                        self._send_batch(key)
                        
    def _send_batch(self, key):
        """Send a batch of notifications"""
        notifications = self.pending.get(key, [])
        if not notifications:
            return
            
        recipient_number, camera_id = key
        recipient = notifications[0]['recipient']
        
        # Check rate limit
        if not self.rate_limiter.can_send(recipient_number, camera_id,
                                         config_manager.get('notifications.rate_limiting', {})):
            # Keep in pending for next window
            return
            
        # Format message
        events = [n['event'] for n in notifications]
        message = self._format_message(events, camera_id)
        
        # Send SMS
        if self.sender.send_sms(recipient_number, message):
            self.rate_limiter.record_sent(recipient_number, camera_id)
            
        # Clear batch
        del self.pending[key]
        if key in self.first_event_time:
            del self.first_event_time[key]
            
    def _format_message(self, events: List[Dict], camera_id: str) -> str:
        """Format notification message"""
        camera_name = config_manager.get('cameras.camera_id_mapping', {}).get(
            camera_id, camera_id
        )
        
        if len(events) == 1:
            # Single event
            event = events[0]
            template = config_manager.get('notifications.message_format',
                                        '{camera_name}: {event_type} at {time}')
            return template.format(
                camera_name=camera_name,
                event_type=event.get('event_type', 'Motion'),
                time=datetime.now().strftime('%H:%M')
            )
        else:
            # Multiple events
            message = f"🚨 {camera_name}: {len(events)} alerts\n"
            for event in events[-3:]:  # Last 3 events
                time_str = event.get('timestamp', datetime.now()).strftime('%H:%M')
                message += f"• {event.get('event_type', 'Motion')} at {time_str}\n"
            return message
            
    def stop(self):
        """Stop the batcher"""
        self.running = False
        if self.thread:
            self.thread.join()


class CameraEventProcessor:
    """Processes camera events and triggers notifications"""
    
    def __init__(self, config_manager, twilio_sender, rate_limiter, batcher):
        self.config_manager = config_manager
        self.twilio_sender = twilio_sender
        self.rate_limiter = rate_limiter
        self.batcher = batcher
        
    def process_event(self, data: Dict) -> Dict:
        """Process incoming camera event"""
        try:
            # Normalize event data
            event = self._normalize_event(data)
            camera_id = event['camera_id']
            
            logger.info(f"Processing event from camera: {camera_id}")
            
            # Check quiet hours
            if self._in_quiet_hours() and event.get('priority') != 'high':
                logger.info("Event suppressed during quiet hours")
                return {'success': True, 'message': 'Quiet hours'}
                
            # Find recipients for this camera
            recipients = self._get_recipients_for_camera(camera_id)
            
            if not recipients:
                logger.warning(f"No recipients configured for camera: {camera_id}")
                return {'success': True, 'message': 'No recipients'}
                
            # Queue notifications
            for recipient in recipients:
                self.batcher.add_notification(recipient, camera_id, event)
                
            return {
                'success': True,
                'message': f'Notifications queued for {len(recipients)} recipients'
            }
            
        except Exception as e:
            logger.error(f"Error processing event: {e}")
            return {'success': False, 'error': str(e)}
            
    def _normalize_event(self, data: Dict) -> Dict:
        """Normalize event data from different camera systems"""
        
        # Hikvision format
        if 'EventNotificationAlert' in data:
            alert = data['EventNotificationAlert']
            return {
                'camera_id': alert.get('ipAddress', 'unknown'),
                'event_type': alert.get('eventType', 'motion'),
                'timestamp': alert.get('dateTime', datetime.now()),
                'priority': 'high' if alert.get('eventType') == 'fielddetection' else 'normal',
                'raw': data
            }
            
        # Dahua format
        elif 'Action' in data and data['Action'] == 'Pulse':
            return {
                'camera_id': data.get('SerialID', data.get('DeviceID', 'unknown')),
                'event_type': self._parse_dahua_code(data.get('Code', 'VideoMotion')),
                'timestamp': datetime.now(),
                'priority': 'high' if 'Alarm' in data.get('Code', '') else 'normal',
                'raw': data
            }
            
        # Blue Iris format
        elif 'camera' in data:
            return {
                'camera_id': data.get('camera', 'unknown'),
                'event_type': data.get('trigger', 'motion'),
                'timestamp': datetime.now(),
                'priority': data.get('priority', 'normal'),
                'raw': data
            }
            
        # Generic format
        else:
            return {
                'camera_id': data.get('camera_id', data.get('id', 'unknown')),
                'event_type': data.get('event_type', data.get('type', 'motion')),
                'timestamp': data.get('timestamp', datetime.now()),
                'priority': data.get('priority', 'normal'),
                'raw': data
            }
            
    def _parse_dahua_code(self, code: str) -> str:
        """Parse Dahua event codes to readable format"""
        code_map = {
            'VideoMotion': 'Motion detected',
            'VideoLoss': 'Video loss',
            'VideoBlind': 'Camera blocked',
            'AlarmLocal': 'Local alarm',
            'CrossLineDetection': 'Line crossed',
            'CrossRegionDetection': 'Region intrusion',
            'LeftDetection': 'Object left',
            'TakenAwayDetection': 'Object removed'
        }
        return code_map.get(code, code)
        
    def _in_quiet_hours(self) -> bool:
        """Check if current time is in quiet hours"""
        quiet_config = self.config_manager.get('notifications.quiet_hours', {})
        if not quiet_config.get('enabled', False):
            return False
            
        now = datetime.now().time()
        start = datetime.strptime(quiet_config['start'], '%H:%M').time()
        end = datetime.strptime(quiet_config['end'], '%H:%M').time()
        
        # Handle overnight quiet hours
        if start > end:
            return now >= start or now <= end
        else:
            return start <= now <= end
            
    def _get_recipients_for_camera(self, camera_id: str) -> List[Dict]:
        """Get list of recipients for a specific camera"""
        recipients = []
        
        for recipient in self.config_manager.get('recipients', []):
            if not recipient.get('active', True):
                continue
                
            cameras = recipient.get('cameras', [])
            if 'all' in cameras or camera_id in cameras:
                recipients.append(recipient)
                
        return recipients


# Global instances
config_manager = None
twilio_sender = None
rate_limiter = None
batcher = None
event_processor = None


@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'twilio_connected': twilio_sender.client is not None if twilio_sender else False,
        'config_file': config_manager.config_file if config_manager else None
    })


@app.route('/webhook', methods=['POST'])
@app.route('/camera/event', methods=['POST'])
@app.route('/alert', methods=['POST'])
def camera_webhook():
    """Main webhook endpoint for camera events"""
    # Check authentication
    webhook_config = config_manager.get('webhook', {})
    
    if webhook_config.get('auth_enabled', False):
        auth_header = request.headers.get('Authorization', '')
        if auth_header.replace('Bearer ', '') != webhook_config.get('auth_token', ''):
            logger.warning(f"Unauthorized webhook attempt from {request.remote_addr}")
            return jsonify({'error': 'Unauthorized'}), 401
            
    # Check IP whitelist
    allowed_ips = webhook_config.get('allowed_ips', [])
    if allowed_ips and request.remote_addr not in allowed_ips:
        logger.warning(f"Webhook attempt from non-whitelisted IP: {request.remote_addr}")
        return jsonify({'error': 'IP not allowed'}), 403
        
    # Process event
    try:
        data = request.get_json(force=True)
        logger.debug(f"Received webhook data: {json.dumps(data, indent=2)}")
        
        result = event_processor.process_event(data)
        return jsonify(result), 200 if result['success'] else 400
        
    except Exception as e:
        logger.error(f"Webhook processing error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/test', methods=['POST'])
def test_notification():
    """Test endpoint to trigger a test notification"""
    data = request.get_json() or {}
    
    test_event = {
        'camera_id': data.get('camera_id', 'test_camera'),
        'event_type': data.get('message', 'Test notification'),
        'priority': data.get('priority', 'high')
    }
    
    result = event_processor.process_event(test_event)
    return jsonify(result)


@app.route('/config', methods=['GET'])
def get_config():
    """Get current configuration (with sensitive data removed)"""
    safe_config = config_manager.config.copy()
    
    # Remove sensitive data
    if 'twilio' in safe_config:
        safe_config['twilio']['auth_token'] = '***hidden***'
        
    return jsonify(safe_config)


@app.route('/stats', methods=['GET'])
def get_stats():
    """Get notification statistics"""
    stats = {
        'rate_limits': {},
        'pending_notifications': 0
    }
    
    # Get rate limit stats
    for recipient in config_manager.get('recipients', []):
        number = recipient['number']
        stats['rate_limits'][recipient['name']] = {
            'sent_last_hour': len(rate_limiter.sent_messages.get(number, []))
        }
        
    # Get pending notification count
    stats['pending_notifications'] = sum(len(notifications) 
                                       for notifications in batcher.pending.values())
    
    return jsonify(stats)


def initialize():
    """Initialize all components"""
    global config_manager, twilio_sender, rate_limiter, batcher, event_processor
    
    # Load configuration
    config_manager = ConfigManager()
    
    # Initialize Twilio
    twilio_sender = TwilioSender(config_manager.get('twilio', {}))
    
    # Initialize rate limiter
    rate_limiter = RateLimiter()
    
    # Initialize batcher
    batcher = NotificationBatcher(
        config_manager.get('notifications.batching', {}),
        twilio_sender,
        rate_limiter
    )
    
    # Initialize event processor
    event_processor = CameraEventProcessor(
        config_manager,
        twilio_sender,
        rate_limiter,
        batcher
    )
    
    logger.info("All components initialized")


def print_setup_guide():
    """Print setup guide for users"""
    print("\n" + "="*60)
    print("SECURITY CAMERA SMS RELAY - SETUP GUIDE")
    print("="*60 + "\n")
    
    print("1. CONFIGURE TWILIO:")
    print("   - Sign up at https://www.twilio.com")
    print("   - Get your Account SID and Auth Token")
    print("   - Buy a phone number ($1/month)")
    print("   - Update config.yaml with your credentials\n")
    
    print("2. CONFIGURE YOUR CAMERAS:")
    print("\n   HIKVISION:")
    print("   - Go to: Configuration → Event → Basic Event → Motion Detection")
    print("   - Enable motion detection")
    print("   - Go to: Configuration → Event → Basic Event → Notify Surveillance Center")
    print(f"   - URL: http://YOUR_SERVER_IP:{config_manager.get('server.port', 8888)}/webhook")
    print("   - Protocol: HTTP")
    
    print("\n   DAHUA:")
    print("   - Go to: Setting → Event → Video Detection → Motion Detection")
    print("   - Enable motion detection")
    print("   - Go to: Setting → Network → Alarm Server")
    print(f"   - Server Address: YOUR_SERVER_IP")
    print(f"   - Port: {config_manager.get('server.port', 8888)}")
    print("   - URL: /webhook")
    
    print("\n   BLUE IRIS:")
    print("   - Camera Properties → Alerts → Actions")
    print("   - On Alert: Web request or MQTT")
    print(f"   - URL: http://YOUR_SERVER_IP:{config_manager.get('server.port', 8888)}/webhook")
    print('   - POST text: {"camera_id":"&CAM","event_type":"motion"}')
    
    print("\n   GENERIC/ONVIF:")
    print("   - Configure HTTP notification/webhook")
    print(f"   - URL: http://YOUR_SERVER_IP:{config_manager.get('server.port', 8888)}/webhook")
    print("   - Method: POST")
    print("   - Format: JSON\n")
    
    print("3. TEST YOUR SETUP:")
    print(f"   curl -X POST http://localhost:{config_manager.get('server.port', 8888)}/test")
    print("   This will send a test notification to all configured recipients\n")
    
    print("4. MONITOR:")
    print(f"   - Health: http://YOUR_SERVER_IP:{config_manager.get('server.port', 8888)}/health")
    print(f"   - Stats: http://YOUR_SERVER_IP:{config_manager.get('server.port', 8888)}/stats")
    print(f"   - Logs: tail -f camera_relay.log\n")
    
    print("="*60 + "\n")


def main():
    """Main entry point"""
    # Initialize components
    initialize()
    
    # Print setup guide
    print_setup_guide()
    
    # Start Flask server
    try:
        app.run(
            host=config_manager.get('server.host', '0.0.0.0'),
            port=config_manager.get('server.port', 8888),
            debug=config_manager.get('server.debug', False)
        )
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        batcher.stop()
    except Exception as e:
        logger.error(f"Server error: {e}")
        batcher.stop()


if __name__ == '__main__':
    main()