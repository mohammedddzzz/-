#!/usr/bin/env python3
"""
Security Camera Notification Bridge
Main application entry point
"""

import os
import sys
import logging
import signal
import asyncio
from datetime import datetime
from pathlib import Path

import yaml
from flask import Flask, request, jsonify
from dotenv import load_dotenv

from src.webhook_receiver import WebhookReceiver
from src.sms_gateway import SMSGatewayManager
from src.camera_manager import CameraManager
from src.notification_manager import NotificationManager
from src.database import EventDatabase

# Load environment variables
load_dotenv()

# Create necessary directories
Path("logs").mkdir(exist_ok=True)
Path("data").mkdir(exist_ok=True)

# Load configuration
def load_config():
    config_path = os.getenv('CONFIG_PATH', 'config.yaml')
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

# Setup logging
def setup_logging(config):
    log_config = config.get('logging', {})
    
    logging.basicConfig(
        level=getattr(logging, log_config.get('level', 'INFO')),
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_config.get('file', 'logs/notification-bridge.log')),
            logging.StreamHandler(sys.stdout)
        ]
    )
    
    return logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)

# Global instances
config = None
logger = None
webhook_receiver = None
sms_manager = None
camera_manager = None
notification_manager = None
event_db = None

def initialize_components():
    """Initialize all system components"""
    global config, logger, webhook_receiver, sms_manager, camera_manager, notification_manager, event_db
    
    config = load_config()
    logger = setup_logging(config)
    
    logger.info("Starting Security Camera Notification Bridge...")
    
    # Initialize database
    if config.get('advanced', {}).get('database', {}).get('enabled', True):
        event_db = EventDatabase(config['advanced']['database']['path'])
        logger.info("Database initialized")
    
    # Initialize SMS gateway manager
    sms_manager = SMSGatewayManager(config['sms_gateway'])
    logger.info("SMS Gateway Manager initialized")
    
    # Initialize camera manager
    camera_manager = CameraManager(config['cameras'])
    logger.info("Camera Manager initialized")
    
    # Initialize notification manager
    notification_manager = NotificationManager(
        config['notifications'],
        sms_manager,
        event_db
    )
    logger.info("Notification Manager initialized")
    
    # Initialize webhook receiver
    webhook_receiver = WebhookReceiver(
        notification_manager,
        camera_manager,
        config.get('advanced', {}).get('webhook_auth', {})
    )
    logger.info("Webhook Receiver initialized")

# Flask routes
@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'components': {
            'sms_gateway': sms_manager.get_status() if sms_manager else 'not_initialized',
            'cameras': camera_manager.get_camera_count() if camera_manager else 0,
            'database': 'connected' if event_db else 'disabled'
        }
    })

@app.route(config.get('server', {}).get('webhook_path', '/camera/event'), methods=['POST'])
def camera_event():
    """Main webhook endpoint for camera events"""
    try:
        # Verify authentication if enabled
        auth_config = config.get('advanced', {}).get('webhook_auth', {})
        if auth_config.get('enabled', False):
            token = request.headers.get('Authorization', '').replace('Bearer ', '')
            if token != auth_config.get('token'):
                return jsonify({'error': 'Unauthorized'}), 401
        
        # Process the event
        event_data = request.get_json()
        result = webhook_receiver.process_event(event_data)
        
        return jsonify(result), 200 if result['success'] else 400
        
    except Exception as e:
        logger.error(f"Error processing camera event: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/events', methods=['GET'])
def get_events():
    """Get recent events from database"""
    if not event_db:
        return jsonify({'error': 'Database not enabled'}), 404
        
    limit = request.args.get('limit', 100, type=int)
    camera_id = request.args.get('camera_id')
    
    events = event_db.get_recent_events(limit, camera_id)
    return jsonify({'events': events})

@app.route('/api/test-notification', methods=['POST'])
def test_notification():
    """Send a test notification"""
    try:
        data = request.get_json()
        recipient = data.get('recipient')
        message = data.get('message', 'Test notification from Camera Bridge')
        
        success = sms_manager.send_sms(recipient, message)
        
        return jsonify({
            'success': success,
            'message': 'Test notification sent' if success else 'Failed to send notification'
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

def signal_handler(signum, frame):
    """Handle shutdown signals"""
    logger.info("Shutdown signal received, cleaning up...")
    
    if camera_manager:
        camera_manager.cleanup()
    if sms_manager:
        sms_manager.cleanup()
    if event_db:
        event_db.close()
        
    sys.exit(0)

def main():
    """Main entry point"""
    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Initialize all components
    initialize_components()
    
    # Start camera polling if configured
    if camera_manager:
        camera_manager.start_polling()
    
    # Start Flask server
    server_config = config.get('server', {})
    app.run(
        host=server_config.get('host', '0.0.0.0'),
        port=server_config.get('port', 8080),
        debug=server_config.get('debug', False)
    )

if __name__ == '__main__':
    main()